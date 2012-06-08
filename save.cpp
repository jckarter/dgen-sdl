// DGen v1.10+
// Megadrive C++ module saving and loading

#include <stdio.h>
#include <string.h>
#include "md.h"

/*
gs0 genecyst save file INfo

GST\0 to start with

80-9f = d0-d7 almost certain
a0-bf = a0-a7 almost certain
c8    = pc    fairly certain
d0    = sr    fairly certain


112 Start of cram len 0x80
192 Start of vsram len 0x50
1e2-474 UNKNOWN sound info?
Start of z80 ram at 474 (they store 2000)
Start of RAM at 2478 almost certain (BYTE SWAPPED)
Start of VRAM at 12478
end of VRAM
*/

// NB - for load and save you don't need to use star_mz80_on/off
// Because stars/mz80 isn't actually used
#define fput(x,y) { if (fwrite(x,1,y,hand)!=y) goto save_error; }
#define fget(x,y) if (fread(x,1,y,hand)!=y) goto load_error;

extern int byteswap_memory(unsigned char *start,int len);

int md::import_gst(FILE *hand)
{
#ifdef COMPILE_WITH_STAR
  if (cpu_emu==0)
  {
    fseek(hand,0x80,SEEK_SET);
    fget(cpu.dreg,8*4);
    fget(cpu.areg,8*4);
    fseek(hand,0xc8,SEEK_SET);
    fget(&cpu.pc,4);
    fseek(hand,0xd0,SEEK_SET);
    fget(&cpu.sr,4);
  }
#endif

#ifdef COMPILE_WITH_MUSA
  if (cpu_emu==1)
  {
    int i,t;
    fseek(hand,0x80,SEEK_SET);
    for (i=0;i<8;i++)
    { fget(&t,4); m68k_poke_dr(i,t); }
    for (i=0;i<8;i++)
    { fget(&t,4); m68k_poke_ar(i,t); }

    fseek(hand,0xc8,SEEK_SET);
    fget(&t,4); m68k_poke_pc(t);

    fseek(hand,0xd0,SEEK_SET);
    fget(&t,4); m68k_poke_sr(t);
  }
#endif

  fseek(hand,0xfa,SEEK_SET);
  fget(vdp.reg,0x18);

  fseek(hand,0x112,SEEK_SET);
  fget(vdp.cram ,0x00080);
  byteswap_memory(vdp.cram,0x80);
  fget(vdp.vsram,0x00050);
  byteswap_memory(vdp.vsram,0x50);

  fseek(hand,0x474,SEEK_SET);
  fget(z80ram,   0x02000);

  fseek(hand,0x2478,SEEK_SET);
  fget(ram,      0x10000);
  byteswap_memory(ram,0x10000);

  fget(vdp.vram ,0x10000);

  memset(vdp.dirt,0xff,0x35); // mark everything as changed

  return 0;
load_error:
  return 1;
}


int md::export_gst(FILE *hand)
{
  int i;
  static unsigned char gst_head[0x80]=
  {
       0x47,0x53,0x54,0,0,0,0xe0,0x40

  //     00 00 00 00 00 00 00 00 00 00 00 00 00 21 80 fa   <.............!..>
  };
  unsigned char *zeros=gst_head+0x40; // 0x40 zeros

  fseek(hand,0x00,SEEK_SET);
  // Make file size 0x22478 with zeros
  for (i=0;i<0x22440;i+=0x40) fput(zeros,0x40);
  fput(zeros,0x38);

  fseek(hand,0x00,SEEK_SET);
  fput(gst_head,0x80);

#ifdef COMPILE_WITH_STAR
  if (cpu_emu==0)
  {
    fseek(hand,0x80,SEEK_SET);
    fput(cpu.dreg,8*4);
    fput(cpu.areg,8*4);
    fseek(hand,0xc8,SEEK_SET);
    fput(&cpu.pc,4);
    fseek(hand,0xd0,SEEK_SET);
    fput(&cpu.sr,4);
  }
#endif
#ifdef COMPILE_WITH_MUSA
  if (cpu_emu==1)
  {
    int i,t;
    fseek(hand,0x80,SEEK_SET);
    for (i=0;i<8;i++)
    { t=m68k_peek_dr(i); fput(&t,4);}
    for (i=0;i<8;i++)
    { t=m68k_peek_ar(i); fput(&t,4);}

    fseek(hand,0xc8,SEEK_SET);
    t=m68k_peek_pc(); fput(&t,4);

    fseek(hand,0xd0,SEEK_SET);
    t=m68k_peek_sr(); fput(&t,4);
  }
#endif

  fseek(hand,0xfa,SEEK_SET);
  fput(vdp.reg,0x18);

  fseek(hand,0x112,SEEK_SET);
  byteswap_memory(vdp.cram,0x80);
  fput(vdp.cram ,0x00080);
  byteswap_memory(vdp.cram,0x80);
  byteswap_memory(vdp.vsram,0x50);
  fput(vdp.vsram,0x00050);
  byteswap_memory(vdp.vsram,0x50);

  fseek(hand,0x474,SEEK_SET);
  fput(z80ram,   0x02000);

  fseek(hand,0x2478,SEEK_SET);
  byteswap_memory(ram,0x10000);
  fput(ram,      0x10000);
  byteswap_memory(ram,0x10000);

  fput(vdp.vram ,0x10000);

  return 0;
save_error:
  return 1;
}

