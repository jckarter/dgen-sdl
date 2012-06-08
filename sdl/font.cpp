// DGen/SDL v1.14+
// by Joe Groff
// How's my programming? E-mail <joe@pknet.com>

/* DGen's font renderer.
 * I hope it's pretty well detached from the DGen core, so you can use it in
 * any other SDL app you like. */

/* Also note that these font renderers do no error detection, and absolutely
 * NO clipping whatsoever, so try to keep the glyphs on-screen. Thank you :-)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "font.h"	/* The interface functions */

extern int *dgen_font[];

// Support function
// Put a glyph at the specified coordinates
// THIS IS REALLY A MACRO - SDL_LockSurface should have been called already if
// necessary
static inline void _putglyph(char *p, int Bpp, int pitch, char which)
{
  int *glyph = dgen_font[which];
  int x = 0, i;

  for(; *glyph != -1; ++glyph)
    {
      p += (((x += *glyph) >> 3) * pitch); x &= 7;
      for(i = 0; i < Bpp; ++i) p[(x * Bpp) + i] = 0xff;
    }
}

// This writes a string of text at the given x and y coordinates
void font_text(SDL_Surface *surf, int x, int y, const char *message)
{
  int pitch = surf->pitch, Bpp = surf->format->BytesPerPixel;
  char *p = (char*)surf->pixels + (pitch * y) + (Bpp * x);

  if(SDL_MUSTLOCK(surf))
    if(SDL_LockSurface(surf) < 0)
      {
        fprintf(stderr, "font: Couldn't lock screen: %s!", SDL_GetError());
	return;
      }
  for(; *message; p += (8 * Bpp), ++message)
    _putglyph(p, Bpp, pitch, *message);
  if(SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
}

// This writes a string of text of fixed length n
void font_text_n(SDL_Surface *surf, int x, int y, const char *message, int n)
{
  int pitch = surf->pitch, Bpp = surf->format->BytesPerPixel;
  char *p = (char*)surf->pixels + (pitch * y) + (Bpp * x);

  if(SDL_MUSTLOCK(surf))
    if(SDL_LockSurface(surf) < 0)
      {
        fprintf(stderr, "font: Couldn't lock screen: %s!", SDL_GetError());
	return;
      }
  for(; n > 0; p += (8 * Bpp), ++message, --n)
    _putglyph(p, Bpp, pitch, *message);
  if(SDL_MUSTLOCK(surf)) SDL_UnlockSurface(surf);
}

