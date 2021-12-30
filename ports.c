#include "ports.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio.h"

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
                case 3:  // sounds
                {
                        static uint8_t prev_port_3 = 0;
                        static int ufo_channel = 0;

                        if ((value & 0x1) && !(prev_port_3 & 0x1)) {
                                ufo_channel = audio_loop(SOUND_UFO);
                        } else if (!(value & 0x1) && (prev_port_3 & 0x1)) {
                                audio_stop(ufo_channel);
                        }

                        if ((value & 0x2) && !(prev_port_3 & 0x2)) {
                                audio_play(SOUND_SHOT);
                        }
                        if ((value & 0x4) && !(prev_port_3 & 0x4)) {
                                audio_play(SOUND_PLAYER_DIE);
                        }
                        if ((value & 0x8) && !(prev_port_3 & 0x8)) {
                                audio_play(SOUND_INVADER_DIE);
                        }

                        prev_port_3 = value;
                        break;
                }
                case 4:  // shift
                {
                        pts->shift = (value << 8) | (pts->shift >> 8);
                        break;
                }
                case 5:  // more sounds
                {
                        static uint8_t prev_port_5 = 0;

                        if ((value & 0x1) && !(prev_port_5 & 0x1)) {
                                audio_play(SOUND_FLEET_MOVEMENT_1);
                        }
                        if ((value & 0x2) && !(prev_port_5 & 0x2)) {
                                audio_play(SOUND_FLEET_MOVEMENT_2);
                        }
                        if ((value & 0x4) && !(prev_port_5 & 0x4)) {
                                audio_play(SOUND_FLEET_MOVEMENT_3);
                        }
                        if ((value & 0x8) && !(prev_port_5 & 0x8)) {
                                audio_play(SOUND_FLEET_MOVEMENT_4);
                        }
                        if ((value & 0x10) && !(prev_port_5 & 0x10)) {
                                audio_play(SOUND_UFO_DIE);
                        }

                        prev_port_5 = value;
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
