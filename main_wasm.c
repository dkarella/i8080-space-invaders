#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "invaders.h"

#define FILE_INVADERS "res/rom/invaders"

void update(void* userData) {
        invaders_update();
        return;
}

EM_BOOL render() {
        invaders_render();
        return 1;
}

int start(void) {
        FILE* f = fopen(FILE_INVADERS, "rb");
        if (!f) {
                fprintf(stderr, "Failed to open file in binary mode\n");
                return EXIT_FAILURE;
        }

        fseek(f, 0L, SEEK_END);
        int fsize = ftell(f);
        fseek(f, 0L, SEEK_SET);
        if (!fsize) {
                fprintf(stderr, "Provided file is empty\n");
                return EXIT_FAILURE;
        }

        atexit(invaders_quit);
        int err = invaders_init(f, fsize);
        fclose(f);
        if (err) {
                return EXIT_FAILURE;
        }

        emscripten_set_interval(update, 16, 0);
        emscripten_request_animation_frame_loop(render, 0);

        return EXIT_SUCCESS;
}
