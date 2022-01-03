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

#define SCREEN_WIDTH 224
#define SCREEN_HEIGHT 256
#define SCREEN_SCALE 2
#define SCREEN_PADDING 40

#define FONT_FILE "res/fonts/arkitech/Arkitech Medium.ttf"
#define FONT_SIZE 12

static int const window_width =
    SCREEN_WIDTH * SCREEN_SCALE + SCREEN_PADDING * 2;
static int const window_height =
    SCREEN_HEIGHT * SCREEN_SCALE + SCREEN_PADDING * 2;

static int paused;

static cpu* state;
static ports* pts;

static SDL_Window* window;
static SDL_Renderer* renderer;

static SDL_Color const color_grey = {.r = 0xbb, .g = 0xbb, .b = 0xbb};
static SDL_Color const color_white = {.r = 0xff, .g = 0xff, .b = 0xff};
static SDL_Color const color_red = {.r = 0xff, .g = 0x00, .b = 0x00};
static SDL_Color const color_green = {.r = 0x00, .g = 0xff, .b = 0x00};
static SDL_Color const color_cyan = {.r = 0x00, .g = 0xff, .b = 0xff};

static SDL_Texture* text_atlas[128];

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

size_t get_screen_section(size_t row) {
        size_t section_size = SCREEN_HEIGHT / 8;
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
        int x_offset = SCREEN_PADDING;
        int y_offset = SCREEN_PADDING;

        size_t i = 0;
        size_t prev_section = -1;
        for (size_t x = 31; x < 32; --x) {
                for (size_t b = 7; b < 8; --b) {
                        size_t section = get_screen_section(i / SCREEN_WIDTH);
                        if (section != prev_section) {
                                SDL_Color color = get_section_color(section);
                                SDL_SetRenderDrawColor(renderer, color.r,
                                                       color.g, color.b, 255);
                                prev_section = section;
                        }

                        for (size_t y = 0; y < SCREEN_WIDTH; ++y) {
                                uint8_t d =
                                    cpu_read(state, 32 * y + x + 0x2400);
                                if (d & (1 << b)) {
                                        rect.x =
                                            (i % SCREEN_WIDTH) * SCREEN_SCALE +
                                            x_offset;
                                        rect.y =
                                            (i / SCREEN_WIDTH) * SCREEN_SCALE +
                                            y_offset;
                                        rect.w = SCREEN_SCALE;
                                        rect.h = SCREEN_SCALE;
                                        SDL_RenderDrawRect(renderer, &rect);
                                }
                                ++i;
                        }
                }
        }
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
        float elapsed = ((float)(clock() - lastTick)) / CLOCKS_PER_SEC;
        return elapsed * 2000000;
}

size_t shouldInterrupt(cpu* state, clock_t lastInterrupt) {
        float elapsed_ms =
            (((float)(clock() - lastInterrupt)) / CLOCKS_PER_SEC) * 1000;
        return state->int_enable && elapsed_ms >= 14;
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

        window = SDL_CreateWindow("Space Invaders Emu", 100, 100, window_width,
                                  window_height, SDL_WINDOW_SHOWN);
        if (!window) {
                fprintf(stderr, "Failed to create Window: %s\n",
                        SDL_GetError());
                return -1;
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
        render_screen(renderer, state);
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

        if (shouldInterrupt(state, lastInterrupt)) {
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
        if (window) {
                SDL_DestroyWindow(window);
        }
        if (renderer) {
                SDL_DestroyRenderer(renderer);
        }
        SDL_Quit();
        TTF_Quit();
}
