#include "ports.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ports* ports_new() {
        ports* pts = malloc(sizeof(ports));
        if (pts) {
                *pts = (ports){0};
        }
        return pts;
}

void ports_delete(ports* pts) { free(pts); }

uint8_t ports_in(ports* pts, uint8_t port) {
        uint8_t a = 0;
        switch (port) {
                case 1:  // inp1
                {
                        a = pts->inp1.value;
                        break;
                }
                case 2:  // inp2
                {
                        a = pts->inp2.value;
                        break;
                }
                case 3:  // shift result
                {
                        a = (pts->shift >> (8 - pts->shift_offset)) & 0xff;
                        break;
                }
                default: {
                        fprintf(stderr, "ports_in: unimplemented port: %d\n",
                                port);
                        exit(EXIT_FAILURE);
                }
        }

        return a;
}

void ports_out(ports* pts, uint8_t port, uint8_t value) {
        switch (port) {
                case 2:  // shift_offset
                {
                        pts->shift_offset = value & 0x7;
                        break;
                }
                case 3:  // sound related
                {
                        // TODO
                        break;
                }
                case 4:  // shift
                {
                        pts->shift = (value << 8) | (pts->shift >> 8);
                        break;
                }
                case 5:  // sound related
                {
                        // TODO
                        break;
                }
                case 6:  // coin info displayed in demo screen
                {
                        // TODO
                        break;
                }
                default: {
                        fprintf(stderr, "ports_out: unimplemented port: %d\n",
                                port);
                        exit(EXIT_FAILURE);
                }
        }
}
