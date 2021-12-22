#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

typedef struct {
        uint8_t credit : 1;
        uint8_t p2_start : 1;
        uint8_t p1_start : 1;
        uint8_t _ : 1;
        uint8_t p1_shot : 1;
        uint8_t p1_left : 1;
        uint8_t p1_right : 1;
        uint8_t __ : 1;
} ports_inp1_s;

typedef union {
        ports_inp1_s bits;
        uint8_t value;
} ports_inp1;

typedef struct {
        uint8_t dip3 : 1;
        uint8_t dip5 : 1;
        uint8_t tilt : 1;
        uint8_t dip6 : 1;
        uint8_t p2_shot : 1;
        uint8_t p2_left : 1;
        uint8_t p2_right : 1;
        uint8_t dip7 : 1;
} ports_inp2_s;

typedef union {
        ports_inp2_s bits;
        uint8_t value;
} ports_inp2;

typedef struct {
        ports_inp1 inp1;
        ports_inp2 inp2;
        uint16_t shift;
        uint8_t shift_offset;
} ports;

ports* ports_new();
void ports_delete(ports* m);

uint8_t ports_in(ports* pts, uint8_t port);
void ports_out(ports* pts, uint8_t port, uint8_t value);

#endif
