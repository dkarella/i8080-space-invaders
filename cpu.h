#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdlib.h>

typedef struct cpu_conditionCodes {
        // NOTE: these are "bit fields"
        // the number after the colon describes how many bits that field uses
        uint8_t z : 1;    // set if result is 0
        uint8_t s : 1;    // set if MSB of result is set
        uint8_t p : 1;    // set if result has even parity
        uint8_t cy : 1;   // set if result had a carry or borrow
        uint8_t ac : 1;   // ignored (not used by space invaders)
        uint8_t pad : 3;  // this is just padding to make the struct 8 bits long
} cpu_conditionCodes;

typedef struct cpu {
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
        cpu_conditionCodes cc;
        uint8_t int_enable;
} cpu;

cpu* cpu_new(size_t memsize);
void cpu_delete(cpu* state);
uint8_t cpu_emulateOp(cpu* state);
void cpu_interrupt(cpu* state, uint8_t interrupt_num);
uint8_t cpu_read(cpu* state, uint16_t addr);

#endif
