#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>

int disassembleOp(uint8_t pc, uint8_t* memory, char* s);

#endif
