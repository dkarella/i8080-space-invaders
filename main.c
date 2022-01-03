#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "invaders.h"

size_t shouldRender(clock_t lastRender) {
        float elapsed = ((float)(clock() - lastRender)) / CLOCKS_PER_SEC;
        return elapsed > (1.0f / 60.0f);
}

size_t shouldUpdate(clock_t lastUpdate) {
        float elapsed = ((float)(clock() - lastUpdate)) / CLOCKS_PER_SEC;
        return elapsed > (1.0f / 120.0f);
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
        clock_t lastUpdate = clock();
        for (;;) {
                if (shouldUpdate(lastUpdate)) {
                        if (invaders_update()) {
                                break;
                        }
                        lastUpdate = clock();
                }

                if (shouldRender(lastRender)) {
                        invaders_render();
                        lastRender = clock();
                }
        }

        return EXIT_SUCCESS;
}