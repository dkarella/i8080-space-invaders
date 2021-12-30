#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
        SOUND_UFO,
        SOUND_SHOT,
        SOUND_PLAYER_DIE,
        SOUND_INVADER_DIE,
        SOUND_FLEET_MOVEMENT_1,
        SOUND_FLEET_MOVEMENT_2,
        SOUND_FLEET_MOVEMENT_3,
        SOUND_FLEET_MOVEMENT_4,
        SOUND_UFO_DIE,
        NUM_SOUNDS
} audio_sound;

int audio_init();
void audio_quit();
void audio_play(audio_sound sound);
int audio_loop(audio_sound sound);
void audio_stop(int channel);

#endif
