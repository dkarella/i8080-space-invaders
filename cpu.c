#include "cpu.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

size_t op_cycles[] = {
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7, 4,
    4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7, 4,
    4,  10, 16, 5,  5,  5,  7,  4,  4,  10, 16, 5,  5,  5,  7, 4,
    4,  10, 13, 5,  10, 10, 10, 4,  4,  10, 13, 5,  5,  5,  7, 4,

    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7, 5,
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7, 5,
    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7, 5,
    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7, 5,

    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7, 4,
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7, 4,
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7, 4,
    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7, 4,

    11, 10, 10, 10, 17, 11, 7,  11, 11, 10, 10, 10, 10, 17, 7, 11,
    11, 10, 10, 10, 17, 11, 7,  11, 11, 10, 10, 10, 10, 17, 7, 11,
    11, 10, 10, 18, 17, 11, 7,  11, 11, 5,  10, 5,  17, 17, 7, 11,
    11, 10, 10, 4,  17, 11, 7,  11, 11, 5,  10, 4,  17, 17, 7, 11,
};

cpu* cpu_new(size_t memsize) {
        cpu* state = malloc(sizeof(cpu));
        if (!state) {
                return 0;
        }

        *state = (cpu){0};
        state->memory = calloc(memsize, sizeof(uint8_t));
        if (!state->memory) {
                return 0;
        }

        return state;
}

void cpu_delete(cpu* state) {
        if (!state) {
                return;
        }
        free(state->memory);
        free(state);
}

void cpu_write(cpu* state, uint16_t addr, uint8_t data) {
        if (addr < 0x2000) {
#ifndef CPUDIAG
                fprintf(stderr, "tried to write to ROM: $%04x #$%02x\n", addr,
                        data);
                return;
#endif
        } else if (addr >= 0x4000) {
                fprintf(
                    stderr,
                    "tried to write to inaccessible address: $%04x #$%02x\n",
                    addr, data);
                return;
        }
        state->memory[addr] = data;
}

uint8_t cpu_read(cpu const* state, uint16_t addr) {
        if (addr >= 0x4000) {
                fprintf(stderr,
                        "tried to read from inaccessible address: $%04x\n",
                        addr);
                return 0;
        }
        return state->memory[addr];
}

void unimplementedInstruction(uint8_t opcode) {
        fprintf(stderr, "Error: Unimplemnted instruction: 0x%02x\n", opcode);
        exit(1);
}

uint8_t parity(uint8_t n) {
        size_t parity = 1;
        while (n != 0) {
                if (n & 0x01) {
                        parity = !parity;
                }
                n = n >> 1;
        }
        return parity;
}

void add(cpu* state, uint8_t d) {
        uint16_t x = (uint16_t)state->a + d;
        state->cc.z = (x & 0xff) == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.cy = x > 0xff;
        state->cc.p = parity(x & 0xff);
        state->a = x & 0xff;
}

void sub(cpu* state, uint8_t d) {
        uint8_t a = state->a;
        uint8_t x = state->a - d;
        state->cc.z = x == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.cy = a < d;
        state->cc.p = parity(x);
        state->a = x;
}

void inr(cpu* state, uint8_t* p) {
        uint8_t x = *p + 1;
        state->cc.z = (x & 0xff) == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.p = parity(x & 0xff);
        *p = x;
}

void dcr(cpu* state, uint8_t* p) {
        uint8_t x = *p - 1;
        state->cc.z = (x & 0xff) == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.p = parity(x & 0xff);
        *p = x;
}

void logic(cpu* state, uint8_t x) {
        state->cc.z = (x == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.cy = 0;
        state->cc.p = parity(x);
        state->a = x;
}

void and (cpu * state, uint8_t d) { logic(state, state->a & d); }

void xra(cpu* state, uint8_t d) { logic(state, state->a ^ d); }

void ora(cpu* state, uint8_t d) { logic(state, state->a | d); }

void cmp(cpu* state, uint8_t d) {
        uint8_t a = state->a;
        sub(state, d);
        state->a = a;
}

void dad(cpu* state, uint16_t dd) {
        uint16_t hl = (state->h << 8) | state->l;
        uint32_t sum = hl + dd;
        hl += dd;
        state->h = (sum >> 8) & 0xff;
        state->l = sum & 0xff;
        state->cc.cy = sum > 0xffff;
}

void call(cpu* state) {
        uint16_t ret = state->pc + 2;
        uint16_t addr =
            cpu_read(state, state->pc + 1) << 8 | cpu_read(state, state->pc);

#ifdef CPUDIAG
        if (addr == 0x0689) {
                fprintf(stderr, "error called at: %04x\n", state->pc - 1);
        }
#endif
        cpu_write(state, state->sp - 1, (ret >> 8) & 0xff);
        cpu_write(state, state->sp - 2, (ret & 0xff));
        state->sp -= 2;
        state->pc = addr;
}

void ret(cpu* state) {
        state->pc =
            (cpu_read(state, state->sp + 1) << 8) | cpu_read(state, state->sp);
        state->sp += 2;
}

size_t cpu_emulateOp(cpu* state) {
        uint8_t opcode = cpu_read(state, state->pc);
        state->pc++;
        switch (opcode) {
                case 0x00:  // NOP
                {
                        break;
                }
                case 0x01:  // LXI B d16
                {
                        state->b = cpu_read(state, state->pc + 1);
                        state->c = cpu_read(state, state->pc);
                        state->pc += 2;
                        break;
                }
                case 0x02:  // STAX B
                {
                        uint16_t addr = (state->b << 8) | state->c;
                        cpu_write(state, addr, state->a);
                        break;
                }
                case 0x03:  // INX B
                {
                        uint16_t bc = (state->b << 8) | state->c;
                        bc++;
                        state->b = (bc >> 8) & 0xff;
                        state->c = bc & 0xff;
                        break;
                }
                case 0x04:  // INR B
                {
                        inr(state, &state->b);
                        break;
                }
                case 0x05:  // DCR B
                {
                        dcr(state, &state->b);
                        break;
                }
                case 0x06:  // MVI B d8
                {
                        state->b = cpu_read(state, state->pc);
                        state->pc++;
                        break;
                }
                case 0x07:  // RLC
                {
                        uint8_t x = state->a;
                        state->a = (x << 1) | ((x >> 7) & 1);
                        state->cc.cy = (x >> 7) & 1;
                        break;
                }
                case 0x08: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0x09:  // DAD B
                {
                        uint16_t bc = (state->b << 8) | state->c;
                        dad(state, bc);
                        break;
                }
                case 0x0a:  // LDAX B
                {
                        uint16_t bc = (state->b << 8) | state->c;
                        state->a = cpu_read(state, bc);
                        break;
                }
                case 0x0b:  // DCX B
                {
                        uint16_t bc = (state->b << 8) | state->c;
                        bc--;
                        state->b = (bc >> 8) & 0xff;
                        state->c = bc & 0xff;
                        break;
                }
                case 0x0c:  // INR C
                {
                        inr(state, &state->c);
                        break;
                }
                case 0x0d:  // DCR C
                {
                        dcr(state, &state->c);
                        break;
                }
                case 0x0e:  // MVI C d8
                {
                        state->c = cpu_read(state, state->pc);
                        state->pc++;
                        break;
                }
                case 0x0f:  // RRC (Rotate Accumulator Right)
                {
                        uint8_t x = state->a;
                        state->a = ((x & 1) << 7) | (x >> 1);
                        state->cc.cy = 1 == (x & 1);
                        break;
                }
                case 0x10: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0x11:  // LXI D d16
                {
                        state->d = cpu_read(state, state->pc + 1);
                        state->e = cpu_read(state, state->pc);
                        state->pc += 2;
                        break;
                }
                case 0x12:  // STAX D
                {
                        uint16_t addr = (state->d << 8) | state->e;
                        cpu_write(state, addr, state->a);
                        break;
                }
                case 0x13:  // INX D
                {
                        uint16_t de = (state->d << 8) | state->e;
                        de++;
                        state->d = (de >> 8) & 0xff;
                        state->e = de & 0xff;
                        break;
                }
                case 0x14:  // INR D
                {
                        inr(state, &state->d);
                        break;
                }
                case 0x15:  // DCR D
                {
                        dcr(state, &state->d);
                        break;
                }
                case 0x16:  // MVI D d8
                {
                        state->d = cpu_read(state, state->pc);
                        state->pc++;
                        break;
                }
                case 0x17:  // RAL
                {
                        uint8_t x = state->a;
                        state->a = (x << 1) | (state->cc.cy & 1);
                        state->cc.cy = (x >> 7) & 1;
                        break;
                }
                case 0x18: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0x19:  // DAD D
                {
                        uint16_t de = (state->d << 8) | state->e;
                        dad(state, de);
                        break;
                }
                case 0x1a:  // LDAX D
                {
                        uint16_t de = (state->d << 8) | state->e;
                        state->a = cpu_read(state, de);
                        break;
                }
                case 0x1b:  // DCX D
                {
                        uint16_t de = (state->d << 8) | state->e;
                        de--;
                        state->d = (de >> 8) & 0xff;
                        state->e = de & 0xff;
                        break;
                }
                case 0x1c:  // INR E
                {
                        inr(state, &state->e);
                        break;
                }
                case 0x1d:  // DCR E
                {
                        dcr(state, &state->e);
                        break;
                }
                case 0x1e:  // MVI E d8
                {
                        state->e = cpu_read(state, state->pc);
                        state->pc++;
                        break;
                }
                case 0x1f:  // RAR (rotate accumulator right through carry)
                {
                        uint8_t x = state->a;
                        state->a = (state->cc.cy << 7) | (x >> 1);
                        state->cc.cy = x & 1;
                        break;
                }
                case 0x20: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0x21:  // LXI H d16
                {
                        state->h = cpu_read(state, state->pc + 1);
                        state->l = cpu_read(state, state->pc);
                        state->pc += 2;
                        break;
                }
                case 0x22:  // SHLD
                {
                        uint16_t addr = (cpu_read(state, state->pc + 1) << 8) |
                                        cpu_read(state, state->pc);
                        cpu_write(state, addr + 1, state->h);
                        cpu_write(state, addr, state->l);
                        state->pc += 2;
                        break;
                }
                case 0x23:  // INX H
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        hl++;
                        state->h = (hl >> 8) & 0xff;
                        state->l = hl & 0xff;
                        break;
                }
                case 0x24:  // INR H
                {
                        inr(state, &state->h);
                        break;
                }
                case 0x25:  // DCR H
                {
                        dcr(state, &state->h);
                        break;
                }
                case 0x26:  // MVI H d8
                {
                        state->h = cpu_read(state, state->pc);
                        state->pc++;
                        break;
                }
                case 0x27:  // DAA
                {
                        if ((state->a & 0xf) > 9) {
                                state->a += 6;
                        }
                        if ((state->a & 0xf0) > 0x90) {
                                add(state, 0x60);
                        }
                        break;
                }
                case 0x28: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0x29:  // DAD H
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        dad(state, hl);
                        break;
                }
                case 0x2a:  // LHLD addr
                {
                        uint16_t addr = (cpu_read(state, state->pc + 1) << 8) |
                                        cpu_read(state, state->pc);
                        state->h = cpu_read(state, addr + 1);
                        state->l = cpu_read(state, addr);
                        state->pc += 2;
                        break;
                }
                case 0x2b:  // DCX H
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        hl--;
                        state->h = (hl >> 8) & 0xff;
                        state->l = hl & 0xff;
                        break;
                }
                case 0x2c:  // INR L
                {
                        inr(state, &state->l);
                        break;
                }
                case 0x2d:  // DCR L
                {
                        dcr(state, &state->l);
                        break;
                }
                case 0x2e:  // MVI L d8
                {
                        state->l = cpu_read(state, state->pc);
                        state->pc++;
                        break;
                }
                case 0x2f:  // CMA (not)
                {
                        state->a = ~state->a;
                        // CMA doesn't affect flags
                        break;
                }
                case 0x30: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0x31:  // LXI SP d16
                {
                        state->sp = (cpu_read(state, state->pc + 1) << 8) |
                                    cpu_read(state, state->pc);
                        state->pc += 2;
                        break;
                }
                case 0x32:  // STA adr
                {
                        uint16_t addr = (cpu_read(state, state->pc + 1) << 8) |
                                        cpu_read(state, state->pc);
                        cpu_write(state, addr, state->a);
                        state->pc += 2;
                        break;
                }
                case 0x33:  // INX SP
                {
                        state->sp++;
                        break;
                }
                case 0x34:  // INR M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        if (hl < 0x2000) {
#ifndef CPUDIAG
                                fprintf(
                                    stderr,
                                    "tried to write to ROM (INR M): $%04x\n",
                                    hl);
                                break;
#endif
                        } else if (hl >= 0x4000) {
                                fprintf(
                                    stderr,
                                    "tried to write to inaccessible address "
                                    "(INR M): "
                                    "$%04x\n",
                                    hl);
                                break;
                        }
                        inr(state, &state->memory[hl]);
                        break;
                }
                case 0x35:  // DCR M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        if (hl < 0x2000) {
#ifndef CPUDIAG
                                fprintf(
                                    stderr,
                                    "tried to write to ROM (DCR M): $%04x\n",
                                    hl);
                                break;
#endif
                        } else if (hl >= 0x4000) {
                                fprintf(
                                    stderr,
                                    "tried to write to inaccessible address "
                                    "(DCR M): "
                                    "$%04x\n",
                                    hl);
                                break;
                        }
                        dcr(state, &state->memory[hl]);
                        break;
                }
                case 0x36:  // MVI M d8
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, cpu_read(state, state->pc));
                        state->pc++;
                        break;
                }
                case 0x37:  // STC
                {
                        state->cc.cy = 1;
                        break;
                }
                case 0x38: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0x39:  // DAD SP
                {
                        dad(state, state->sp);
                        break;
                }
                case 0x3a:  // LDA adr
                {
                        uint16_t addr = (cpu_read(state, state->pc + 1) << 8) |
                                        cpu_read(state, state->pc);
                        state->a = cpu_read(state, addr);
                        state->pc += 2;
                        break;
                }
                case 0x3b:  // DCX SP
                {
                        state->sp--;
                        break;
                }
                case 0x3c:  // INR A
                {
                        inr(state, &state->a);
                        break;
                }
                case 0x3d:  // DCR A
                {
                        dcr(state, &state->a);
                        break;
                }
                case 0x3e:  // MVI A d8
                {
                        state->a = cpu_read(state, state->pc);
                        state->pc++;
                        break;
                }
                case 0x3f:  // CMC
                {
                        state->cc.cy = !state->cc.cy;
                        break;
                }
                case 0x40:  // MOV B B
                {
                        state->b = state->b;
                        break;
                }
                case 0x41:  // MOV B C
                        state->b = state->c;
                        break;
                case 0x42:  // MOV B D
                        state->b = state->d;
                        break;
                case 0x43:  // MOV B E
                        state->b = state->e;
                        break;
                case 0x44:  // MOV B H
                {
                        state->b = state->h;
                        break;
                }
                case 0x45:  // MOV B L
                {
                        state->b = state->l;
                        break;
                }
                case 0x46:  // MOV B M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->b = cpu_read(state, hl);
                        break;
                }
                case 0x47:  // MOV B A
                {
                        state->b = state->a;
                        break;
                }
                case 0x48:  // MOV C B
                {
                        state->c = state->b;
                        break;
                }
                case 0x49:  // MOV C C
                {
                        state->c = state->c;
                        break;
                }
                case 0x4a:  // MOV C D
                {
                        state->c = state->d;
                        break;
                }
                case 0x4b:  // MOV C E
                {
                        state->c = state->e;
                        break;
                }
                case 0x4c:  // MOV C H
                {
                        state->c = state->h;
                        break;
                }
                case 0x4d:  // MOV C L
                {
                        state->c = state->l;
                        break;
                }
                case 0x4e:  // MOV C M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->c = cpu_read(state, hl);
                        break;
                }
                case 0x4f:  // MOV C A
                {
                        state->c = state->a;
                        break;
                }
                case 0x50:  // MOV D B
                {
                        state->d = state->b;
                        break;
                }
                case 0x51:  // MOV D C
                {
                        state->d = state->c;
                        break;
                }
                case 0x52:  // MOV D D
                {
                        state->d = state->d;
                        break;
                }
                case 0x53:  // MOV D E
                {
                        state->d = state->e;
                        break;
                }
                case 0x54:  // MOV D H
                {
                        state->d = state->h;
                        break;
                }
                case 0x55:  // MOV D L
                {
                        state->d = state->l;
                        break;
                }
                case 0x56:  // MOV D M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->d = cpu_read(state, hl);
                        break;
                }
                case 0x57:  // MOV D A
                {
                        state->d = state->a;
                        break;
                }
                case 0x58:  // MOV E B
                {
                        state->e = state->b;
                        break;
                }
                case 0x59:  // MOV E C
                {
                        state->e = state->c;
                        break;
                }
                case 0x5a:  // MOV E D
                {
                        state->e = state->d;
                        break;
                }
                case 0x5b:  // MOV E E
                {
                        state->e = state->e;
                        break;
                }
                case 0x5c:  // MOV E H
                {
                        state->e = state->h;
                        break;
                }
                case 0x5d:  // MOV E L
                {
                        state->e = state->l;
                        break;
                }
                case 0x5e:  // MOV E M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->e = cpu_read(state, hl);
                        break;
                }
                case 0x5f:  // MOV E A
                {
                        state->e = state->a;
                        break;
                }
                case 0x60:  // MOV H B
                {
                        state->h = state->b;
                        break;
                }
                case 0x61:  // MOV H C
                {
                        state->h = state->c;
                        break;
                }
                case 0x62:  // MOV H D
                {
                        state->h = state->d;
                        break;
                }
                case 0x63:  // MOV H E
                {
                        state->h = state->e;
                        break;
                }
                case 0x64:  // MOV H H
                {
                        state->h = state->h;
                        break;
                }
                case 0x65:  // MOV H L
                {
                        state->h = state->l;
                        break;
                }
                case 0x66:  // MOV H M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->h = cpu_read(state, hl);
                        break;
                }
                case 0x67:  // MOV H A
                {
                        state->h = state->a;
                        break;
                }
                case 0x68:  // MOV L B
                {
                        state->l = state->b;
                        break;
                }
                case 0x69:  // MOV L C
                {
                        state->l = state->c;
                        break;
                }
                case 0x6a:  // MOV L D
                {
                        state->l = state->d;
                        break;
                }
                case 0x6b:  // MOV L E
                {
                        state->l = state->e;
                        break;
                }
                case 0x6c:  // MOV L H
                {
                        state->l = state->h;
                        break;
                }
                case 0x6d:  // MOV L L
                {
                        state->l = state->l;
                        break;
                }
                case 0x6e:  // MOV L M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->l = cpu_read(state, hl);
                        break;
                }
                case 0x6f:  // MOV L A
                {
                        state->l = state->a;
                        break;
                }
                case 0x70:  // MOV M B
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, state->b);
                        break;
                }
                case 0x71:  // MOV M C
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, state->c);
                        break;
                }
                case 0x72:  // MOV M D
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, state->d);
                        break;
                }
                case 0x73:  // MOV M E
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, state->e);
                        break;
                }
                case 0x74:  // MOV M H
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, state->h);
                        break;
                }
                case 0x75:  // MOV M L
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, state->l);
                        break;
                }
                case 0x76:  // HLT
                {
                        exit(0);
                        break;
                }
                case 0x77:  // MOV M A
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cpu_write(state, hl, state->a);
                        break;
                }
                case 0x78:  // MOV A B
                {
                        state->a = state->b;
                        break;
                }
                case 0x79:  // MOV A C
                {
                        state->a = state->c;
                        break;
                }
                case 0x7a:  // MOV A D
                {
                        state->a = state->d;
                        break;
                }
                case 0x7b:  // MOV A E
                {
                        state->a = state->e;
                        break;
                }
                case 0x7c:  // MOV A H
                {
                        state->a = state->h;
                        break;
                }
                case 0x7d:  // MOV A L
                {
                        state->a = state->l;
                        break;
                }
                case 0x7e:  // MOV A M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->a = cpu_read(state, hl);
                        break;
                }
                case 0x7f:  // MOV A A
                {
                        state->a = state->a;
                        break;
                }
                case 0x80:  // ADD B
                {
                        add(state, state->b);
                        break;
                }
                case 0x81:  // ADD C
                {
                        add(state, state->c);
                        break;
                }
                case 0x82:  // ADD D
                {
                        add(state, state->d);
                        break;
                }
                case 0x83:  // ADD E
                {
                        add(state, state->e);
                        break;
                }
                case 0x84:  // ADD H
                {
                        add(state, state->h);
                        break;
                }
                case 0x85:  // ADD L
                {
                        add(state, state->l);
                        break;
                }
                case 0x86:  // ADD M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        add(state, cpu_read(state, hl));
                        break;
                }
                case 0x87:  // ADD A
                {
                        add(state, state->a);
                        break;
                }
                case 0x88:  // ADC B
                {
                        add(state, state->b + state->cc.cy);
                        break;
                }
                case 0x89:  // ADC C
                {
                        add(state, state->c + state->cc.cy);
                        break;
                }
                case 0x8a:  // ADC D
                {
                        add(state, state->d + state->cc.cy);
                        break;
                }
                case 0x8b:  // ADC E
                {
                        add(state, state->e + state->cc.cy);
                        break;
                }
                case 0x8c:  // ADC H
                {
                        add(state, state->h + state->cc.cy);
                        break;
                }
                case 0x8d:  // ADC L
                {
                        add(state, state->l + state->cc.cy);
                        break;
                }
                case 0x8e:  // ADC M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        add(state, cpu_read(state, hl) + state->cc.cy);
                        break;
                }
                case 0x8f:  // ADC A
                {
                        add(state, state->a + state->cc.cy);
                        break;
                }
                case 0x90:  // SUB B
                {
                        sub(state, state->b);
                        break;
                }
                case 0x91:  // SUB C
                {
                        sub(state, state->c);
                        break;
                }
                case 0x92:  // SUB D
                {
                        sub(state, state->d);
                        break;
                }
                case 0x93:  // SUB E
                {
                        sub(state, state->e);
                        break;
                }
                case 0x94:  // SUB H
                {
                        sub(state, state->h);
                        break;
                }
                case 0x95:  // SUB L
                {
                        sub(state, state->l);
                        break;
                }
                case 0x96:  // SUB M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        sub(state, cpu_read(state, hl));
                        break;
                }
                case 0x97:  // SUB A
                {
                        sub(state, state->a);
                        break;
                }
                case 0x98:  // SBB B
                {
                        sub(state, state->b + state->cc.cy);
                        break;
                }
                case 0x99:  // SBB C
                {
                        sub(state, state->c + state->cc.cy);
                        break;
                }
                case 0x9a:  // SBB D
                {
                        sub(state, state->d + state->cc.cy);
                        break;
                }
                case 0x9b:  // SBB E
                {
                        sub(state, state->e + state->cc.cy);
                        break;
                }
                case 0x9c:  // SBB H
                {
                        sub(state, state->h + state->cc.cy);
                        break;
                }
                case 0x9d:  // SBB L
                {
                        sub(state, state->l + state->cc.cy);
                        break;
                }
                case 0x9e:  // SBB M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        sub(state, cpu_read(state, hl) + state->cc.cy);
                        break;
                }
                case 0x9f:  // SBB A
                {
                        sub(state, state->a + state->cc.cy);
                        break;
                }
                case 0xa0:  // ANA B
                {
                        and(state, state->b);
                        break;
                }
                case 0xa1:  // ANA C
                {
                        and(state, state->c);
                        break;
                }
                case 0xa2:  // ANA D
                {
                        and(state, state->d);
                        break;
                }
                case 0xa3:  // ANA E
                {
                        and(state, state->e);
                        break;
                }
                case 0xa4:  // ANA H
                {
                        and(state, state->h);
                        break;
                }
                case 0xa5:  // ANA L
                {
                        and(state, state->l);
                        break;
                }
                case 0xa6:  // ANA M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        and(state, cpu_read(state, hl));
                        break;
                }
                case 0xa7:  // ANA A
                {
                        and(state, state->a);
                        break;
                }
                case 0xa8:  // XRA B
                {
                        xra(state, state->b);
                        break;
                }
                case 0xa9:  // XRA C
                {
                        xra(state, state->c);
                        break;
                }
                case 0xaa:  // XRA D
                {
                        xra(state, state->d);
                        break;
                }
                case 0xab:  // XRA E
                {
                        xra(state, state->e);
                        break;
                }
                case 0xac:  // XRA H
                {
                        xra(state, state->h);
                        break;
                }
                case 0xad:  // XRA L
                {
                        xra(state, state->l);
                        break;
                }
                case 0xae:  // XRA M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        xra(state, cpu_read(state, hl));
                        break;
                }
                case 0xaf:  // XRA A
                {
                        xra(state, state->a);
                        break;
                }
                case 0xb0:  // ORA B
                {
                        ora(state, state->b);
                        break;
                }
                case 0xb1:  // ORA C
                {
                        ora(state, state->c);
                        break;
                }
                case 0xb2:  // ORA D
                {
                        ora(state, state->d);
                        break;
                }
                case 0xb3:  // ORA E
                {
                        ora(state, state->e);
                        break;
                }
                case 0xb4:  // ORA H
                {
                        ora(state, state->h);
                        break;
                }
                case 0xb5:  // ORA L
                {
                        ora(state, state->l);
                        break;
                }
                case 0xb6:  // ORA M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        ora(state, cpu_read(state, hl));
                        break;
                }
                case 0xb7:  // ORA A
                {
                        ora(state, state->a);
                        break;
                }
                case 0xb8:  // CMP B
                {
                        cmp(state, state->b);
                        break;
                }
                case 0xb9:  // CMP C
                {
                        cmp(state, state->c);
                        break;
                }
                case 0xba:  // CMP D
                {
                        cmp(state, state->d);
                        break;
                }
                case 0xbb:  // CMP E
                {
                        cmp(state, state->e);
                        break;
                }
                case 0xbc:  // CMP H
                {
                        cmp(state, state->h);
                        break;
                }
                case 0xbd:  // CMP L
                {
                        cmp(state, state->l);
                        break;
                }
                case 0xbe:  // CMP M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        cmp(state, cpu_read(state, hl));
                        break;
                }
                case 0xbf:  // CMP A
                {
                        cmp(state, state->a);
                        break;
                }
                case 0xc0:  // RNZ
                {
                        if (state->cc.z) {
                                break;
                        }
                        state->pc = (cpu_read(state, state->sp + 1) << 8) |
                                    cpu_read(state, state->sp);
                        state->sp += 2;
                        break;
                }
                case 0xc1:  // POP B
                {
                        state->b = cpu_read(state, state->sp + 1);
                        state->c = cpu_read(state, state->sp);
                        state->sp += 2;
                        break;
                }
                case 0xc2:  // JNZ addr
                {
                        if (state->cc.z) {
                                state->pc += 2;
                                break;
                        }
                        state->pc = cpu_read(state, state->pc + 1) << 8 |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xc3:  // JMP addr
                {
                        state->pc = cpu_read(state, state->pc + 1) << 8 |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xc4:  // CNZ addr
                {
                        if (!state->cc.z) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xc5:  // PUSH B
                {
                        cpu_write(state, state->sp - 1, state->b);
                        cpu_write(state, state->sp - 2, state->c);
                        state->sp -= 2;
                        break;
                }
                case 0xc6:  // ADI d8
                {
                        add(state, cpu_read(state, state->pc));
                        state->pc++;
                        break;
                }
                case 0xc7: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xc8:  // RZ
                {
                        if (!state->cc.z) {
                                break;
                        }
                        state->pc = (cpu_read(state, state->sp + 1) << 8) |
                                    cpu_read(state, state->sp);
                        state->sp += 2;
                        break;
                }
                case 0xc9:  // RET
                {
                        ret(state);
                        break;
                }
                case 0xca:  // JZ addr
                {
                        if (!state->cc.z) {
                                state->pc += 2;
                                break;
                        }
                        state->pc = (cpu_read(state, state->pc + 1) << 8) |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xcb: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xcc:  // CZ addr
                {
                        if (state->cc.z) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xcd:  // CALL addr
                {
#ifdef CPUDIAG
                        uint16_t addr = cpu_read(state, state->pc + 1) << 8 |
                                        cpu_read(state, state->pc);
                        if (addr == 0x0005) {
                                if (state->c == 9) {
                                        uint16_t offset =
                                            (state->d << 8) | (state->e);
                                        char* str =
                                            (char*)&state
                                                ->memory[offset +
                                                         3];  // skip the
                                                              // prefix bytes
                                        while (*str != '$')
                                                printf("%c", *str++);
                                        printf("\n");
                                } else if (state->c == 2) {
                                        printf("print char routine called\n");
                                        exit(1);
                                }
                        } else if (addr == 0x0000) {
                                exit(0);
                        } else
#endif
                        {
                                call(state);
                                break;
                        }
                }
                case 0xce:  // ACI d8
                {
                        add(state, cpu_read(state, state->pc) + state->cc.cy);
                        state->pc++;
                        break;
                }
                case 0xcf: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xd0:  // RNC
                {
                        if (!state->cc.cy) {
                                ret(state);
                                break;
                        }
                        break;
                }
                case 0xd1:  // POP D
                {
                        state->d = cpu_read(state, state->sp + 1);
                        state->e = cpu_read(state, state->sp);
                        state->sp += 2;
                        break;
                }
                case 0xd2:  // JNC addr
                {
                        if (state->cc.cy) {
                                state->pc += 2;
                                break;
                        }
                        state->pc = cpu_read(state, state->pc + 1) << 8 |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xd3:  // OUT d8
                {
                        // TODO: implement, for now just skip over data byte
                        state->pc++;
                        break;
                }
                case 0xd4:  // CNC addr
                {
                        if (!state->cc.cy) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xd5:  // PUSH D
                {
                        cpu_write(state, state->sp - 1, state->d);
                        cpu_write(state, state->sp - 2, state->e);
                        state->sp -= 2;
                        break;
                }
                case 0xd6:  // SUI d8
                {
                        sub(state, cpu_read(state, state->pc));
                        state->pc++;
                        break;
                }
                case 0xd7: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xd8:  // RC
                {
                        if (state->cc.cy) {
                                ret(state);
                                break;
                        }
                        break;
                }
                case 0xd9: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xda:  // JC addr
                {
                        if (!state->cc.cy) {
                                state->pc += 2;
                                break;
                        }
                        state->pc = (cpu_read(state, state->pc + 1) << 8) |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xdb: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xdc:  // CC addr
                {
                        if (state->cc.cy) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xdd: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xde:  // SBI d8
                {
                        sub(state, cpu_read(state, state->pc) + state->cc.cy);
                        state->pc++;
                        break;
                }
                case 0xdf: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xe0:  // RPO
                {
                        if (!state->cc.p) {
                                ret(state);
                                break;
                        }
                        break;
                }
                case 0xe1:  // POP H
                {
                        state->h = cpu_read(state, state->sp + 1);
                        state->l = cpu_read(state, state->sp);
                        state->sp += 2;
                        break;
                }
                case 0xe2:  // JPO addr
                {
                        if (!state->cc.p) {
                                state->pc =
                                    (cpu_read(state, state->pc + 1) << 8) |
                                    cpu_read(state, state->pc);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xe3:  // XTHL
                {
                        uint8_t tmp = state->h;
                        state->h = cpu_read(state, state->sp + 1);
                        cpu_write(state, state->sp + 1, tmp);
                        tmp = state->l;
                        state->l = cpu_read(state, state->sp);
                        cpu_write(state, state->sp, tmp);
                        break;
                }
                case 0xe4:  // CPO addr
                {
                        if (!state->cc.p) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xe5:  // PUSH H
                {
                        cpu_write(state, state->sp - 1, state->h);
                        cpu_write(state, state->sp - 2, state->l);
                        state->sp -= 2;
                        break;
                }
                case 0xe6:  // ANI d8
                {
                        and(state, cpu_read(state, state->pc));
                        state->pc++;
                        break;
                }
                case 0xe7: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xe8:  // RPE
                {
                        if (state->cc.p) {
                                ret(state);
                                break;
                        }
                        break;
                }
                case 0xe9:  // PCHL
                {
                        state->pc = (state->h << 8) | state->l;
                        break;
                }
                case 0xea:  // JPE addr
                {
                        if (!state->cc.p) {
                                state->pc += 2;
                                break;
                        }
                        state->pc = (cpu_read(state, state->pc + 1) << 8) |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xeb:  // XCHG
                {
                        uint8_t tmp = state->h;
                        state->h = state->d;
                        state->d = tmp;
                        tmp = state->l;
                        state->l = state->e;
                        state->e = tmp;
                        break;
                }
                case 0xec:  // CPE addr
                {
                        if (state->cc.p) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xed: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xee:  // XRI d8
                {
                        xra(state, cpu_read(state, state->pc));
                        state->pc++;
                        break;
                }
                case 0xef: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xf0:  // RP
                {
                        if (!state->cc.s) {
                                ret(state);
                                break;
                        }
                        break;
                }
                case 0xf1:  // POP PSW
                {
                        state->a = cpu_read(state, state->sp + 1);
                        uint8_t psw = cpu_read(state, state->sp);
                        state->cc.z = 0x01 == (psw & 0x01);
                        state->cc.s = 0x02 == (psw & 0x02);
                        state->cc.p = 0x04 == (psw & 0x04);
                        state->cc.cy = 0x08 == (psw & 0x08);
                        state->cc.ac = 0x10 == (psw & 0x10);
                        state->sp += 2;
                        break;
                }
                case 0xf2:  // JP addr
                {
                        if (state->cc.s) {
                                state->pc += 2;
                                break;
                        }
                        state->pc = (cpu_read(state, state->pc + 1) << 8) |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xf3:  // DI
                {
                        state->int_enable = 0;
                        break;
                }
                case 0xf4:  // CP addr
                {
                        if (!state->cc.s) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xf5:  // PUSH PSW
                {
                        cpu_write(state, state->sp - 1, state->a);
                        uint8_t psw =
                            (state->cc.z | state->cc.s << 1 | state->cc.p << 2 |
                             state->cc.cy << 3 | state->cc.ac << 4);
                        cpu_write(state, state->sp - 2, psw);
                        state->sp -= 2;
                        break;
                }
                case 0xf6:  // ORI d8
                {
                        ora(state, cpu_read(state, state->pc));
                        state->pc++;
                        break;
                }
                case 0xf7: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xf8:  // RM
                {
                        if (state->cc.s) {
                                ret(state);
                                break;
                        }
                        break;
                }
                case 0xf9:  // SPHL
                {
                        state->sp = (state->h << 8) | state->l;
                        break;
                }
                case 0xfa:  // JM addr
                {
                        if (!state->cc.s) {
                                state->pc += 2;
                                break;
                        }
                        state->pc = (cpu_read(state, state->pc + 1) << 8) |
                                    cpu_read(state, state->pc);
                        break;
                }
                case 0xfb:  // EI
                {
                        state->int_enable = 1;
                        break;
                }
                case 0xfc:  // CM addr
                {
                        if (state->cc.s) {
                                call(state);
                                break;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xfd: {
                        unimplementedInstruction(opcode);
                        break;
                }
                case 0xfe:  // CPI d8
                {
                        cmp(state, cpu_read(state, state->pc));
                        state->pc++;
                        break;
                }
                case 0xff: {
                        unimplementedInstruction(opcode);
                        break;
                }
                default: {
                        unimplementedInstruction(opcode);
                        break;
                }
        }

        return op_cycles[opcode];
}

void cpu_interrupt(cpu* state, uint8_t interrupt_num) {
        cpu_write(state, state->sp - 1, (state->pc >> 8) & 0xff);
        cpu_write(state, state->sp - 2, state->pc & 0xff);
        state->sp -= 2;

        state->pc = 8 * interrupt_num;

        state->int_enable = 0;
}
