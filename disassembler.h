#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>

int disassembleOp(uint16_t pc, uint8_t const* memory, char* s);

#endif
