#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ConditionCodes {
        // NOTE: these are "bit fields"
        // the number after the colon describes how many bits that field uses
        uint8_t z : 1;    // set if result is 0
        uint8_t s : 1;    // set if MSB of result is set
        uint8_t p : 1;    // set if result has even parity
        uint8_t cy : 1;   // set if result had a carry or borrow
        uint8_t ac : 1;   // ignored (not used by space invaders)
        uint8_t pad : 3;  // this is just padding to make the struct 8 bits long
} ConditionCodes;

typedef struct State {
        uint8_t a;
        uint8_t b;
        uint8_t c;
        uint8_t d;
        uint8_t e;
        uint8_t h;
        uint8_t l;
        uint16_t sp;
        uint16_t pc;
        uint8_t* memory;
        ConditionCodes cc;
        uint8_t int_enable;
} State;

void UnimplementedInstruction(uint8_t opcode) {
        fprintf(stderr, "Error: Unimplemnted instruction: 0x%02x\n", opcode);
        exit(1);
}

uint8_t Parity(uint8_t n) {
        size_t parity = 0;
        while (n != 0) {
                parity = !parity;
                n &= (n - 1);
        }
        return parity;
}

void Add(State* state, uint8_t d) {
        uint16_t x = (uint16_t)state->a + d;
        state->cc.z = (x & 0xff) == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.cy = x > 0xff;
        state->cc.p = Parity(x & 0xff);
        state->a = x & 0xff;
}

void Sub(State* state, uint8_t d) {
        uint8_t a = state->a;
        uint8_t x = state->a - d;
        state->cc.z = (x & 0xff) == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.cy = a < d;
        state->cc.p = Parity(x & 0xff);
        state->a = x & 0xff;
}

void Inr(State* state, uint8_t* p) {
        uint8_t x = *p + 1;
        state->cc.z = (x & 0xff) == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.p = Parity(x & 0xff);
        *p = x;
}

void Dcr(State* state, uint8_t* p) {
        uint8_t x = *p - 1;
        state->cc.z = (x & 0xff) == 0;
        state->cc.s = (x & 0x80) != 0;
        state->cc.p = Parity(x & 0xff);
        *p = x;
}

void Logic(State* state, uint8_t x) {
        state->cc.z = (x == 0);
        state->cc.s = (0x80 == (x & 0x80));
        state->cc.p = Parity(x);
        state->cc.cy = 0;  // clear CY
        state->a = x;
}

void And(State* state, uint8_t d) { Logic(state, state->a & d); }

void Xra(State* state, uint8_t d) { Logic(state, state->a ^ d); }

void Ora(State* state, uint8_t d) { Logic(state, state->a | d); }

void Cmp(State* state, uint8_t d) {
        uint8_t a = state->a;
        Sub(state, d);
        state->a = a;
}

void EmulateOp(State* state) {
        uint8_t opcode = state->memory[state->pc];
        switch (opcode) {
                case 0x00:  // NOP
                {
                        break;
                }
                case 0x01:  // LXI B d16
                {
                        state->b = state->memory[state->pc + 2];
                        state->c = state->memory[state->pc + 1];
                        state->pc += 2;
                        break;
                }
                case 0x02:  // STAX B
                {
                        uint16_t addr = (state->b << 8) | state->c;
                        state->memory[addr] = state->a;
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
                        Inr(state, &state->b);
                        break;
                }
                case 0x05:  // DCR B
                {
                        Dcr(state, &state->b);
                        break;
                }
                case 0x06:  // MVI B d8
                {
                        state->b = state->memory[state->pc + 1];
                        state->pc++;
                        break;
                }
                case 0x07: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x08: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x09: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x0a: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x0b: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x0c:  // INR C
                {
                        Inr(state, &state->c);
                        break;
                }
                case 0x0d:  // DCR C
                {
                        Dcr(state, &state->c);
                        break;
                }
                case 0x0e:  // MVI C d8
                {
                        state->c = state->memory[state->pc + 1];
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
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x11:  // LXI D d16
                {
                        state->d = state->memory[state->pc + 2];
                        state->e = state->memory[state->pc + 1];
                        state->pc += 2;
                        break;
                }
                case 0x12: {
                        UnimplementedInstruction(opcode);
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
                        Inr(state, &state->d);
                        break;
                }
                case 0x15:  // DCR D
                {
                        Dcr(state, &state->d);
                        break;
                }
                case 0x16:  // MVI D d8
                {
                        state->d = state->memory[state->pc + 1];
                        state->pc++;
                        break;
                }
                case 0x17: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x18: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x19:  // DAD D
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        uint16_t de = (state->d << 8) | state->e;
                        hl += de;
                        state->h = (hl >> 8) & 0xff;
                        state->l = hl & 0xff;
                        state->cc.cy = de < hl;
                        break;
                }
                case 0x1a:  // LDAX D
                {
                        uint16_t de = (state->d << 8) | state->e;
                        state->a = state->memory[de];
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
                        Inr(state, &state->e);
                        break;
                }
                case 0x1d:  // DCR E
                {
                        Dcr(state, &state->e);
                        break;
                }
                case 0x1e:  // MVI E d8
                {
                        state->e = state->memory[state->pc + 1];
                        state->pc++;
                        break;
                }
                case 0x1f:  // RAR (rotate accumulator right through carry)
                {
                        uint8_t x = state->a;
                        state->a = (state->cc.cy << 7) | (x >> 1);
                        state->cc.cy = 1 == (x & 1);
                        break;
                }
                case 0x20: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x21:  // LXI H d16
                {
                        state->h = state->memory[state->pc + 2];
                        state->l = state->memory[state->pc + 1];
                        state->pc += 2;
                        break;
                }
                case 0x22: {
                        UnimplementedInstruction(opcode);
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
                        Inr(state, &state->h);
                        break;
                }
                case 0x25:  // DCR H
                {
                        Dcr(state, &state->h);
                        break;
                }
                case 0x26:  // MVI H d8
                {
                        state->h = state->memory[state->pc + 1];
                        state->pc++;
                        break;
                }
                case 0x27: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x28: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x29: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x2a: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x2b: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x2c:  // INR L
                {
                        Inr(state, &state->l);
                        break;
                }
                case 0x2d:  // DCR L
                {
                        Dcr(state, &state->l);
                        break;
                }
                case 0x2e:  // MVI L d8
                {
                        state->l = state->memory[state->pc + 1];
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
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x31:  // LXI SP d16
                {
                        state->sp = (state->memory[state->pc + 2] << 8) |
                                    state->memory[state->pc + 1];
                        state->pc += 2;
                        break;
                }
                case 0x32:  // STA adr
                {
                        uint16_t addr = (state->memory[state->pc + 2] << 8) |
                                        state->memory[state->pc + 1];
                        state->memory[addr] = state->a;
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
                        Inr(state, &state->memory[hl]);
                        break;
                }
                case 0x35:  // DCR M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Dcr(state, &state->memory[hl]);
                        break;
                }
                case 0x36:  // MVI M d8
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->memory[hl] = state->memory[state->pc + 1];
                        state->pc++;
                        break;
                }
                case 0x37: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x38: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x39: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0x3a:  // LDA adr
                {
                        uint16_t addr = (state->memory[state->pc + 2] << 8) |
                                        state->memory[state->pc + 1];
                        state->a = state->memory[addr];
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
                        Inr(state, &state->a);
                        break;
                }
                case 0x3d:  // DCR A
                {
                        Dcr(state, &state->a);
                        break;
                }
                case 0x3e:  // MVI A d8
                {
                        state->a = state->memory[state->pc + 1];
                        state->pc++;
                        break;
                }
                case 0x3f: {
                        UnimplementedInstruction(opcode);
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
                        state->b = state->memory[hl];
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
                        state->c = state->memory[hl];
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
                        state->d = state->memory[hl];
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
                        state->e = state->memory[hl];
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
                        state->h = state->memory[hl];
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
                        state->l = state->memory[hl];
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
                        state->memory[hl] = state->b;
                        break;
                }
                case 0x71:  // MOV M C
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->memory[hl] = state->c;
                        break;
                }
                case 0x72:  // MOV M D
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->memory[hl] = state->d;
                        break;
                }
                case 0x73:  // MOV M E
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->memory[hl] = state->e;
                        break;
                }
                case 0x74:  // MOV M H
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->memory[hl] = state->h;
                        break;
                }
                case 0x75:  // MOV M L
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        state->memory[hl] = state->l;
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
                        state->memory[hl] = state->a;
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
                        state->a = state->memory[hl];
                        break;
                }
                case 0x7f:  // MOV A A
                {
                        state->a = state->a;
                        break;
                }
                case 0x80:  // ADD B
                {
                        Add(state, state->b);
                        break;
                }
                case 0x81:  // ADD C
                {
                        Add(state, state->c);
                        break;
                }
                case 0x82:  // ADD D
                {
                        Add(state, state->d);
                        break;
                }
                case 0x83:  // ADD E
                {
                        Add(state, state->e);
                        break;
                }
                case 0x84:  // ADD H
                {
                        Add(state, state->h);
                        break;
                }
                case 0x85:  // ADD L
                {
                        Add(state, state->l);
                        break;
                }
                case 0x86:  // ADD M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Add(state, state->memory[hl]);
                        break;
                }
                case 0x87:  // ADD A
                {
                        Add(state, state->a);
                        break;
                }
                case 0x88:  // ADC B
                {
                        Add(state, state->b + state->cc.cy);
                        break;
                }
                case 0x89:  // ADC C
                {
                        Add(state, state->c + state->cc.cy);
                        break;
                }
                case 0x8a:  // ADC D
                {
                        Add(state, state->d + state->cc.cy);
                        break;
                }
                case 0x8b:  // ADC E
                {
                        Add(state, state->e + state->cc.cy);
                        break;
                }
                case 0x8c:  // ADC H
                {
                        Add(state, state->h + state->cc.cy);
                        break;
                }
                case 0x8d:  // ADC L
                {
                        Add(state, state->l + state->cc.cy);
                        break;
                }
                case 0x8e:  // ADC M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Add(state, state->memory[hl] + state->cc.cy);
                        break;
                }
                case 0x8f:  // ADC A
                {
                        Add(state, state->a + state->cc.cy);
                        break;
                }
                case 0x90:  // SUB B
                {
                        Sub(state, state->b);
                        break;
                }
                case 0x91:  // SUB C
                {
                        Sub(state, state->c);
                        break;
                }
                case 0x92:  // SUB D
                {
                        Sub(state, state->d);
                        break;
                }
                case 0x93:  // SUB E
                {
                        Sub(state, state->e);
                        break;
                }
                case 0x94:  // SUB H
                {
                        Sub(state, state->h);
                        break;
                }
                case 0x95:  // SUB L
                {
                        Sub(state, state->l);
                        break;
                }
                case 0x96:  // SUB M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Sub(state, state->memory[hl]);
                        break;
                }
                case 0x97:  // SUB A
                {
                        Sub(state, state->a);
                        break;
                }
                case 0x98:  // SBB B
                {
                        Sub(state, state->b + state->cc.cy);
                        break;
                }
                case 0x99:  // SBB C
                {
                        Sub(state, state->c + state->cc.cy);
                        break;
                }
                case 0x9a:  // SBB D
                {
                        Sub(state, state->d + state->cc.cy);
                        break;
                }
                case 0x9b:  // SBB E
                {
                        Sub(state, state->e + state->cc.cy);
                        break;
                }
                case 0x9c:  // SBB H
                {
                        Sub(state, state->h + state->cc.cy);
                        break;
                }
                case 0x9d:  // SBB L
                {
                        Sub(state, state->l + state->cc.cy);
                        break;
                }
                case 0x9e:  // SBB M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Sub(state, state->memory[hl] + state->cc.cy);
                        break;
                }
                case 0x9f:  // SBB A
                {
                        Sub(state, state->a + state->cc.cy);
                        break;
                }
                case 0xa0:  // ANA B
                {
                        And(state, state->b);
                        break;
                }
                case 0xa1:  // ANA C
                {
                        And(state, state->c);
                        break;
                }
                case 0xa2:  // ANA D
                {
                        And(state, state->d);
                        break;
                }
                case 0xa3:  // ANA E
                {
                        And(state, state->e);
                        break;
                }
                case 0xa4:  // ANA H
                {
                        And(state, state->h);
                        break;
                }
                case 0xa5:  // ANA L
                {
                        And(state, state->l);
                        break;
                }
                case 0xa6:  // ANA M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        And(state, state->memory[hl]);
                        break;
                }
                case 0xa7:  // ANA A
                {
                        And(state, state->a);
                        break;
                }
                case 0xa8:  // XRA B
                {
                        Xra(state, state->b);
                        break;
                }
                case 0xa9:  // XRA C
                {
                        Xra(state, state->c);
                        break;
                }
                case 0xaa:  // XRA D
                {
                        Xra(state, state->d);
                        break;
                }
                case 0xab:  // XRA E
                {
                        Xra(state, state->e);
                        break;
                }
                case 0xac:  // XRA H
                {
                        Xra(state, state->h);
                        break;
                }
                case 0xad:  // XRA L
                {
                        Xra(state, state->l);
                        break;
                }
                case 0xae:  // XRA M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Xra(state, state->memory[hl]);
                        break;
                }
                case 0xaf:  // XRA A
                {
                        Xra(state, state->a);
                        break;
                }
                case 0xb0:  // ORA B
                {
                        Ora(state, state->b);
                        break;
                }
                case 0xb1:  // ORA C
                {
                        Ora(state, state->c);
                        break;
                }
                case 0xb2:  // ORA D
                {
                        Ora(state, state->d);
                        break;
                }
                case 0xb3:  // ORA E
                {
                        Ora(state, state->e);
                        break;
                }
                case 0xb4:  // ORA H
                {
                        Ora(state, state->h);
                        break;
                }
                case 0xb5:  // ORA L
                {
                        Ora(state, state->l);
                        break;
                }
                case 0xb6:  // ORA M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Ora(state, state->memory[hl]);
                        break;
                }
                case 0xb7:  // ORA A
                {
                        Ora(state, state->a);
                        break;
                }
                case 0xb8:  // CMP B
                {
                        Cmp(state, state->b);
                        break;
                }
                case 0xb9:  // CMP C
                {
                        Cmp(state, state->c);
                        break;
                }
                case 0xba:  // CMP D
                {
                        Cmp(state, state->d);
                        break;
                }
                case 0xbb:  // CMP E
                {
                        Cmp(state, state->e);
                        break;
                }
                case 0xbc:  // CMP H
                {
                        Cmp(state, state->h);
                        break;
                }
                case 0xbd:  // CMP L
                {
                        Cmp(state, state->l);
                        break;
                }
                case 0xbe:  // CMP M
                {
                        uint16_t hl = (state->h << 8) | state->l;
                        Cmp(state, state->memory[hl]);
                        break;
                }
                case 0xbf:  // CMP A
                {
                        Cmp(state, state->a);
                        break;
                }
                case 0xc0:  // RNZ
                {
                        if (!state->cc.z) {
                                state->pc =
                                    (state->memory[state->sp + 1] << 8) |
                                    state->memory[state->sp];
                                state->sp += 2;
                                return;
                        }
                        break;
                }
                case 0xc1:  // POP B
                {
                        state->c = state->memory[state->sp];
                        state->b = state->memory[state->sp + 1];
                        state->sp += 2;
                        break;
                }
                case 0xc2:  // JNZ addr
                {
                        if (!state->cc.z) {
                                state->pc = state->memory[state->pc + 2] << 8 |
                                            state->memory[state->pc + 1];
                                return;
                        }
                        state->pc += 2;
                        break;
                }
                case 0xc3:  // JMP addr
                {
                        state->pc = state->memory[state->pc + 2] << 8 |
                                    state->memory[state->pc + 1];
                        return;
                }
                case 0xc4: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xc5:  // PUSH B
                {
                        state->memory[state->sp - 1] = state->b;
                        state->memory[state->sp - 2] = state->c;
                        state->sp -= 2;
                        break;
                }
                case 0xc6:  // ADI d8
                {
                        Add(state, state->memory[state->pc + 1]);
                        state->pc++;
                        break;
                }
                case 0xc7: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xc8: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xc9:  // RET
                {
                        state->pc = (state->memory[state->sp + 1] << 8) |
                                    state->memory[state->sp];
                        state->sp += 2;
                        return;
                }
                case 0xca: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xcb: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xcc: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xcd:  // CALL addr
                {
                        uint16_t ret = state->pc + 2;
                        state->memory[state->sp - 1] = (ret >> 8) & 0xff;
                        state->memory[state->sp - 2] = (ret & 0xff);
                        state->sp -= 2;
                        state->pc = state->memory[state->pc + 2] << 8 |
                                    state->memory[state->pc + 1];
                        return;
                }
                case 0xce: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xcf: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd0: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd1: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd2: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd3:  // OUT d8
                {
                        // TODO: implement, for now just skip over data byte
                        state->pc++;
                        break;
                }
                case 0xd4: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd5: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd6: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd7: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd8: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xd9: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xda: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xdb: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xdc: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xdd: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xde: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xdf: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe0: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe1: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe2: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe3: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe4: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe5: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe6:  // ANI d8
                {
                        And(state, state->memory[state->pc + 1]);
                        state->pc++;
                        break;
                }
                case 0xe7: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe8: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xe9: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xea: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xeb: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xec: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xed: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xee: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xef: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xf0: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xf1:  // POP PSW
                {
                        state->a = state->memory[state->sp + 1];
                        uint8_t psw = state->memory[state->sp];
                        state->cc.z = 0x01 == (psw & 0x01);
                        state->cc.z = 0x02 == (psw & 0x02);
                        state->cc.z = 0x04 == (psw & 0x04);
                        state->cc.z = 0x08 == (psw & 0x08);
                        state->cc.z = 0x10 == (psw & 0x10);
                        state->sp += 2;
                        break;
                }
                case 0xf2: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xf3:  // DI
                {
                        state->int_enable = 0;
                        break;
                }
                case 0xf4: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xf5:  // PUSH PSW
                {
                        state->memory[state->sp - 1] = state->a;
                        uint8_t psw =
                            (state->cc.z | state->cc.s << 1 | state->cc.p << 2 |
                             state->cc.cy << 3 | state->cc.ac << 4);
                        state->memory[state->sp - 2] = psw;
                        state->sp -= 2;
                        break;
                }
                case 0xf6: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xf7: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xf8: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xf9: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xfa: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xfb:  // EI
                {
                        state->int_enable = 1;
                        break;
                }
                case 0xfc: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xfd: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                case 0xfe:  // CPI d8
                {
                        Cmp(state, state->memory[state->pc + 1]);
                        state->pc++;
                        break;
                }
                case 0xff: {
                        UnimplementedInstruction(opcode);
                        break;
                }
                default: {
                        UnimplementedInstruction(opcode);
                        break;
                }
        }

        state->pc++;
}

int main(int argc, char* argv[static argc + 1]) {
        if (argc < 2) {
                fprintf(stderr, "Please provide ROM file\n");
                return EXIT_FAILURE;
        }

        FILE* f = fopen(argv[1], "rb");
        if (!f) {
                fprintf(stderr, "Failed to open file in binary mode\n");
                return EXIT_FAILURE;
        }

        fseek(f, 0L, SEEK_END);
        int fsize = ftell(f);
        fseek(f, 0L, SEEK_SET);

        State state = {0};
        state.memory = malloc(fsize);
        if (!state.memory) {
                fprintf(stderr, "Failed to allocate memory buffer\n");
                return EXIT_FAILURE;
        }

        fread(state.memory, fsize, 1, f);
        fclose(f);

        while (1) {
                // printf("%04x %02x\n", state.pc, state.memory[state.pc]);
                EmulateOp(&state);
        }

        return EXIT_SUCCESS;
}