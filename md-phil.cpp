// Ripped Snes9X joystick code and made it Genesis friendly, with a few
// slight modifications. [PKH]

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "md.h"
#include "md-phil.h"

// Default setup for Gravis GamePad Pro 10 button controllers.

// Yellow ... A
// Green .... C
// Red ...... A
// Blue ..... B
// L1 ....... Y
// R1 ....... Z
// L2 ....... X
// R2 ....... X
// Start .... START  :)
// Select ... MODE

// Mappings are configurable via dgenrc.  Mappings are done from 0-16
// Standard 2 button joysticks will only have buttons 0 and 1.  [PKH]

extern int js_map_button [2][16];

#ifdef LINUX_JOYSTICK_SUPPORT
#warning USING LINUX JOYSTICK
#include <linux/joystick.h>

int joypads[2] = {0};

int js_fd[2] = {-1, -1};

char *js_device [2] = {"/dev/js0", "/dev/js1"};

void md::init_joysticks() {
#ifdef JSIOCGVERSION
    int version;
    unsigned char axes, buttons;

    if ((js_fd [0] = open (js_device [0], O_RDONLY | O_NONBLOCK)) < 0)
    {
	perror (js_device [0]);
	return;
    }

    if (ioctl (js_fd [0], JSIOCGVERSION, &version))
    {
        puts("joystick: You need at least driver version 1.0 for joystick support");
	close (js_fd [0]);
	return;
    }
    js_fd [1] = open (js_device [1], O_RDONLY | O_NONBLOCK);

#ifdef JSIOCGNAME
    char name [130];
    bzero (name, 128);
    if (ioctl (js_fd [0], JSIOCGNAME(128), name) > 0) 
    {
        printf ("Using %s (%s) as pad1", name, js_device [0]);
        if (js_fd [1] > 0)
	{
	    ioctl (js_fd [1], JSIOCGNAME(128), name);
	    printf ("and %s (%s) as pad2", name, js_device [1]);
	}
    } 
    else

#endif // JSIOCGNAME
    {
	ioctl (js_fd [0], JSIOCGAXES, &axes);
	ioctl (js_fd [0], JSIOCGBUTTONS, &buttons);
	printf ("Using %d-axis %d-button joystick (%s) as pad1", axes, buttons, js_device [0]);
	if (js_fd [1] > 0)
	{
	    ioctl (js_fd [0], JSIOCGAXES, &axes);
	    ioctl (js_fd [0], JSIOCGBUTTONS, &buttons);
	    printf (" and %d-axis %d-button (%s) as pad2", axes, buttons, js_device [1]);
	}
    }
    puts (".");
    
#endif // JSIOCGVERSION
}

void md::read_joysticks()
{
#ifdef JSIOCGVERSION
    struct js_event js_ev;
    int i;

    for (i = 0; i < 2 && js_fd [i] >= 0; i++)
    {
	while (read (js_fd[i], &js_ev, sizeof (struct js_event)) == sizeof (struct js_event) )
	{
	    switch (js_ev.type & ~JS_EVENT_INIT)
	    {
	    case JS_EVENT_AXIS:
		if (js_ev.number == 0)
		{
		    if(js_ev.value < -16384)
		    {
			pad[i] &= ~MD_LEFT_MASK; 
			pad[i] |= MD_RIGHT_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			pad[i] |= MD_LEFT_MASK;
			pad[i] &= ~MD_RIGHT_MASK; 
			break;
		    }
		    pad[i] |= MD_LEFT_MASK;
		    pad[i] |= MD_RIGHT_MASK;
		    break;	
		}

		if (js_ev.number == 1)
		{
		    if (js_ev.value < -16384)
		    {
			pad[i] &= ~MD_UP_MASK; 
			pad[i] |= MD_DOWN_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			pad[i] |= MD_UP_MASK;
			pad[i] &= ~MD_DOWN_MASK; 
			break;
		    }
		    pad[i] |= MD_UP_MASK;
		    pad[i] |= MD_DOWN_MASK;
		    break;	
		}

		break;

	    case JS_EVENT_BUTTON:
		if (js_ev.number > 15)
		    break;

		if (js_ev.value)
		    pad[i] &= ~js_map_button [i][js_ev.number];
		else
		    pad[i] |= js_map_button [i][js_ev.number];

		break;
	    }
	}
    }
#endif // JSIOCGVERSION
}

#elif defined(SDL_JOYSTICK_SUPPORT)
#warning USING SDL JOYSTICK
#include <SDL.h>
#include <SDL_joystick.h>

static SDL_Joystick *js_handle[2] = { NULL, NULL };

void md::init_joysticks() {
  // Initialize the joystick support
  // Thanks to Cameron Moore <cameron@unbeatenpath.net>
  if(SDL_Init(SDL_INIT_JOYSTICK) < 0)
    {
      fprintf(stderr, "joystick: Unable to initialize joystick system\n");
      return;
    }

  // Open the first couple of joysticks, if possible
  js_handle[0] = SDL_JoystickOpen(0);
  js_handle[1] = SDL_JoystickOpen(1);

  // If neither opened, quit
  if(!(js_handle[0] || js_handle[1]))
    {
      fprintf(stderr, "joystick: Unable to open any joysticks\n");
      return;
    }

  // Print the joystick names
  printf("Using ");
  if(js_handle[0]) printf("%s (#0) as pad1 ", SDL_JoystickName(0));
  if(js_handle[0] && js_handle[1]) printf("and ");
  if(js_handle[1]) printf("%s (#1) as pad2 ", SDL_JoystickName(1));
  printf("\n");

  // Enable joystick events
  SDL_JoystickEventState(SDL_ENABLE);
}

// This does nothing; SDL joystick handling is in the main event loop in
// sdl/sdl.cpp
void md::read_joysticks()
{
  //empty
}

#endif
