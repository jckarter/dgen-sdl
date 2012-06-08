#ifndef __RC_VARS_H__
#define __RC_VARS_H__

// DGen/SDL v1.17+
// RC-modified variables listed here

#include "pd-defs.h" // Keysyms are defined in here

// main.cpp defines IS_MAIN_CPP, which means we actually define the variables.
// Otherwise, we just declare them as externs
#ifdef IS_MAIN_CPP
#define RCVAR(name, def) long name = def
#else
#define RCVAR(name, def) extern long name
#endif

RCVAR(pad1_up, PDK_UP);
RCVAR(pad1_down, PDK_DOWN);
RCVAR(pad1_left, PDK_LEFT);
RCVAR(pad1_right, PDK_RIGHT);
RCVAR(pad1_a, PDK_a);
RCVAR(pad1_b, PDK_s);
RCVAR(pad1_c, PDK_d);
RCVAR(pad1_x, PDK_q);
RCVAR(pad1_y, PDK_w);
RCVAR(pad1_z, PDK_e);
RCVAR(pad1_mode, PDK_BACKSPACE);
RCVAR(pad1_start, PDK_RETURN);

RCVAR(pad2_up, PDK_KP8);
RCVAR(pad2_down, PDK_KP2);
RCVAR(pad2_left, PDK_KP4);
RCVAR(pad2_right, PDK_KP6);
RCVAR(pad2_a, PDK_DELETE);
RCVAR(pad2_b, PDK_END);
RCVAR(pad2_c, PDK_PAGEDOWN);
RCVAR(pad2_x, PDK_INSERT);
RCVAR(pad2_y, PDK_HOME);
RCVAR(pad2_z, PDK_PAGEUP);
RCVAR(pad2_mode, PDK_KP_PLUS);
RCVAR(pad2_start, PDK_KP_ENTER);

RCVAR(dgen_fix_checksum, PDK_F1);
RCVAR(dgen_quit, PDK_ESCAPE);
RCVAR(dgen_splitscreen_toggle, PDK_F4);
RCVAR(dgen_craptv_toggle, PDK_F5);
RCVAR(dgen_screenshot, PDK_F12);
RCVAR(dgen_reset, PDK_TAB);
RCVAR(dgen_cpu_toggle, PDK_F11);
RCVAR(dgen_stop, PDK_z);
RCVAR(dgen_fullscreen_toggle, KEYSYM_MOD_ALT | PDK_RETURN);

RCVAR(dgen_slot_0, PDK_0);
RCVAR(dgen_slot_1, PDK_1);
RCVAR(dgen_slot_2, PDK_2);
RCVAR(dgen_slot_3, PDK_3);
RCVAR(dgen_slot_4, PDK_4);
RCVAR(dgen_slot_5, PDK_5);
RCVAR(dgen_slot_6, PDK_6);
RCVAR(dgen_slot_7, PDK_7);
RCVAR(dgen_slot_8, PDK_8);
RCVAR(dgen_slot_9, PDK_9);
RCVAR(dgen_save, PDK_F2);
RCVAR(dgen_load, PDK_F3);

RCVAR(dgen_splitscreen_startup, 0);
RCVAR(dgen_autoload, 0);
RCVAR(dgen_autosave, 0);
RCVAR(dgen_frameskip, 1);
RCVAR(dgen_show_carthead, 0);

RCVAR(dgen_sound, 1);
RCVAR(dgen_soundrate, 22050);
RCVAR(dgen_16bit, 1);
RCVAR(dgen_soundsegs, 8);

RCVAR(dgen_craptv, 0);
RCVAR(dgen_nice, 0);
RCVAR(dgen_joystick, 0);

RCVAR(dgen_fullscreen, 0);
RCVAR(dgen_scale, 1);
RCVAR(dgen_opengl, 0);
RCVAR(dgen_opengl_width, 640);
RCVAR(dgen_opengl_height, 480);

#endif // __RC_VARS_H__
