// DGen/SDL v1.15+

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md.h"
#include "decode.h"

// These are my (Joe's) functions added to the md class.

// This takes a comma or whitespace-separated list of Game Genie and/or hex 
// codes to patch the ROM with.
void md::patch(const char *list)
{
  static const char delims[] = " \t\n,";
  char *worklist, *tok;
  struct patch p;
  
  // Copy the given list to a working list so we can strtok it
  worklist = (char*)malloc(strlen(list)+1);
  strcpy(worklist, list);

  for(tok = strtok(worklist, delims); tok; tok = strtok(NULL, delims))
    {
      // If it's empty, toss it
      if(*tok == '\0') continue;
      // Decode it
      decode(tok, &p);
      // Discard it if it was bad code
      if((signed)p.addr == -1) {
	printf("Bad patch \"%s\"\n", tok);
	continue;
      }
      // Put it into the ROM (remember byteswapping)
      printf("Patch \"%s\" -> %06X:%04X\n", tok, p.addr, p.data);
      rom[p.addr] = (char)(p.data & 0xFF);
      rom[p.addr+1] = (char)((p.data & 0xFF00) >> 8);
    }
  // Done!
  free(worklist);
  return;
}

// Get/put saveram from/to FILE*'s
void md::get_save_ram(FILE *from)
{
  // Pretty simple, just read the saveram raw
  fread((void*)saveram, 1, save_len, from);
}

void md::put_save_ram(FILE *into)
{
  // Just the opposite of the above :)
  fwrite((void*)saveram, 1, save_len, into);
}

// Dave: This is my code, but I thought it belonged here
// Joe: Thanks Dave! No problem ;)
static unsigned short calculate_checksum(unsigned char *rom,int len)
{
  unsigned short checksum=0;
  int i;
  for (i=512;i<=(len-2);i+=2)
  {
    checksum+=(rom[i+1]<<8);
    checksum+=rom[i+0];
  }
  return checksum;
}

void md::fix_rom_checksum()
{
  unsigned short cs; cs=calculate_checksum(rom,romlen);
  if (romlen>=0x190) { rom[0x18f]=cs>>8; rom[0x18e]=cs&255; }
}

