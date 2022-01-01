#ifndef INVADERS_H
#define INVADERS_H

#include <stdio.h>

int invaders_init(FILE* f, size_t fsize);
void invaders_render();
int invaders_update();
void invaders_quit();

#endif
