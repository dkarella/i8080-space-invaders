#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

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

        cpu* state = cpu_new(fsize);
        if (!state) {
                fprintf(stderr, "Failed initialize cpu state\n");
                return EXIT_FAILURE;
        }
        fread(state->memory, fsize, 1, f);
        fclose(f);

        while (1) {
                printf("%04x %02x\n", state->pc, state->memory[state->pc]);
                cpu_emulateOp(state);
        }

        return EXIT_SUCCESS;
}