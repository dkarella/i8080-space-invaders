#include "audio.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdint.h>
#include <stdio.h>

#define DIR_PATH "./res/sounds"
#define FREQ 44100
#define STEREO 2
#define CHUNK_SIZE 2048

static char const* const fileNames[NUM_SOUNDS] = {
    [SOUND_UFO] = "0.wav",
    [SOUND_SHOT] = "1.wav",
    [SOUND_PLAYER_DIE] = "2.wav",
    [SOUND_INVADER_DIE] = "3.wav",
    [SOUND_FLEET_MOVEMENT_1] = "4.wav",
    [SOUND_FLEET_MOVEMENT_2] = "5.wav",
    [SOUND_FLEET_MOVEMENT_3] = "6.wav",
    [SOUND_FLEET_MOVEMENT_4] = "7.wav",
    [SOUND_UFO_DIE] = "8.wav",
};

static Mix_Chunk* sounds[NUM_SOUNDS];

int audio_init() {
        int err = Mix_OpenAudio(FREQ, MIX_DEFAULT_FORMAT, STEREO, CHUNK_SIZE);
        if (err) {
                fprintf(stderr, "Failed to initialize SDL_mixer: %s\n",
                        Mix_GetError());
                return err;
        }

        for (size_t sound = 0; sound < NUM_SOUNDS; ++sound) {
                char file[256] = "";
                sprintf(file, "%s/%s", DIR_PATH, fileNames[sound]);

                sounds[sound] = Mix_LoadWAV(file);
                if (!sounds[sound]) {
                        fprintf(stderr, "Failed to load sound: %s\n",
                                Mix_GetError());
                        continue;
                }
        }

        return 0;
}

void audio_quit() {
        for (size_t i = 0; i < NUM_SOUNDS; ++i) {
                if (!sounds[i]) {
                        continue;
                }
                Mix_FreeChunk(sounds[i]);
                sounds[i] = 0;
        }

        Mix_Quit();
}

void audio_play(audio_sound sound) {
        Mix_Chunk* chunk = sounds[sound];
        if (!chunk) {
                return;
        }

        Mix_PlayChannelTimed(-1, chunk, 0, -1);
}

int audio_loop(audio_sound sound) {
        Mix_Chunk* chunk = sounds[sound];
        if (!chunk) {
                return -1;
        }

        return Mix_PlayChannelTimed(-1, chunk, -1, -1);
}

void audio_stop(int channel) {
        if (channel >= 0) {
                Mix_FadeOutChannel(channel, 0);
        }
}
