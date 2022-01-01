#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "audio.h"
#include "cpu.h"
#include "disassembler.h"
#include "ports.h"

#define CPU_MEM 16384

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define FONT_FILE "res/fonts/arkitech/Arkitech Medium.ttf"
#define FONT_SIZE 12

static double playSpeed = 1.0;
static int paused = 0;

static cpu* state = 0;
static ports* pts = 0;

static SDL_Window* window = 0;
static SDL_Renderer* renderer = 0;

static SDL_Color const color_grey = {.r = 0xbb, .g = 0xbb, .b = 0xbb};
static SDL_Color const color_white = {.r = 0xff, .g = 0xff, .b = 0xff};
static SDL_Color const color_red = {.r = 0xff, .g = 0x00, .b = 0x00};
static SDL_Color const color_green = {.r = 0x00, .g = 0xff, .b = 0x00};
static SDL_Color const color_cyan = {.r = 0x00, .g = 0xff, .b = 0xff};

static SDL_Texture* text_atlas[128] = {0};

static clock_t lastTick;
static clock_t lastInterrupt;

static uint8_t interrupt = 1;

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
        SDL_SetRenderDrawColor(renderer, color_grey.r, color_grey.g,
                               color_grey.b, 255);
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

size_t get_screen_section(size_t row) {
        size_t section_size = 256 / 8;
        return (row / section_size);
}

SDL_Color get_section_color(size_t section) {
        SDL_Color colors[8] = {
            color_white, color_green, color_grey, color_grey,
            color_grey,  color_red,   color_cyan, color_white,
        };

        return colors[section % 8];
}

void render_screen(SDL_Renderer* renderer, cpu const* state) {
        SDL_Rect rect = {0};
        int scale = 2;
        int x_offset = (SCREEN_WIDTH / 4) + 20;
        int y_offset = 20;

        size_t i = 0;
        size_t prev_section = -1;
        for (size_t x = 31; x < 32; --x) {
                for (size_t b = 7; b < 8; --b) {
                        size_t section = get_screen_section(i / 224);
                        if (section != prev_section) {
                                SDL_Color color = get_section_color(section);
                                SDL_SetRenderDrawColor(renderer, color.r,
                                                       color.g, color.b, 255);
                                prev_section = section;
                        }

                        for (size_t y = 0; y < 224; ++y) {
                                uint8_t d =
                                    cpu_read(state, 32 * y + x + 0x2400);
                                if (d & (1 << b)) {
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

                surfaceMessage = TTF_RenderText_Solid(font, c, color_white);
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

int invaders_init(FILE* f, size_t fsize) {
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
        fread(state->memory, fsize, 1, f);

        int err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (err) {
                fprintf(stderr, "Failed init SDL: %s\n", SDL_GetError());
                return -1;
        }

        err = TTF_Init();
        if (err) {
                SDL_Quit();
                fprintf(stderr, "Failed to init SDL_ttf: %s\n", TTF_GetError());
                return -1;
        }

        window = SDL_CreateWindow("Space Invaders Emu", 100, 100, SCREEN_WIDTH,
                                  SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
                fprintf(stderr, "Failed to create Window: %s\n",
                        SDL_GetError());
                return -1;
                ;
        }

        renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
                fprintf(stderr, "Failed to create renderer: %s\n",
                        SDL_GetError());
                return -1;
        }

        pts = ports_new();

        err = text_atlas_init();
        if (err) {
                fprintf(stderr, "Failed to initialize text atlas\n");
                return -1;
        };

        audio_init();
        lastTick = clock();
        lastInterrupt = clock();

        return 0;
}

void invaders_render() {
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
}

int invaders_update() {
        SDL_Event e = {0};
        while (SDL_PollEvent(&e)) {
                switch (e.type) {
                        case SDL_QUIT: {
                                return 1;
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

        return 0;
}

void invaders_quit() {
        printf("Cleaning up...\n");
        ports_delete(pts);
        cpu_delete(state);
        text_atlas_destroy();
        audio_quit();
        if (renderer) {
                SDL_DestroyRenderer(renderer);
        }
        if (window) {
                SDL_DestroyWindow(window);
        }
        SDL_Quit();
        TTF_Quit();
}
