// DGen/SDL 1.17
// by Joe Groff <joe@pknet.com>
// Read LICENSE for copyright etc., but if you've seen one BSDish license,
// you've seen them all ;)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>

#define IS_MAIN_CPP
#include "md.h"
#include "pd.h"
#include "pd-defs.h"
#include "rc.h"
#include "rc-vars.h"

#ifdef __BEOS__
#include <OS.h>
#endif

// Ideal usec/frame for 60Hz
#define USEC_FRAME_NTSC 16667 // 1000000/60

// Ideal usec/frame for 50Hz
#define USEC_FRAME_PAL 20000 // 1000000/50

// Neat little macro to pick which one of the above :)
#define USEC_FRAME (pal_mode? USEC_FRAME_PAL : USEC_FRAME_NTSC)

// Defined in ras.cpp, and set to true if the Genesis palette's changed.
extern int pal_dirty;

int sound_is_okay = 0;
FILE *debug_log = NULL;

// Do a demo frame, if active
#define DO_DEMO \
	if(demo_record) { \
	  foo = htonl(megad.pad[0]); \
	  fwrite(&foo, sizeof(foo), 1, demo); \
	  foo = htonl(megad.pad[1]); \
	  fwrite(&foo, sizeof(foo), 1, demo); \
	} if(demo_play) { \
	  fread(&foo, sizeof(foo), 1, demo); \
	  megad.pad[0] = ntohl(foo); \
	  fread(&foo, sizeof(foo), 1, demo); \
	  megad.pad[1] = ntohl(foo); \
	  if(feof(demo)) \
	    { \
	      pd_message("Demo finished."); \
	      demo_play = 0; \
	    } \
	}

// Convenience, so I don't have to type this constantly
#define DO_FRAME(scr, pal) \
	running = pd_handle_events(megad); \
	DO_DEMO \
	if (dgen_sound) { \
	  megad.one_frame((scr), (pal), &sndi); \
	} else megad.one_frame((scr), (pal), NULL);

// Directory to put savestates in
static char saves[2048] = "";

// Directory to put battery RAM in
static char ramdir[2048] = "";

// Temporary garbage can string :)
static char temp[65536] = "";

// Get the basename from the ROM filename
// An equivalent perl one-liner would be perl -pe 's@.*/([^.]*?)\..*@\1@' :)
/* Modified: 20-11-1999 Dylan_G@bigfoot.com
	Made very much less evil. */
static char *gst_name(char *fn)
{
    char buf[1024]; /* strtok modifies its arguments */
    char *p = NULL, *p1 = NULL;

    memset(buf, 0, sizeof(buf));
    strncpy(buf, fn, sizeof(buf));
    if(strchr(buf, '.'))
    { /* Need to strip extension */
    	p = strtok(buf, ".");
    }
    
    if(strchr(buf, '/'))
    { /* Need to strip /path/name */
        p = strtok(buf, "/");
	/* We have to walk through until we hit NULL, then use N-1 pointer :-). */
	while(p != NULL)
    {
	    p1 = p;
	    p = strtok(NULL, "/");
	}
    }
    /* Fix in case there is no / in the filename */
    if(p) p1 = p;
    return(p1);
}

// Show help and exit with code 2
static void help()
{
  printf(
  "DGen/SDL v"VER"\n"
  "Usage: dgen [options] romname\n\n"
  "Where options are:\n"
  "    -v              Print version number and exit.\n"
  "    -r RCFILE       Read in the file RCFILE after parsing\n"
  "                    $HOME/.dgen/dgenrc.\n"
  "    -n USEC         Cuses dgen to sleep USEC microseconds per frame, to be\n"
  "                    nice to other processes.\n"
  "    -p CODE,CODE... Takes a comma-delimited list of Game Genie (ABCD-EFGH)\n"
  "                    or Hex (123456:ABCD) codes to patch the ROM with.\n"
  "    -R              Set realtime priority -20, so no other processes may\n"
  "                    interrupt. dgen definitely needs root priviledges for\n"
  "                    this.\n"
  "    -P              Use PAL mode (50Hz) instead of normal NTSC (60Hz).\n"
  "    -d DEMONAME     Record a demo of the game you are playing.\n"
  "    -D DEMONAME     Play back a previously recorded demo.\n"
  "    -s SLOT         Load the saved state from the given slot at startup.\n"
#ifdef JOYSTICK_SUPPORT
  "    -j              Use joystick if detected.\n"
#endif
  );
  // Display platform-specific options
  pd_help();
  exit(2);
}

// Create the .dgen directory structure in the user's home directory
static void mk_dgendir()
{
  strcpy(temp, getenv("HOME"));
  strcat(temp, "/.dgen");
  mkdir(temp, 0777);
  // Make save dir
  strcpy(saves, temp);
  strcat(saves, "/saves");
  mkdir(saves, 0777);
  // Make ram dir
  strcpy(ramdir, temp);
  strcat(ramdir, "/ram");
  mkdir(ramdir, 0777);
}

// Save/load states
// It is externed from your implementation to change the current slot
// (I know this is a hack :)
int slot = 0;
void md_save(md& megad)
{
  FILE *save = NULL;
  sprintf(temp, "%s/%s.gs%d", saves, gst_name(megad.romfilename), slot);
  if((save = fopen(temp, "wb")))
    {
      megad.export_gst(save);
      fclose(save);
      sprintf(temp, "Saved state to slot %d.", slot);
      pd_message(temp);
    }
  else 
    {
      sprintf(temp, "Couldn't save state to slot %d!", slot);
      pd_message(temp);
    }
}

void md_load(md& megad)
{
  FILE *load = NULL;
  sprintf(temp, "%s/%s.gs%d", saves, gst_name(megad.romfilename), slot);
  if((load = fopen(temp, "rb")))
    {
      megad.import_gst(load);
      fclose(load);
      sprintf(temp, "Loaded state from slot %d.", slot);
      pd_message(temp);
    }
  else
    {
      sprintf(temp, "Couldn't load state from slot %d!", slot);
      pd_message(temp);
    }
}
 
// Load/save states from file
static void ram_save(md& megad)
{
  FILE *save = NULL;
  if(!megad.has_save_ram()) return;
  sprintf(temp, "%s/%s", ramdir, gst_name(megad.romfilename));
  if((save = fopen(temp, "wb")))
    {
      megad.put_save_ram(save);
      fclose(save);
    }
  else
    {
      fprintf(stderr, "Couldn't save battery RAM to %s!\n", temp);
    }
}

static void ram_load(md& megad)
{
  FILE *load = NULL;
  if(!megad.has_save_ram()) return;
  sprintf(temp, "%s/%s", ramdir, gst_name(megad.romfilename));
  if((load = fopen(temp, "rb")))
    {
      megad.get_save_ram(load);
      fclose(load);
    }
}

int main(int argc, char *argv[])
{
  int c = 0, pal_mode = 0, running = 1, usec = 0,
      wp = 0, rp = 0, start_slot = -1;
  unsigned long long f = 0;
  char *patches = NULL, *rom = NULL;
  struct timeval oldclk, newclk, startclk, endclk;
  FILE *demo = NULL;
  int demo_record = 0, demo_play = 0, foo;

  // Parse the RC file
  parse_rc(NULL);

  // Check all our options
  strcpy(temp, "s:hvr:n:p:RPjd:D:");
  strcat(temp, pd_options);
  while((c = getopt(argc, argv, temp)) != EOF)
    {
      switch(c)
	{
	case 'v':
	  // Show version and exit
	  printf("DGen/SDL version "VER"\n");
	  return 0;
	case 'r':
	  // Parse another RC file
	  parse_rc(optarg);
	  break;
	case 'n':
	  // Sleep for n microseconds
	  dgen_nice = atoi(optarg);
	  break;
	case 'p':
	  // Game Genie patches
	  patches = optarg;
	  break;
#ifndef __BEOS__
	case 'R':
	  // Try to set realtime priority
	  if(geteuid()) {
	    fprintf(stderr, "main: Only root can set lower priorities!\n");
	    break;
	  }
	  if(setpriority(PRIO_PROCESS, 0, -20) == -1)
	    perror("main: setpriority");
	  break;
#endif
	case 'P':
	  // PAL mode
	  pal_mode = 1;
	  break;
#ifdef JOYSTICK_SUPPORT
	case 'j':
	  // Phil's joystick code
	  dgen_joystick = 1;
	  break;
#endif
	case 'd':
	  // Record demo
	  if(demo)
	    {
	      fprintf(stderr,"main: Can't record and play at the same time!\n");
	      break;
	    }
	  if(!(demo = fopen(optarg, "wb")))
	    {
	      fprintf(stderr, "main: Can't record demo file %s!\n", optarg);
	      break;
	    }
	  demo_record = 1;
	  break;
	case 'D':
	  // Play demo
	  if(demo)
	    {
	      fprintf(stderr,"main: Can't record and play at the same time!\n");
	      break;
	    }
	  if(!(demo = fopen(optarg, "rb")))
	    {
	      fprintf(stderr, "main: Can't play demo file %s!\n", optarg);
	      break;
	    }
	  demo_play = 1;
	  break;
	case '?': // Bad option!
	case 'h': // A cry for help :)
	  help();
        case 's':
          // Pick a savestate to autoload
          start_slot = atoi(optarg);
          break;
	default:
	  // Pass it on to platform-dependent stuff
	  pd_option(c, optarg);
	  break;
	}
    }

#ifdef __BEOS__
  // BeOS snooze() sleeps in milliseconds, not microseconds
  dgen_nice /= 1000;
#endif
  
  // There should be a romname after all those options. If not, show help and
  // exit.
  if(optind >= argc)
    help();

  // Initialize the platform-dependent stuff.
  if(!pd_graphics_init(dgen_sound, pal_mode))
    {
      fprintf(stderr, "main: Couldn't initialize graphics!\n");
      return 1;
    }
  if(dgen_sound)
    {
      dgen_16bit = dgen_16bit? PD_SND_16 : PD_SND_8;
      dgen_sound = pd_sound_init(dgen_16bit, dgen_soundrate, dgen_soundsegs);
    }
  // If sound fared OK, start up the sound chips
  if(dgen_sound)
    {
      if(YM2612Init(1, 7520000L, dgen_soundrate, NULL, NULL) ||
	 SN76496_init(0, 3478000L, dgen_soundrate, 16))
	fprintf(stderr, "main: Couldn't start sound chipset emulators!\n");
      else
	sound_is_okay = 1;
    }
  // Decrement the sound seg count. This makes it a nice AND mask :)
  --dgen_soundsegs;

  rom = argv[optind];

  // Create the megadrive object
  md megad;
  if(!megad.okay())
    {
      fprintf(stderr, "main: Megadrive init failed!\n");
      return 1;
    }
  // Load the requested ROM
  if(megad.load(rom))
    {
      fprintf(stderr, "main: Couldn't load ROM file %s!\n", rom);
      return 1;
    }
  // Set untouched pads
  megad.pad[0] = megad.pad[1] = 0xF303F;
#ifdef JOYSTICK_SUPPORT
  if(dgen_joystick)
    megad.init_joysticks();
#endif
  // Load patches, if given
  if(patches)
    {
      printf("main: Using patch codes %s\n", patches);
      megad.patch(patches);
    }
  // Fix checksum
  megad.fix_rom_checksum();
  // Reset
  megad.reset();
  // Set PAL mode
  megad.pal = pal_mode;
  
  // Make sure the .dgen hierarchy is setup
  mk_dgendir();
  // Load up save RAM
  ram_load(megad);
  // If autoload is on, load save state 0
  if(dgen_autoload)
    {
      slot = 0;
      md_load(megad);
    }
  // If -s option was given, load the requested slot
  if(start_slot >= 0)
    {
      slot = start_slot;
      md_load(megad);
    }

  // Start the timing refs
  gettimeofday(&oldclk, NULL);
  gettimeofday(&startclk, NULL);
  // Start audio
  if(dgen_sound) pd_sound_start();

  // Show cartridge header
  if(dgen_show_carthead) pd_show_carthead(megad);

  // Go around, and around, and around, and around... ;)
  while(running)
    {
      int frames_todo;
      frames_todo = 1;

      // Measure how many frames to do this round
      if(!dgen_sound && dgen_frameskip)
        {
	  gettimeofday(&newclk, NULL);
	  if(newclk.tv_usec < oldclk.tv_usec)
	    usec += 1000000 + newclk.tv_usec - oldclk.tv_usec;
	  else
	    usec += newclk.tv_usec - oldclk.tv_usec;
	  frames_todo = usec / USEC_FRAME;
	  usec %= USEC_FRAME;
	  oldclk = newclk;
	  // We don't want to skip too many frames - this isn't Unreal ;)
	  if(frames_todo > 8) frames_todo = 8;
	  // Skip these frames
	  for(;frames_todo > 1; --frames_todo)
	    {
	      DO_DEMO
	      megad.one_frame(NULL, NULL, NULL);
	    }
	} else if(dgen_sound) {
	  // We can use the sound buffer for timing, instead of the above loop
	  // If we are already caught up, wait for the read pointer to advance
	  while((rp = pd_sound_rp()) == wp);
	  while(wp != rp)
	    {
	      pd_sound_write(wp);
	      ++wp; wp &= dgen_soundsegs;
	      // Skip a frame to keep the sound going, until we hit the read
	      // point.
	      if(wp != rp)
	        {
		  DO_DEMO
	          megad.one_frame(NULL, NULL, &sndi);
		}
	    }
	}
      // If there are frames to do, do them! :)
      if(frames_todo)
        {
	  DO_FRAME(&mdscr, mdpal);
	  // Update palette
	  if(mdpal && pal_dirty)
	    {
	      pd_graphics_palette_update();
	      pal_dirty = 0;
	    }
	}
      // Update screen
      pd_graphics_update();
      ++f;

      // Sleep a bit
#ifdef __BEOS__
      if(dgen_nice) snooze(dgen_nice);
#else
      if(dgen_nice) usleep(dgen_nice);
#endif
    }
  // Print fps
  gettimeofday(&endclk, NULL);
  printf("%d frames per second (optimal %d)\n",
	 (unsigned)(f / (endclk.tv_sec - startclk.tv_sec)), (pal_mode? 50 : 60));
  
  // Cleanup
  if(demo) fclose(demo);
  ram_save(megad);
  if(dgen_autosave) { slot = 0; md_save(megad); }
  megad.unplug();
  pd_quit();
  YM2612Shutdown();

  // Come back anytime :)
  return 0; 
}
