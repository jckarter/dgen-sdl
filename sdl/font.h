#ifndef __FONT_H__
#define __FONT_H__
// DGen/SDL v1.14+
// Font routine interface

#include <SDL.h>

// Writes a string of text on the surface at given x any y coordinates
void font_text(SDL_Surface *surf, int x, int y, const char *message);
// Writes a string with given length
void font_text_n(SDL_Surface *surf, int x, int y, const char *message, int n);

#endif __FONT_H__
