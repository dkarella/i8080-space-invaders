#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "invaders.h"

size_t shouldRender(clock_t lastRender) {
        double elapsed = ((double)(clock() - lastRender)) / CLOCKS_PER_SEC;
        return elapsed > (1.0 / 60.0);
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

        atexit(invaders_quit);
        int err = invaders_init(f, fsize);
        fclose(f);
        if (err) {
                return EXIT_FAILURE;
        }

        clock_t lastRender = clock();
        for (;;) {
                int quit = invaders_update();
                if (quit) {
                        break;
                }

                if (shouldRender(lastRender)) {
                        invaders_render();
                }
        }

        return EXIT_SUCCESS;
}