#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "disassembler.h"
#include "ports.h"

#define CPU_MEM 16384

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define FONT_FILE "fonts/arkitech/Arkitech Medium.ttf"

int quit = 0;
int running = 0;

cpu* state = 0;
ports* pts = 0;

SDL_Window* window = 0;
SDL_Renderer* renderer = 0;

SDL_Color const color_debugInfo = {.r = 255, .g = 255, .b = 255};
SDL_Color const color_debugInfoCurrent = {.r = 125, .g = 255, .b = 125};
SDL_Color const color_grid = {.r = 128, .g = 128, .b = 128};

void cleanup() {
        ports_delete(pts);
        cpu_delete(state);
        if (renderer) {
                SDL_DestroyRenderer(renderer);
        }
        if (window) {
                SDL_DestroyWindow(window);
        }
        SDL_Quit();
        TTF_Quit();
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
}

void render_text(SDL_Renderer* renderer, char const* s, int ptsize, SDL_Color c,
                 int x, int y) {
        TTF_Font* font = 0;
        SDL_Surface* surfaceMessage = 0;
        SDL_Texture* message = 0;

        // TODO: should probably use a font atlas instead
        font = TTF_OpenFont(FONT_FILE, ptsize);
        if (!font) {
                goto TTF_ERROR;
        }

        surfaceMessage = TTF_RenderText_Solid(font, s, c);
        if (!surfaceMessage) {
                goto TTF_ERROR;
        }

        message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
        if (!message) {
                goto SDL_ERROR;
        }

        SDL_Rect rect = {.x = x, .y = y};
        int err = SDL_QueryTexture(message, 0, 0, &rect.w, &rect.h);
        if (err) {
                goto SDL_ERROR;
        }

        err = SDL_RenderCopy(renderer, message, 0, &rect);
        if (err) {
                goto SDL_ERROR;
        }

DONE:
        SDL_FreeSurface(surfaceMessage);
        SDL_DestroyTexture(message);
        TTF_CloseFont(font);
        if (err) {
                exit(err);
        }
        return;
TTF_ERROR:
        fprintf(stderr, "render_text: TTF_error: %s\n", SDL_GetError());
        err = EXIT_FAILURE;
        goto DONE;
SDL_ERROR:
        fprintf(stderr, "render_text: SDL_Error: %s\n", SDL_GetError());
        err = EXIT_FAILURE;
        goto DONE;
}

void render_debugInfo(SDL_Renderer* renderer, cpu const* state,
                      ports const* pts) {
        char buf[128] = "";
        char buf2[128] = "";
        int x = 0;
        int y = 0;
        int ptsize = 12;
        int err = 0;

        SDL_SetRenderDrawColor(renderer, color_grid.r, color_grid.g,
                               color_grid.b, 255);
        x = SCREEN_WIDTH / 4;
        SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_HEIGHT);

        y = 20;
        x = 20;

        sprintf(buf, "PC: $%04x", state->pc);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "SP: $%02x", state->sp);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        err = dissassembleOp(state->pc, state->memory, buf2);
        if (err) {
                fprintf(stderr, "render_debugInfo: dissassembleOp error\n");
                exit(EXIT_FAILURE);
        }
        sprintf(buf, "Op: %s", buf2);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        y += 20;

        render_text(renderer, "Registers:", ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "A: $%02x", state->a);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "B: $%02x", state->b);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "C: $%02x", state->c);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "D: $%02x", state->d);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "E: $%02x", state->e);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "H: $%02x", state->h);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "L: $%02x", state->l);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        y += 20;

        render_text(renderer, "Condition Codes:", ptsize, color_debugInfo, x,
                    y);
        y += 20;

        sprintf(buf, "Z: %d", state->cc.z);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "S: %d", state->cc.s);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "P: %d", state->cc.p);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "CY: %d", state->cc.cy);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "AC: %d", state->cc.ac);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        y += 20;

        sprintf(buf, "Interrupts: %d", state->int_enable);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        y += 20;

        render_text(renderer, "Ports:", ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "Shift: %04x", pts->shift);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "Inp1: %04x", pts->inp1.value);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;

        sprintf(buf, "Inp2: %04x", pts->inp2.value);
        render_text(renderer, buf, ptsize, color_debugInfo, x, y);
        y += 20;
}

void tick(cpu* state, ports* pts) {
        uint8_t opcode = state->memory[state->pc];
        switch (opcode) {
                case 0xdb:  // IN
                {
                        uint8_t port = state->memory[state->pc + 1];
                        state->a = ports_in(pts, port);
                        state->pc++;
                        break;
                }
                case 0xd3:  // OUT
                {
                        uint8_t port = state->memory[state->pc + 1];
                        ports_out(pts, port, state->a);
                        state->pc++;
                        break;
                }
                default: {
                        cpu_emulateOp(state);
                        break;
                }
        }
}

void keydown(SDL_KeyboardEvent key, cpu* state, ports* pts) {
        switch (key.keysym.sym) {
                case SDLK_0: {
                        if (!running) {
                                tick(state, pts);
                        }
                        break;
                }
                case SDLK_9: {
                        running = !running;
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
                case SDLK_SPACE: {
                        pts->inp1.bits.p1_shot = 1;
                        break;
                }
                case SDLK_LEFT: {
                        pts->inp1.bits.p1_left = 1;
                        break;
                }
                case SDLK_RIGHT: {
                        pts->inp1.bits.p1_right = 1;
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
                case SDLK_p: {
                        pts->inp2.bits.p2_shot = 1;
                        break;
                }
                case SDLK_a: {
                        pts->inp2.bits.p2_left = 1;
                        break;
                }
                case SDLK_d: {
                        pts->inp2.bits.p2_right = 1;
                        break;
                }
                case SDLK_7: {
                        pts->inp2.bits.dip7 = 1;
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
                case SDLK_SPACE: {
                        pts->inp1.bits.p1_shot = 0;
                        break;
                }
                case SDLK_LEFT: {
                        pts->inp1.bits.p1_left = 0;
                        break;
                }
                case SDLK_RIGHT: {
                        pts->inp1.bits.p1_right = 0;
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
                case SDLK_p: {
                        pts->inp2.bits.p2_shot = 0;
                        break;
                }
                case SDLK_a: {
                        pts->inp2.bits.p2_left = 0;
                        break;
                }
                case SDLK_d: {
                        pts->inp2.bits.p2_right = 0;
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

        state = cpu_new(CPU_MEM);
        if (!state) {
                fprintf(stderr, "Failed initialize cpu state\n");
                return EXIT_FAILURE;
        }
        fread(state->memory, fsize, 1, f);
        fclose(f);

        pts = ports_new();

        init();
        atexit(cleanup);

        window = SDL_CreateWindow("Space Invaders Emu", 100, 100, SCREEN_WIDTH,
                                  SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
                fprintf(stderr, "Failed to create Window: %s\n",
                        SDL_GetError());
                return EXIT_FAILURE;
        }

        renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
                fprintf(stderr, "Failed to create renderer: %s\n",
                        SDL_GetError());
                return EXIT_FAILURE;
        }

        SDL_Event e = {0};
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

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                render_debugInfo(renderer, state, pts);
                SDL_RenderPresent(renderer);

                if (running) {
                        tick(state, pts);
                }
        }

        return EXIT_SUCCESS;
}