#ifndef __SDL_PD_DEFS_H__
#define __SDL_PD_DEFS_H__

#include <SDL.h>
#include <SDL_audio.h>

// Platform-dependent definitions and inlines.
// In this file, you should define all the keysyms and audio formats, and
// if you want to inline any functions, put them in here too. :)

// There are two necessary formats: PD_SND_8, unsigned 8-bit audio, and
// PD_SND_16, signed 16-bit audio in the same endianness as your CPU.
#define PD_SND_8 AUDIO_U8
#ifdef WORDS_BIGENDIAN
#define PD_SND_16 AUDIO_S16MSB
#else
#define PD_SND_16 AUDIO_S16LSB
#endif

// Now for a grueling list of keysyms. *muahahahahahaha*
// Seriously, this is tedious, I realise, but don't be discouraged please. :)
// Just remember I had to do it also.
#define PDK_ESCAPE SDLK_ESCAPE
#define PDK_1 SDLK_1
#define PDK_2 SDLK_2
#define PDK_3 SDLK_3
#define PDK_4 SDLK_4
#define PDK_5 SDLK_5
#define PDK_6 SDLK_6
#define PDK_7 SDLK_7
#define PDK_8 SDLK_8
#define PDK_9 SDLK_9
#define PDK_0 SDLK_0
#define PDK_MINUS SDLK_MINUS
#define PDK_EQUALS SDLK_EQUALS
#define PDK_BACKSPACE SDLK_BACKSPACE
#define PDK_TAB SDLK_TAB
#define PDK_q SDLK_q
#define PDK_w SDLK_w
#define PDK_e SDLK_e
#define PDK_r SDLK_r
#define PDK_t SDLK_t
#define PDK_y SDLK_y
#define PDK_u SDLK_u
#define PDK_i SDLK_i
#define PDK_o SDLK_o
#define PDK_p SDLK_p
#define PDK_LEFTBRACKET SDLK_LEFTBRACKET
#define PDK_RIGHTBRACKET SDLK_RIGHTBRACKET
#define PDK_RETURN SDLK_RETURN
#define PDK_a SDLK_a
#define PDK_s SDLK_s
#define PDK_d SDLK_d
#define PDK_f SDLK_f
#define PDK_g SDLK_g
#define PDK_h SDLK_h
#define PDK_j SDLK_j
#define PDK_k SDLK_k
#define PDK_l SDLK_l
#define PDK_SEMICOLON SDLK_SEMICOLON
#define PDK_QUOTE SDLK_QUOTE
#define PDK_BACKQUOTE SDLK_BACKQUOTE
#define PDK_BACKSLASH SDLK_BACKSLASH
#define PDK_z SDLK_z
#define PDK_x SDLK_x
#define PDK_c SDLK_c
#define PDK_v SDLK_v
#define PDK_b SDLK_b
#define PDK_n SDLK_n
#define PDK_m SDLK_m
#define PDK_COMMA SDLK_COMMA
#define PDK_PERIOD SDLK_PERIOD
#define PDK_SLASH SDLK_SLASH
#define PDK_KP_MULTIPLY SDLK_KP_MULTIPLY
#define PDK_SPACE SDLK_SPACE
#define PDK_F1 SDLK_F1
#define PDK_F2 SDLK_F2
#define PDK_F3 SDLK_F3
#define PDK_F4 SDLK_F4
#define PDK_F5 SDLK_F5
#define PDK_F6 SDLK_F6
#define PDK_F7 SDLK_F7
#define PDK_F8 SDLK_F8
#define PDK_F9 SDLK_F9
#define PDK_F10 SDLK_F10
#define PDK_KP7 SDLK_KP7
#define PDK_KP8 SDLK_KP8
#define PDK_KP9 SDLK_KP9
#define PDK_KP_MINUS SDLK_KP_MINUS
#define PDK_KP4 SDLK_KP4
#define PDK_KP5 SDLK_KP5
#define PDK_KP6 SDLK_KP6
#define PDK_KP_PLUS SDLK_KP_PLUS
#define PDK_KP1 SDLK_KP1
#define PDK_KP2 SDLK_KP2
#define PDK_KP3 SDLK_KP3
#define PDK_KP0 SDLK_KP0
#define PDK_KP_PERIOD SDLK_KP_PERIOD
#define PDK_F11 SDLK_F11
#define PDK_F12 SDLK_F12
#define PDK_KP_ENTER SDLK_KP_ENTER
#define PDK_KP_DIVIDE SDLK_KP_DIVIDE
#define PDK_HOME SDLK_HOME
#define PDK_UP SDLK_UP
#define PDK_PAGEUP SDLK_PAGEUP
#define PDK_LEFT SDLK_LEFT
#define PDK_RIGHT SDLK_RIGHT
#define PDK_END SDLK_END
#define PDK_DOWN SDLK_DOWN
#define PDK_PAGEDOWN SDLK_PAGEDOWN
#define PDK_INSERT SDLK_INSERT
#define PDK_DELETE SDLK_DELETE
#define PDK_NUMLOCK SDLK_NUMLOCK
#define PDK_CAPSLOCK SDLK_CAPSLOCK
#define PDK_SCROLLOCK SDLK_SCROLLOCK
#define PDK_LSHIFT SDLK_LSHIFT
#define PDK_RSHIFT SDLK_RSHIFT
#define PDK_LCTRL SDLK_LCTRL
#define PDK_RCTRL SDLK_RCTRL
#define PDK_LALT SDLK_LALT
#define PDK_RALT SDLK_RALT
#define PDK_LMETA SDLK_LMETA
#define PDK_RMETA SDLK_RMETA

// There, that wasn't so hard, was it? :)
// If you want to inline any pd_ functions, put their bodies here.
// Otherwise, you're done with this file! :D

#endif // __SDL_PD_DEFS_H__
