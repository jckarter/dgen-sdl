// DGen/SDL v1.21+
// SDL interface
// OpenGL code added by Andre Duarte de Souza <asouza@olinux.com.br>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <SDL.h>
#include <SDL_audio.h>

#ifdef SDL_OPENGL_SUPPORT
# include <GL/gl.h>
# include "ogl_fonts.h"
#endif

#include "md.h"
#include "rc.h"
#include "rc-vars.h"
#include "pd.h"
#include "pd-defs.h"
#include "font.h"

#ifdef SDL_OPENGL_SUPPORT
// Defines for RGBA
# define R 0
# define G 1
# define B 2
# define A 3
// Constant alpha value
# define Alpha 255
// Width and height of screen
# define XRES xs
# define YRES ys

// Where tex (256x256) ends in x
// (256/320 == 512/640. 512-320 == 192 (Negative half ignored).
// Positive tex end pos (range from 0.0 to 1.0 (0 to 320) in x) == 192/320)
static double tex_end = (double)192/320;
// Framebuffer textures
static unsigned char mybuffer[256][256][4];
static unsigned char mybufferb[256][64][4];
// Textures (one 256x256 and on 64x256 => 320x256)
static GLuint texture[2];
// xtabs for faster buffer access
static int xtab[321]; // x - 256 for the 64x256 texture
static int x4tab_r[321], x4tab_g[321], x4tab_b[321]; // x*4 (RGBA)
// Display list
static GLuint dlist;
// Texture pitches
static int mypitch = 256*4, mypitchb = 64*4;
// Is OpenGL mode enabled?
static int opengl = 0;
// Text
static unsigned char message[5][256][4];
static unsigned char m_clear[256][5][4];
#endif // SDL_OPENGL_SUPPORT

// Bad hack- extern slot etc. from main.cpp so we can save/load states
extern int slot;
void md_save(md &megad);
void md_load(md &megad);

// Temp space
static char temp[256];

// Define externed variables
struct bmap mdscr;
unsigned char *mdpal = NULL;
struct sndinfo sndi;
char *pd_options = "fX:Y:S:"
#ifdef SDL_OPENGL_SUPPORT
  "G:"
#endif
  ;

// Define our personal variables
// Graphics
static SDL_Surface *screen = NULL;
static SDL_Color colors[64];
static int ysize = 0, fullscreen = 0, bytes_pixel = 0, pal_mode = 0;
static int x_scale = 1, y_scale = 1, xs, ys;
// Sound
static SDL_AudioSpec spec;
static int snd_rate, snd_segs, snd_16bit, snd_buf;
static volatile int snd_read = 0;
static Uint8 *playbuf = NULL;
// Messages
static volatile int sigalrm_happened = 0;

#ifdef SDL_JOYSTICK_SUPPORT
// Extern joystick stuff
extern int js_map_button[2][16];
#endif

// Number of seconds to sustain messages
#define MESSAGE_LIFE 3

// Catch SIGALRM
static void sigalrm_handler(int)
{
  sigalrm_happened = 1;
}

// Screenshots, thanks to Allan Noe <psyclone42@geocities.com>
static void do_screenshot(void) {
  char fname[20], msg[80], *ok;
  static int n = 0;
  int x;
  FILE *fp;

#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    {
      pd_message("Screenshot not supported in OpenGL mode");
      return;
    }
#endif

  for(;;)
    { 
      sprintf(fname, "shot%04d.bmp", n);
      if ((fp = fopen(fname, "r")) == NULL)
        break;
      else
	fclose(fp);
      if (++n > 9999)
        {
	  pd_message("No more screenshot filenames!");
	  return;
	}
     }
 
  x = SDL_SaveBMP(screen, fname);

  if(x)
     pd_message("Screenshot failed!");
  else
    {
      sprintf(msg, "Screenshot written to %s", fname);
      pd_message(msg);
    }
}

// Document the -f switch
void pd_help()
{
  printf(
  "    -f              Attempt to run fullscreen.\n"
  "    -X scale        Scale the screen in the X direction.\n"
  "    -Y scale        Scale the screen in the Y direction.\n"
  "    -S scale        Scale the screen by the same amount in both directions.\n"
#ifdef SDL_OPENGL_SUPPORT
  "    -G XxY          Use OpenGL mode, with width X and height Y.\n"
#endif
  );
}

// Handle the switches
void pd_option(char c, const char *)
{
  // Set stuff up from the rcfile first, so we can override it with commandline
  // options
  fullscreen = dgen_fullscreen;
  x_scale = y_scale = dgen_scale;
#ifdef SDL_OPENGL_SUPPORT
  opengl = dgen_opengl;
  if(opengl)
    {
      xs = dgen_opengl_width;
      ys = dgen_opengl_height;
    }
#endif

  if(c == 'f') fullscreen = !fullscreen;
  if(c == 'X') x_scale = atoi(optarg);
  if(c == 'Y') y_scale = atoi(optarg);
  if(c == 'S') x_scale = y_scale = atoi(optarg);
#ifdef SDL_OPENGL_SUPPORT
  if(c == 'G')
    {
      sscanf(optarg, " %d x %d ", &xs, &ys);
      opengl = 1;
    }
#endif
}

#ifdef SDL_OPENGL_SUPPORT
static void maketex(int num, int size)
{
  glGenTextures(num,&texture[num-1]);
  glBindTexture(GL_TEXTURE_2D,texture[num-1]);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  switch (num)
    {
    case 1:
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,size,256,0,GL_RGBA, GL_UNSIGNED_BYTE, mybuffer);
      break;
    case 2:
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,size,256,0,GL_RGBA, GL_UNSIGNED_BYTE, mybufferb);
      break;
    };
}

static void makedlist()
{
  int i;

  dlist=glGenLists(1);
  glNewList(dlist,GL_COMPILE);

  glEnable(GL_TEXTURE_2D);

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_REPLACE);

  // 256x256
  glBindTexture(GL_TEXTURE_2D, texture[0]);

  glBegin(GL_QUADS);
    glTexCoord2f(0.0,1.0); glVertex2f(-1.0,-1.0); // upper left
    glTexCoord2f(0.0,0.0); glVertex2f(-1.0,1.0); // lower left
    glTexCoord2f(1.0,0.0); glVertex2f(tex_end,1.0); // lower right
    glTexCoord2f(1.0,1.0); glVertex2f(tex_end,-1.0); // upper right
  glEnd();

  // 64x256
  glBindTexture(GL_TEXTURE_2D, texture[1]);

  glBegin(GL_QUADS);
    glTexCoord2f(0.0,1.0); glVertex2f(tex_end,-1.0); // upper left
    glTexCoord2f(0.0,0.0); glVertex2f(tex_end,1.0); // lower left
    glTexCoord2f(1.0,0.0); glVertex2f(1.0,1.0); // lower right
    glTexCoord2f(1.0,1.0); glVertex2f(1.0,-1.0); // upper right
  glEnd();

  glDisable(GL_TEXTURE_2D);

  glEndList();
}

static void init_textures()
{
  int i,j;

  // First, the x tables (for a little faster access)
  for (i=256;i<321;i++)
    xtab[i]=i-256;
  for (i=0;i<321;i++)
    {
      x4tab_r[i]=(i*4)+2;
      x4tab_g[i]=(i*4)+1;
      x4tab_b[i]=i*4;
    }

  // Constant Alpha
  for (j=0;j<256;j++)
    for (i=0;i<320;i++)
      {
	if (i<256) mybuffer[j][i][A]=Alpha;
	else mybufferb[j][i-256][A]=Alpha;
      }

  // Clear Message Buffer
  for (j=0;j<256;j++)
    for (i=0;i<5;i++)
      {
	m_clear[i][j][R]=m_clear[i][j][G]=m_clear[i][j][B]=0;
	m_clear[i][j][A]=255;
      }

  // Dithering
  glEnable(GL_DITHER);
  // Anti-aliasing
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POINT_SMOOTH);
  
  glClearColor(1.0,1.0,1.0,1.0);
  glShadeModel(GL_FLAT);

  maketex(1,256);
  maketex(2,64);

  makedlist();
}

static void display()
{
  glCallList(dlist);
  SDL_GL_SwapBuffers();
}
#endif // SDL_OPENGL_SUPPORT

// Initialize SDL, and the graphics
int pd_graphics_init(int want_sound, int want_pal)
{
  SDL_Color color;

  pal_mode = want_pal;

  /* Neither scale value may be 0 or negative */
  if(x_scale <= 0) x_scale = 1;
  if(y_scale <= 0) y_scale = 1;

  if(SDL_Init(want_sound? (SDL_INIT_VIDEO | SDL_INIT_AUDIO) : (SDL_INIT_VIDEO)))
    {
      fprintf(stderr, "sdl: Couldn't init SDL: %s!\n", SDL_GetError());
      return 0;
    }
  atexit(SDL_Quit);
  ysize = (want_pal? 240 : 224);

  // Set screen size vars
#ifdef SDL_OPENGL_SUPPORT
  if(!opengl)
#endif
    xs = 320*x_scale, ys = ysize*y_scale;

  // Make a 320x224 or 320x240 display for the MegaDrive, with an extra 16 lines
  // for the message bar.
#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    screen = SDL_SetVideoMode(xs, ys, 0,
      fullscreen? (SDL_HWPALETTE | SDL_HWSURFACE | SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_FULLSCREEN) :
		  (SDL_HWPALETTE | SDL_HWSURFACE | SDL_OPENGL | SDL_GL_DOUBLEBUFFER));
  else
#endif
    screen = SDL_SetVideoMode(xs, ys + 16, 0,
      fullscreen? (SDL_HWPALETTE | SDL_HWSURFACE | SDL_FULLSCREEN) :
		  (SDL_HWPALETTE | SDL_HWSURFACE));
  if(!screen)
    {
      fprintf(stderr, "sdl: Couldn't set %dx%d video mode: %s!",
	      xs, ys, SDL_GetError());
      return 0;
    }
  // We don't need setuid priveledges anymore
  if(getuid() != geteuid())
    setuid(getuid());

  // Set the titlebar
  SDL_WM_SetCaption("DGen "VER, "dgen");
  // Hide the cursor
  SDL_ShowCursor(0);

#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    init_textures();
#endif

#ifdef SDL_OPENGL_SUPPORT
  if(!opengl)
#endif
    // If we're in 8 bit mode, set color 0xff to white for the text,
    // and make a palette buffer
    if(screen->format->BitsPerPixel == 8)
      {
        color.r = color.g = color.b = 0xff;
        SDL_SetColors(screen, &color, 0xff, 1);
        mdpal = (unsigned char*)malloc(256);
        if(!mdpal)
          {
	    fprintf(stderr, "sdl: Couldn't allocate palette!\n");
	    return 0;
	  }
      }
  
  // Ignore events besides quit and keyboard
  SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);

  // Set up the MegaDrive screen
#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    bytes_pixel = 4;
  else
#endif
    bytes_pixel = screen->format->BytesPerPixel;
  mdscr.w = 320 + 16;
  mdscr.h = ysize + 16;
#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    mdscr.bpp = 32;
  else
#endif
    mdscr.bpp = screen->format->BitsPerPixel;
  mdscr.pitch = mdscr.w * bytes_pixel;
  mdscr.data = (unsigned char*) malloc(mdscr.pitch * mdscr.h);
  if(!mdscr.data)
    {
      fprintf(stderr, "sdl: Couldn't allocate screen!\n");
      return 0;
    }

  // Set SIGALRM handler (used to clear messages after 3 seconds)
  signal(SIGALRM, sigalrm_handler);

  // And that's it! :D
  return 1;
}

// Update palette
void pd_graphics_palette_update()
{
  int i;
  for(i = 0; i < 64; ++i)
    {
      colors[i].r = mdpal[(i << 2)  ];
      colors[i].g = mdpal[(i << 2)+1];
      colors[i].b = mdpal[(i << 2)+2];
    }
#ifdef SDL_OPENGL_SUPPORT
  if(!opengl)
#endif
    SDL_SetColors(screen, colors, 0, 64);
}

#ifdef SDL_OPENGL_SUPPORT
void update_textures() {
  int i,x,y;
  int c,x2;

  glBindTexture(GL_TEXTURE_2D,texture[0]);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,15,256,224,GL_RGBA,GL_UNSIGNED_BYTE,mybuffer);

  glBindTexture(GL_TEXTURE_2D,texture[1]);
  glTexSubImage2D(GL_TEXTURE_2D,0,0,15,64,224,GL_RGBA,GL_UNSIGNED_BYTE,mybufferb);

  display();
}
#endif

// Update screen
// This code is fairly transmittable to any linear display, just change p to
// point to your favorite raw framebuffer. ;) But planar buffers are a 
// completely different deal...
// Anyway, feel free to use it in your implementation. :)
void pd_graphics_update()
{
  static int f = 0;
  int i, j, k;
  unsigned char *p, *q;
#ifdef SDL_OPENGL_SUPPORT
  int x, y, x2;
  unsigned char *pb, *qb;
#endif
  
  // If you need to do any sort of locking before writing to the buffer, do so
  // here.
  if(SDL_MUSTLOCK(screen))
    SDL_LockSurface(screen);
  
  // If SIGALRM was tripped, clear message
  if(sigalrm_happened)
    {
      sigalrm_happened = 0;
      pd_clear_message();
    }

#ifdef SDL_OPENGL_SUPPORT
  if(!opengl)
#endif
    p = (unsigned char*)screen->pixels;
  // 2696 = 336 * 8 + 8. 336 should be the width of mdscr, so we move 8 pixels
  // down and 8 to the right to skip the messy border.
  q = (unsigned char*)mdscr.data + 2696 * bytes_pixel;

  for(i = 0; i < ysize; ++i)
    {
#ifdef SDL_OPENGL_SUPPORT
      if(opengl)
	{
	  // Copy, converting from BGRA to RGBA
	  for(x = 0; x < 320; ++x)
	    {
	      if(x < 256)
	        {
		  mybuffer[i][x][R] = *(q + x4tab_r[x]);
		  mybuffer[i][x][G] = *(q + x4tab_g[x]);
		  mybuffer[i][x][B] = *(q + x4tab_b[x]);
		}
	      else if(x < 320)
	        {
		  mybufferb[i][xtab[x]][R] = *(q + x4tab_r[x]);
		  mybufferb[i][xtab[x]][G] = *(q + x4tab_g[x]);
		  mybufferb[i][xtab[x]][B] = *(q + x4tab_b[x]);
		}
	    }
	}
      else
        {
#endif // SDL_OPENGL_SUPPORT
#ifdef ASM_CTV
          if(dgen_craptv) switch(dgen_craptv)
            {
	    // Blur, by Dave
	    case CTV_BLUR:
	      if(mdscr.bpp == 16) blur_bitmap_16(q, 319);
	      else if(mdscr.bpp == 15) blur_bitmap_15(q, 319);
	      break;
	    // Scanline, by Phil
	    case CTV_SCANLINE:
	      if((i & 1) && (mdscr.bpp == 16 || mdscr.bpp == 15))
	        test_ctv(q, 320);
	      break;
	    // Interlace, a hacked form of Scanline by me :)
	    case CTV_INTERLACE:
	      if((i & 1) ^ (++f & 1) && (mdscr.bpp == 16 || mdscr.bpp == 15))
	        test_ctv(q, 320);
	      break;
	    default:
	      break;
	    }
#endif // ASM_CTV
          if(x_scale == 1)
            {
	      if(y_scale == 1)
	        {
	          memcpy(p, q, 320 * bytes_pixel);
	          p += screen->pitch;
	        }
	      else
	        {
	          for(j = 0; j < y_scale; ++j)
	            {
		      memcpy(p, q, 320 * bytes_pixel);
		      p += screen->pitch;
		    }
	        }
	    }
          else
            {
	      /* Stretch the scanline out */
	      switch(bytes_pixel)
	        {
	        case 1:
	          {
	            unsigned char *pp = p, *qq = q;
	            for(j = 0; j < 320; ++j, ++qq)
	              for(k = 0; k < x_scale; ++k)
		        *(pp++) = *qq;
	            if(y_scale != 1)
	              for(pp = p, j = 1; j < y_scale; ++j)
		        {
		          p += screen->pitch;
		          memcpy(p, pp, xs);
		        }
	          }
	          break;
	        case 2:
	          {
	            short *pp = (short*)p, *qq = (short*)q;
	            for(j = 0; j < 320; ++j, ++qq)
	              for(k = 0; k < x_scale; ++k)
		        *(pp++) = *qq;
	            if(y_scale != 1)
	              for(pp = (short*)p, j = 1; j < y_scale; ++j)
		        {
		          p += screen->pitch;
		          memcpy(p, pp, xs*2);
		        }
	          }
	          break;
	        case 3:
	          /* FIXME */
	          break;
	        case 4:
	          {
	            long *pp = (long*)p, *qq = (long*)q;
	            for(j = 0; j < 320; ++j, ++qq)
	              for(k = 0; k < x_scale; ++k)
		        *(pp++) = *qq;
	            if(y_scale != 1)
	              for(pp = (long*)p, j = 1; j < y_scale; ++j)
		        {
		          p += screen->pitch;
		          memcpy(p, pp, xs*4);
		        }
	          }
	          break;
	        }
	      p += screen->pitch;
	    }
#ifdef SDL_OPENGL_SUPPORT
        }
#endif
      q += mdscr.pitch;
    }
  // Unlock when you're done!
  if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
  // Update the screen
#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    update_textures();
  else
#endif
    SDL_UpdateRect(screen, 0, 0, xs, ys);
}
  
// Callback for sound
static void _snd_callback(void*, Uint8 *stream, int len)
{
  int i;
  // Slurp off the play buffer
  for(i = 0; i < len; ++i)
    {
      if(snd_read == snd_buf) snd_read = 0;
      stream[i] = playbuf[snd_read++];
    }
}

// Initialize the sound
int pd_sound_init(long &format, long &freq, long &segs)
{
  SDL_AudioSpec wanted;

  // Set the desired format
  wanted.freq = freq;
  wanted.format = format;
  wanted.channels = 2;
  wanted.samples = 512;
  wanted.callback = _snd_callback;
  wanted.userdata = NULL;

  // Open audio, and get the real spec
  if(SDL_OpenAudio(&wanted, &spec) < 0)
    {
      fprintf(stderr, "sdl: Couldn't open audio: %s!\n", SDL_GetError());
      return 0;
    }
  // Check everything
  if(spec.channels != 2)
    {
      fprintf(stderr, "sdl: Couldn't get stereo audio format!\n");
      goto snd_error;
    }
  if(spec.format == PD_SND_16)
    snd_16bit = 1, format = PD_SND_16;
  else if(spec.format == PD_SND_8)
    snd_16bit = 0, format = PD_SND_8;
  else
    {
      fprintf(stderr, "sdl: Couldn't get a supported audio format!\n");
      goto snd_error;
    }
  // Set things as they really are
  snd_rate = freq = spec.freq;
  sndi.len = spec.freq / (pal_mode? 50 : 60);
  if(segs <= 4) segs = snd_segs = 4;
  else if(segs <= 8) segs = snd_segs = 8;
  else if(segs <= 16) segs = snd_segs = 16;
  else segs = snd_segs = 32;

  // Calculate buffer size
  snd_buf = sndi.len * snd_segs * (snd_16bit? 4 : 2);
  --snd_segs;
  // Allocate play buffer
  if(!(sndi.l = (signed short*) malloc(sndi.len * sizeof(signed short))) ||
     !(sndi.r = (signed short*) malloc(sndi.len * sizeof(signed short))) ||
     !(playbuf = (Uint8*)malloc(snd_buf)))
    {
      fprintf(stderr, "sdl: Couldn't allocate sound buffers!\n");
      goto snd_error;
    }

  // It's all good!
  return 1;

snd_error:
  // Oops! Something bad happened, cleanup.
  SDL_CloseAudio();
  if(sndi.l) free((void*)sndi.l);
  if(sndi.r) free((void*)sndi.r);
  if(playbuf) free((void*)playbuf);
  return 0;
}

// Start/stop audio processing
void pd_sound_start()
{
  SDL_PauseAudio(0);
}

void pd_sound_pause()
{
  SDL_PauseAudio(1);
}

// Return segment we're currently playing from
int pd_sound_rp()
{
  return (snd_read / (sndi.len << (1+snd_16bit))) & snd_segs;
}

// Write contents of sndi to playbuf
void pd_sound_write(int seg)
{
  int i;
  signed short *w =
    (signed short*)(playbuf + seg * (sndi.len << (1+snd_16bit)));

  // Thanks Daniel Wislocki for this much improved audio loop :)
  if(!snd_16bit)
    {
      SDL_LockAudio();
      for(i = 0; i < sndi.len; ++i)
	*w++ = ((sndi.l[i] & 0xFF00) + 0x8000) | ((sndi.r[i] >> 8) + 0x80);
      SDL_UnlockAudio();
    } else {
      SDL_LockAudio();
      for(i = 0; i < sndi.len; ++i)
	{
	  *w++ = sndi.l[i];
	  *w++ = sndi.r[i];
	}
      SDL_UnlockAudio();
    }
}

// This is a small event loop to handle stuff when we're stopped.
static int stop_events(md &/*megad*/)
{
  SDL_Event event;

  // We still check key events, but we can wait for them
  while(SDL_WaitEvent(&event))
    {
      switch(event.type)
        {
	case SDL_KEYDOWN:
	  // We can still quit :)
	  if(event.key.keysym.sym == dgen_quit) return 0;
	  else return 1;
	case SDL_QUIT:
	  return 0;
	default: break;
	}
    }
  // SDL_WaitEvent only returns zero on error :(
  fprintf(stderr, "sdl: SDL_WaitEvent broke: %s!", SDL_GetError());
  return 1;
}

// The massive event handler!
// I know this is an ugly beast, but please don't be discouraged. If you need
// help, don't be afraid to ask me how something works. Basically, just handle
// all the event keys, or even ignore a few if they don't make sense for your
// interface.
int pd_handle_events(md &megad)
{
  SDL_Event event;
  long ksym;

  // If there's any chance your implementation might run under Linux, add these
  // next four lines for joystick handling.
#ifdef LINUX_JOYSTICK_SUPPORT
  if(dgen_joystick)
    megad.read_joysticks();
#endif
  // Check key events
  while(SDL_PollEvent(&event))
    {
      switch(event.type)
	{
#if SDL_JOYSTICK_SUPPORT
       case SDL_JOYAXISMOTION:
         // x-axis
         if(event.jaxis.axis == 0)
           {
             if(event.jaxis.value < -16384)
               {
                 megad.pad[event.jaxis.which] &= ~0x04;
                 megad.pad[event.jaxis.which] |=  0x08;
                 break;
               }
             if(event.jaxis.value > 16384)
               {
                 megad.pad[event.jaxis.which] |=  0x04;
                 megad.pad[event.jaxis.which] &= ~0x08;
                 break;
               }
             megad.pad[event.jaxis.which] |= 0xC;
             break;
           }
         // y-axis
         if(event.jaxis.axis == 1)
           {
             if(event.jaxis.value < -16384)
               {
                 megad.pad[event.jaxis.which] &= ~0x01;
                 megad.pad[event.jaxis.which] |=  0x02;
                 break;
               }
             if(event.jaxis.value > 16384)
               {
                 megad.pad[event.jaxis.which] |=  0x01;
                 megad.pad[event.jaxis.which] &= ~0x02;
                 break;
               }
             megad.pad[event.jaxis.which] |= 0x3;
             break;
           }
         break;
       case SDL_JOYBUTTONDOWN:
         // Ignore more than 16 buttons (a reasonable limit :)
         if(event.jbutton.button > 15) break;
         megad.pad[event.jbutton.which] &= ~js_map_button[event.jbutton.which]
                                                         [event.jbutton.button];
         break;
       case SDL_JOYBUTTONUP:
         // Ignore more than 16 buttons (a reasonable limit :)
         if(event.jbutton.button > 15) break;
         megad.pad[event.jbutton.which] |= js_map_button[event.jbutton.which]
                                                        [event.jbutton.button];
         break;
#endif // SDL_JOYSTICK_SUPPORT
	case SDL_KEYDOWN:
	  ksym = event.key.keysym.sym;
	  // Check for modifiers
	  if(event.key.keysym.mod & KMOD_SHIFT) ksym |= KEYSYM_MOD_SHIFT;
	  if(event.key.keysym.mod & KMOD_CTRL) ksym |= KEYSYM_MOD_CTRL;
	  if(event.key.keysym.mod & KMOD_ALT) ksym |= KEYSYM_MOD_ALT;
	  if(event.key.keysym.mod & KMOD_META) ksym |= KEYSYM_MOD_META;
	  // Check if it was a significant key that was pressed
	  if(ksym == pad1_up) megad.pad[0] &= ~0x01;
	  else if(ksym == pad1_down) megad.pad[0] &= ~0x02;
	  else if(ksym == pad1_left) megad.pad[0] &= ~0x04;
	  else if(ksym == pad1_right) megad.pad[0] &= ~0x08;
	  else if(ksym == pad1_a) megad.pad[0] &= ~0x1000;
	  else if(ksym == pad1_b) megad.pad[0] &= ~0x10;
	  else if(ksym == pad1_c) megad.pad[0] &= ~0x20;
	  else if(ksym == pad1_x) megad.pad[0] &= ~0x40000;
	  else if(ksym == pad1_y) megad.pad[0] &= ~0x20000;
	  else if(ksym == pad1_z) megad.pad[0] &= ~0x10000;
	  else if(ksym == pad1_mode) megad.pad[0] &= ~0x80000;
	  else if(ksym == pad1_start) megad.pad[0] &= ~0x2000;

	  else if(ksym == pad2_up) megad.pad[1] &= ~0x01;
	  else if(ksym == pad2_down) megad.pad[1] &= ~0x02;
	  else if(ksym == pad2_left) megad.pad[1] &= ~0x04;
	  else if(ksym == pad2_right) megad.pad[1] &= ~0x08;
	  else if(ksym == pad2_a) megad.pad[1] &= ~0x1000;
	  else if(ksym == pad2_b) megad.pad[1] &= ~0x10;
	  else if(ksym == pad2_c) megad.pad[1] &= ~0x20;
	  else if(ksym == pad2_x) megad.pad[1] &= ~0x40000;
	  else if(ksym == pad2_y) megad.pad[1] &= ~0x20000;
	  else if(ksym == pad2_z) megad.pad[1] &= ~0x10000;
	  else if(ksym == pad2_mode) megad.pad[1] &= ~0x80000;
	  else if(ksym == pad2_start) megad.pad[1] &= ~0x2000;

	  else if(ksym == dgen_quit) return 0;
// Split screen is unnecessary with new renderer.
//	  else if(ksym == dgen_splitscreen_toggle)
//	    {
//	      split_screen = !split_screen;
//	      pd_message(split_screen? "Split screen enabled." : 
//				       "Split screen disabled.");
//	    }
	  else if(ksym == dgen_craptv_toggle)
	    {
		  dgen_craptv = ((++dgen_craptv) % NUM_CTV);
		  sprintf(temp, "Crap TV mode \"%s\".", ctv_names[dgen_craptv]);
		  pd_message(temp);
	    }
	  else if(ksym == dgen_reset)
	    { megad.reset(); pd_message("Genesis reset."); }
	  else if(ksym == dgen_slot_0)
	    { slot = 0; pd_message("Selected save slot 0."); }
	  else if(ksym == dgen_slot_1)
	    { slot = 1; pd_message("Selected save slot 1."); }
	  else if(ksym == dgen_slot_2)
	    { slot = 2; pd_message("Selected save slot 2."); }
	  else if(ksym == dgen_slot_3)
	    { slot = 3; pd_message("Selected save slot 3."); }
	  else if(ksym == dgen_slot_4)
	    { slot = 4; pd_message("Selected save slot 4."); }
	  else if(ksym == dgen_slot_5)
	    { slot = 5; pd_message("Selected save slot 5."); }
	  else if(ksym == dgen_slot_6)
	    { slot = 6; pd_message("Selected save slot 6."); }
	  else if(ksym == dgen_slot_7)
	    { slot = 7; pd_message("Selected save slot 7."); }
	  else if(ksym == dgen_slot_8)
	    { slot = 8; pd_message("Selected save slot 8."); }
	  else if(ksym == dgen_slot_9)
	    { slot = 9; pd_message("Selected save slot 9."); }
	  else if(ksym == dgen_save) md_save(megad);
	  else if(ksym == dgen_load) md_load(megad);
// Added this CPU core hot swap.  Compile both Musashi and StarScream
// in, and swap on the fly like DirectX DGen. [PKH]
#if defined (COMPILE_WITH_MUSA) && (COMPILE_WITH_STAR)
	  else if(ksym == dgen_cpu_toggle)
	    {
	      if(megad.cpu_emu) {
	        megad.change_cpu_emu(0);
  	        pd_message("StarScream CPU core activated.");
              }
	      else {
	        megad.change_cpu_emu(1);
	        pd_message("Musashi CPU core activated."); 
              }
	    }
#endif
	  else if(ksym == dgen_stop) {
	    pd_message("* STOPPED * Press any key to continue.");
	    SDL_PauseAudio(1); // Stop audio :)
#ifdef HAVE_SDL_WM_TOGGLEFULLSCREEN
	    int fullscreen = 0;
	    // Switch out of fullscreen mode (assuming this is supported)
	    if(screen->flags & SDL_FULLSCREEN) {
	      fullscreen = 1;
	      SDL_WM_ToggleFullScreen(screen);
	    }
#endif
	    int r = stop_events(megad);
#ifdef HAVE_SDL_WM_TOGGLEFULLSCREEN
	    if(fullscreen)
	      SDL_WM_ToggleFullScreen(screen);
#endif
	    pd_clear_message();
	    if(r) SDL_PauseAudio(0); // Restart audio
	    return r;
	  }
#ifdef HAVE_SDL_WM_TOGGLEFULLSCREEN
	  else if(ksym == dgen_fullscreen_toggle) {
	    SDL_WM_ToggleFullScreen(screen);
	    pd_message("Fullscreen mode toggled.");
	  }
#endif
	  else if(ksym == dgen_fix_checksum) {
	    pd_message("Checksum fixed.");
	    megad.fix_rom_checksum();
	  }
          else if(ksym == dgen_screenshot) {
            do_screenshot();
          }
	  break;
	case SDL_KEYUP:
	  ksym = event.key.keysym.sym;
	  // Check for modifiers
	  if(event.key.keysym.mod & KMOD_SHIFT) ksym |= KEYSYM_MOD_SHIFT;
	  if(event.key.keysym.mod & KMOD_CTRL) ksym |= KEYSYM_MOD_CTRL;
	  if(event.key.keysym.mod & KMOD_ALT) ksym |= KEYSYM_MOD_ALT;
	  if(event.key.keysym.mod & KMOD_META) ksym |= KEYSYM_MOD_META;
	  // The only time we care about key releases is for the controls
	  if(ksym == pad1_up) megad.pad[0] |= 0x01;
	  else if(ksym == pad1_down) megad.pad[0] |= 0x02;
	  else if(ksym == pad1_left) megad.pad[0] |= 0x04;
	  else if(ksym == pad1_right) megad.pad[0] |= 0x08;
	  else if(ksym == pad1_a) megad.pad[0] |= 0x1000;
	  else if(ksym == pad1_b) megad.pad[0] |= 0x10;
	  else if(ksym == pad1_c) megad.pad[0] |= 0x20;
	  else if(ksym == pad1_x) megad.pad[0] |= 0x40000;
	  else if(ksym == pad1_y) megad.pad[0] |= 0x20000;
	  else if(ksym == pad1_z) megad.pad[0] |= 0x10000;
	  else if(ksym == pad1_mode) megad.pad[0] |= 0x80000;
	  else if(ksym == pad1_start) megad.pad[0] |= 0x2000;

	  else if(ksym == pad2_up) megad.pad[1] |= 0x01;
	  else if(ksym == pad2_down) megad.pad[1] |= 0x02;
	  else if(ksym == pad2_left) megad.pad[1] |= 0x04;
	  else if(ksym == pad2_right) megad.pad[1] |= 0x08;
	  else if(ksym == pad2_a) megad.pad[1] |= 0x1000;
	  else if(ksym == pad2_b) megad.pad[1] |= 0x10;
	  else if(ksym == pad2_c) megad.pad[1] |= 0x20;
	  else if(ksym == pad2_x) megad.pad[1] |= 0x40000;
	  else if(ksym == pad2_y) megad.pad[1] |= 0x20000;
	  else if(ksym == pad2_z) megad.pad[1] |= 0x10000;
	  else if(ksym == pad2_mode) megad.pad[1] |= 0x80000;
	  else if(ksym == pad2_start) megad.pad[1] |= 0x2000;
	  break;
	case SDL_QUIT:
	  // We've been politely asked to exit, so let's leave
	  return 0;
	default:
	  break;
	}
    }
  return 1;
}

#ifdef SDL_OPENGL_SUPPORT
static void ogl_write_text(const char *msg)
{
  int j, k, x;
  unsigned char c, *p;
  const char *q;
  char draw;

  for(q = msg, x = 0, draw = 1;
      *q;
      ++q, x += 7, draw = 1)
    {
      switch(*q)
        {
	case 'A': case 'a': p = font_a; break;
	case 'B': case 'b': p = font_b; break;
	case 'C': case 'c': p = font_c; break;
	case 'D': case 'd': p = font_d; break;
	case 'E': case 'e': p = font_e; break;
	case 'F': case 'f': p = font_f; break;
	case 'G': case 'g': p = font_g; break;
	case 'H': case 'h': p = font_h; break;
	case 'I': case 'i': p = font_i; break;
	case 'J': case 'j': p = font_j; break;
	case 'K': case 'k': p = font_k; break;
	case 'L': case 'l': p = font_l; break;
	case 'M': case 'm': p = font_m; break;
	case 'N': case 'n': p = font_n; break;
	case 'O': case 'o': p = font_o; break;
	case 'P': case 'p': p = font_p; break;
	case 'Q': case 'q': p = font_q; break;
	case 'R': case 'r': p = font_r; break;
	case 'S': case 's': p = font_s; break;
	case 'T': case 't': p = font_t; break;
	case 'U': case 'u': p = font_u; break;
	case 'V': case 'v': p = font_v; break;
	case 'W': case 'w': p = font_w; break;
	case 'X': case 'x': p = font_x; break;
	case 'Y': case 'y': p = font_y; break;
	case 'Z': case 'z': p = font_z; break;
	case '0':	    p = font_0; break;
	case '1':	    p = font_1; break;
	case '2':	    p = font_2; break;
	case '3':	    p = font_3; break;
	case '4':	    p = font_4; break;
	case '5':	    p = font_5; break;
	case '6':	    p = font_6; break;
	case '7':	    p = font_7; break;
	case '8':	    p = font_0; break;
	case '9':	    p = font_0; break;
	case '*':	    p = font_ast; break;
	case '!':	    p = font_ex; break;
	case '.':	    p = font_per; break;
	case '"':	    p = font_quot; break;
	case '\'':	    p = font_apos; break;
	case ' ':	    draw = 0; break;
	default:	    draw = 0; break;
        }
      if(draw)
        for(j = 0; j < 5; ++j)
	  for(k = 0; k < 5; ++k)
	    {
	      if(p[k+j*5]) c = 255; else c = 0;
	      message[j][k+x][R] = message[j][k+x][G] = message[j][k+x][B] = c;
	      message[j][k+x][A] = 255;
	    }
    }
}
#endif // SDL_OPENGL_SUPPORT

// Write a message to the status bar
void pd_message(const char *msg)
{
  pd_clear_message();
#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    {
      ogl_write_text(msg);
      glBindTexture(GL_TEXTURE_2D, texture[0]);
      glTexSubImage2D(GL_TEXTURE_2D, 0,0,ys,256,5,GL_RGBA,GL_UNSIGNED_BYTE,message);
    }
  else
    {
#endif
      font_text(screen, 0, ys, msg);
      SDL_UpdateRect(screen, 0, ys, xs, 16);
#ifdef SDL_OPENGL_SUPPORT
    }
#endif
  // Clear message in 3 seconds
  alarm(MESSAGE_LIFE);
}

inline void pd_clear_message()
{
  int i, j;
  long *p = (long*)((char*)screen->pixels + (screen->pitch * ys));
#ifdef SDL_OPENGL_SUPPORT
  if(opengl)
    {
      memset(message,0,256*5*4);
      glBindTexture(GL_TEXTURE_2D, texture[0]);
      glTexSubImage2D(GL_TEXTURE_2D,0,0,ys,256,5,GL_RGBA,GL_UNSIGNED_BYTE,m_clear);
    }
  else
    {
#endif
      for(i = 0; i < 16; ++i, p += (screen->pitch >> 2))
        for(j = 0; j < 80 * screen->format->BytesPerPixel; ++j)
          p[j] = 0;
      SDL_UpdateRect(screen, 0, ys, xs, 16);
#ifdef SDL_OPENGL_SUPPORT
    }
#endif
}

/* FIXME: Implement this
 * Look at v1.16 to see how I did carthead there */
void pd_show_carthead(md&)
{
}

/* Clean up this awful mess :) */
void pd_quit()
{
  if(mdscr.data) free((void*)mdscr.data);
  if(playbuf)
    {
      SDL_CloseAudio();
      free((void*)playbuf);
    }
  if(sndi.l) free((void*)sndi.l);
  if(sndi.r) free((void*)sndi.r);
  if(mdpal) free((void*)mdpal);
}
