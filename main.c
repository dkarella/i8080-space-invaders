#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cpu.h"
#include "disassembler.h"
#include "ports.h"

#define CPU_MEM 16384

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define FONT_FILE "fonts/arkitech/Arkitech Medium.ttf"
#define FONT_SIZE 12

int quit = 0;
int paused = 0;
double playSpeed = 1.0;

cpu* state = 0;
ports* pts = 0;

SDL_Window* window = 0;
SDL_Renderer* renderer = 0;

SDL_Color const color_grid = {.r = 128, .g = 128, .b = 128};
SDL_Color const color_fill = {.r = 255, .g = 255, .b = 255};

SDL_Texture* text_atlas[128] = {0};

int text_atlas_init() {
        TTF_Font* font = 0;
        SDL_Surface* surfaceMessage = 0;
        SDL_Texture* texture = 0;
        int err = 0;

        font = TTF_OpenFont(FONT_FILE, FONT_SIZE);
        if (!font) {
                goto TTF_ERROR;
        }

        for (size_t i = 1; i < 128; ++i) {
                char const c[] = {i, 0};

                surfaceMessage = TTF_RenderText_Solid(font, c, color_fill);
                if (!surfaceMessage) {
                        goto TTF_ERROR;
                }

                texture =
                    SDL_CreateTextureFromSurface(renderer, surfaceMessage);
                if (!texture) {
                        SDL_FreeSurface(surfaceMessage);
                        goto SDL_ERROR;
                }

                text_atlas[i] = texture;
                SDL_FreeSurface(surfaceMessage);
        }

DONE:
        TTF_CloseFont(font);
        return err;
TTF_ERROR:
        fprintf(stderr, "text_atlas_init: TTF_error: %s\n", SDL_GetError());
        err = -1;
        goto DONE;
SDL_ERROR:
        fprintf(stderr, "text_atlas_init: SDL_Error: %s\n", SDL_GetError());
        err = -1;
        goto DONE;
}

void text_atlas_destroy() {
        for (size_t i = 0; i < 128; ++i) {
                SDL_DestroyTexture(text_atlas[i]);
        }
}

void init() {
        int err = SDL_Init(SDL_INIT_VIDEO);
        if (err) {
                fprintf(stderr, "Failed init SDL: %s\n", SDL_GetError());
                exit(EXIT_FAILURE);
        }

        err = TTF_Init();
        if (err) {
                SDL_Quit();
                fprintf(stderr, "Failed to init SDL_ttf: %s\n", TTF_GetError());
                exit(EXIT_FAILURE);
        }

        window = SDL_CreateWindow("Space Invaders Emu", 100, 100, SCREEN_WIDTH,
                                  SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
                fprintf(stderr, "Failed to create Window: %s\n",
                        SDL_GetError());
                exit(EXIT_FAILURE);
        }

        renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
                fprintf(stderr, "Failed to create renderer: %s\n",
                        SDL_GetError());
                exit(EXIT_FAILURE);
        }

        err = text_atlas_init();
        if (err) {
                fprintf(stderr, "Failed to initialize text atlas\n");
                exit(EXIT_FAILURE);
        };
}

void cleanup() {
        printf("Cleaning up...\n");
        ports_delete(pts);
        cpu_delete(state);
        text_atlas_destroy();
        if (renderer) {
                SDL_DestroyRenderer(renderer);
        }
        if (window) {
                SDL_DestroyWindow(window);
        }
        SDL_Quit();
        TTF_Quit();
}

void render_text(SDL_Renderer* renderer, char* s, int x, int y) {
        if (!s || !renderer) {
                return;
        }

        SDL_Rect r = {.x = x, .y = y};
        for (size_t i = 0; s[i] != 0; ++i) {
                if (s[i] == 0 || i > 127) {
                        fprintf(stderr,
                                "render_text: non-ASCII character: %zu\n", i);
                        continue;
                }

                SDL_Texture* t = text_atlas[(size_t)s[i]];

                int err = SDL_QueryTexture(t, 0, 0, &r.w, &r.h);
                if (err) {
                        fprintf(stderr, "render_text: SDL_Error: %s\n",
                                SDL_GetError());
                        continue;
                }

                err = SDL_RenderCopy(renderer, t, 0, &r);
                if (err) {
                        fprintf(stderr, "render_text: SDL_Error: %s\n",
                                SDL_GetError());
                        continue;
                }

                r.x += r.w;
        }
}

void render_grid(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, color_grid.r, color_grid.g,
                               color_grid.b, 255);
        int x = SCREEN_WIDTH / 4;
        SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_HEIGHT);
}

void render_info(SDL_Renderer* renderer) {
        char buf[128] = "";
        int x = 20;
        int y = 20;

        render_text(renderer, "Space Invaders Emulator", x, y);
        y += 20;

        render_text(renderer, "github.com/dkarella", x, y);
        y += 40;

        sprintf(buf, "Speed: %.2f", playSpeed);
        render_text(renderer, buf, x, y);
}

void render_debugInfo(SDL_Renderer* renderer, cpu const* state,
                      ports const* pts) {
        char buf[128] = "";
        char buf2[128] = "";
        int x = 20;
        int y = 20;

        sprintf(buf, "PC: $%04x", state->pc);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "SP: $%02x", state->sp);
        render_text(renderer, buf, x, y);
        y += 20;

        if (disassembleOp(state->pc, state->memory, buf2) == -1) {
                fprintf(stderr, "render_debugInfo: disassembleOp error\n");
                exit(EXIT_FAILURE);
        }
        sprintf(buf, "Op: %s", buf2);
        render_text(renderer, buf, x, y);
        y += 20;

        y += 20;

        render_text(renderer, "Registers:", x, y);
        y += 20;

        sprintf(buf, "A: $%02x", state->a);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "B: $%02x", state->b);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "C: $%02x", state->c);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "D: $%02x", state->d);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "E: $%02x", state->e);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "H: $%02x", state->h);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "L: $%02x", state->l);
        render_text(renderer, buf, x, y);
        y += 20;

        y += 20;

        render_text(renderer, "Condition Codes:", x, y);
        y += 20;

        sprintf(buf, "Z: %d", state->cc.z);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "S: %d", state->cc.s);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "P: %d", state->cc.p);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "CY: %d", state->cc.cy);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "AC: %d", state->cc.ac);
        render_text(renderer, buf, x, y);
        y += 20;

        y += 20;

        sprintf(buf, "Interrupts: %d", state->int_enable);
        render_text(renderer, buf, x, y);
        y += 20;

        y += 20;

        render_text(renderer, "Ports:", x, y);
        y += 20;

        sprintf(buf, "Shift: %04x", pts->shift);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "Inp1: %04x", pts->inp1.value);
        render_text(renderer, buf, x, y);
        y += 20;

        sprintf(buf, "Inp2: %04x", pts->inp2.value);
        render_text(renderer, buf, x, y);
        y += 20;
}

void render_screen(SDL_Renderer* renderer, cpu const* state) {
        SDL_Rect rect = {0};
        int scale = 2;
        int x_offset = (SCREEN_WIDTH / 4) + 20;
        int y_offset = 20;

        SDL_SetRenderDrawColor(renderer, color_fill.r, color_fill.g,
                               color_fill.b, 255);

        size_t i = 0;
        for (size_t x = 31; x < 32; --x) {
                for (size_t b = 7; b < 8; --b) {
                        for (size_t y = 0; y < 224; ++y) {
                                uint8_t d =
                                    cpu_read(state, 32 * y + x + 0x2400);
                                if ((d >> b) & 0x1) {
                                        rect.x = (i % 224) * scale + x_offset;
                                        rect.y = (i / 224) * scale + y_offset;
                                        rect.w = scale;
                                        rect.h = scale;
                                        SDL_RenderDrawRect(renderer, &rect);
                                }
                                ++i;
                        }
                }
        }
}

void dumpMEM(cpu* state, char const* file) {
        FILE* f = fopen(file, "wb");
        if (!f) {
                fprintf(stderr, "Failed to open file in binary mode\n");
                exit(1);
        }

        for (uint16_t i = 0x0000; i < 0x4000; ++i) {
                fprintf(f, "0x%04x 0x%02x\n", i, state->memory[i]);
        }
        fclose(f);
}

uint8_t tick(cpu* state, ports* pts) {
        uint8_t opcode = cpu_read(state, state->pc);
        switch (opcode) {
                case 0xdb:  // IN
                {
                        uint8_t port = cpu_read(state, state->pc + 1);
                        state->a = ports_in(pts, port);
                        state->pc += 2;
                        return 10;
                }
                case 0xd3:  // OUT
                {
                        uint8_t port = cpu_read(state, state->pc + 1);
                        ports_out(pts, port, state->a);
                        state->pc += 2;
                        return 10;
                }
                default: {
                }
        }

        size_t cycles = cpu_emulateOp(state);
        return cycles;
}

void keydown(SDL_KeyboardEvent key, cpu* state, ports* pts) {
        switch (key.keysym.sym) {
                case SDLK_0: {
                        if (!paused) {
                                paused = 1;
                        } else {
                                tick(state, pts);
                        }
                        break;
                }
                case SDLK_9: {
                        paused = !paused;
                        break;
                }
                case SDLK_RETURN: {
                        pts->inp1.bits.credit = 1;
                        break;
                }
                case SDLK_2: {
                        pts->inp1.bits.p2_start = 1;
                        break;
                }
                case SDLK_1: {
                        pts->inp1.bits.p1_start = 1;
                        break;
                }
                case SDLK_p:
                case SDLK_SPACE: {
                        pts->inp1.bits.p1_shot = 1;
                        pts->inp2.bits.p2_shot = 1;
                        break;
                }
                case SDLK_a:
                case SDLK_LEFT: {
                        pts->inp1.bits.p1_left = 1;
                        pts->inp2.bits.p2_left = 1;
                        break;
                }
                case SDLK_d:
                case SDLK_RIGHT: {
                        pts->inp1.bits.p1_right = 1;
                        pts->inp2.bits.p2_right = 1;
                        break;
                }
                case SDLK_3: {
                        pts->inp2.bits.dip3 = 1;
                        break;
                }
                case SDLK_5: {
                        pts->inp2.bits.dip5 = 1;
                        break;
                }
                case SDLK_TAB: {
                        pts->inp2.bits.tilt = 1;
                        break;
                }
                case SDLK_6: {
                        pts->inp2.bits.dip6 = 1;
                        break;
                }
                case SDLK_7: {
                        pts->inp2.bits.dip7 = 1;
                        break;
                }
                case SDLK_r: {
                        dumpMEM(state, "memdump.txt");
                        break;
                }
                case SDLK_PAGEUP: {
                        playSpeed += .25;
                        if (playSpeed > 2.0) {
                                playSpeed = 2.0;
                        }
                        break;
                }
                case SDLK_PAGEDOWN: {
                        playSpeed -= .25;
                        if (playSpeed < .25) {
                                playSpeed = .25;
                        }
                        break;
                }
                default: {
                        break;
                }
        }
}

void keyup(SDL_KeyboardEvent key, ports* pts) {
        switch (key.keysym.sym) {
                case SDLK_RETURN: {
                        pts->inp1.bits.credit = 0;
                        break;
                }
                case SDLK_2: {
                        pts->inp1.bits.p2_start = 0;
                        break;
                }
                case SDLK_1: {
                        pts->inp1.bits.p1_start = 0;
                        break;
                }
                case SDLK_p:
                case SDLK_SPACE: {
                        pts->inp1.bits.p1_shot = 0;
                        pts->inp2.bits.p2_shot = 0;
                        break;
                }
                case SDLK_a:
                case SDLK_LEFT: {
                        pts->inp1.bits.p1_left = 0;
                        pts->inp2.bits.p2_left = 0;
                        break;
                }
                case SDLK_d:
                case SDLK_RIGHT: {
                        pts->inp1.bits.p1_right = 0;
                        pts->inp2.bits.p2_right = 0;
                        break;
                }
                case SDLK_3: {
                        pts->inp2.bits.dip3 = 0;
                        break;
                }
                case SDLK_5: {
                        pts->inp2.bits.dip5 = 0;
                        break;
                }
                case SDLK_TAB: {
                        pts->inp2.bits.tilt = 0;
                        break;
                }
                case SDLK_6: {
                        pts->inp2.bits.dip6 = 0;
                        break;
                }
                case SDLK_7: {
                        pts->inp2.bits.dip7 = 0;
                        break;
                }
                default: {
                        break;
                }
        }
}

size_t ncycles(clock_t lastTick) {
        double elapsed = ((double)(clock() - lastTick)) / CLOCKS_PER_SEC;
        return elapsed * 2000000 * playSpeed;
}

size_t shouldInterrupt(clock_t lastInterrupt) {
        double elapsed = ((double)(clock() - lastInterrupt)) / CLOCKS_PER_SEC;
        return state->int_enable && elapsed > (1.0 / (120.0 * playSpeed));
}

size_t shouldRender(clock_t lastRender) {
        double elapsed = ((double)(clock() - lastRender)) / CLOCKS_PER_SEC;
        return elapsed > (1.0 / (60.0 * playSpeed));
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
        if (fsize > CPU_MEM) {
                fprintf(stderr,
                        "Provided file too big, wrong file or corrupted?\n");
                return EXIT_FAILURE;
        }

        state = cpu_new(CPU_MEM);
        if (!state) {
                fprintf(stderr, "Failed to initialize cpu state\n");
                return EXIT_FAILURE;
        }
#ifdef CPUDIAG
        fread(state->memory + 0x100, fsize, 1, f);
#else
        fread(state->memory, fsize, 1, f);
#endif

        fclose(f);

#ifdef CPUDIAG
        // Fix the first instruction to be JMP 0x100
        state->memory[0] = 0xc3;
        state->memory[1] = 0x00;
        state->memory[2] = 0x01;

        // Fix the stack pointer from 0x6ad to 0x7ad
        // this 0x06 byte 112 in the code, which is
        // byte 112 + 0x100 = 368 in memory
        state->memory[368] = 0x7;

        // Skip DAA test
        state->memory[0x59c] = 0xc3;  // JMP
        state->memory[0x59d] = 0xc2;
        state->memory[0x59e] = 0x05;

        FILE* asmf = fopen("cpudag.asm", "wb");
        for (size_t pc = 0; pc < fsize + 0x100; ++pc) {
                char s[265] = "";
                int n = disassembleOp(pc, state->memory, s);
                if (n == -1) {
                        printf("failed to disassemble\n");
                        return EXIT_FAILURE;
                }
                fprintf(asmf, "%04zx %s\n", pc, s);
                pc += n;
        }
        fclose(asmf);
#endif

        pts = ports_new();

        init();
        atexit(cleanup);

        SDL_Event e = {0};
        clock_t lastTick = clock();
        clock_t lastInterrupt = clock();
        clock_t lastRender = clock();
        uint8_t interrupt = 1;
        while (!quit) {
                while (SDL_PollEvent(&e)) {
                        switch (e.type) {
                                case SDL_QUIT: {
                                        quit = 1;
                                        break;
                                }
                                case SDL_KEYDOWN: {
                                        keydown(e.key, state, pts);
                                        break;
                                }
                                case SDL_KEYUP: {
                                        keyup(e.key, pts);
                                        break;
                                }
                                default: {
                                        break;
                                }
                        }
                }
                if (shouldInterrupt(lastInterrupt)) {
                        cpu_interrupt(state, interrupt);
                        interrupt = interrupt == 1 ? 2 : 1;
                        lastInterrupt = clock();
                }
                if (!paused) {
                        size_t n = ncycles(lastTick);
                        while (n) {
                                size_t cycles = tick(state, pts);
                                if (cycles > n) {
                                        break;
                                }
                                n -= cycles;
                        }
                        lastTick = clock();
                } else {
                        // prevent fast-forwarding
                        lastTick = clock();
                }

                if (shouldRender(lastRender)) {
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        SDL_RenderClear(renderer);

                        render_grid(renderer);
                        render_screen(renderer, state);
                        if (paused) {
                                render_debugInfo(renderer, state, pts);
                        } else {
                                render_info(renderer);
                        }

                        SDL_RenderPresent(renderer);
                        lastRender = clock();
                }
        }

        return EXIT_SUCCESS;
}
