// DGen/SDL v1.17+
// Megadrive C++ module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md.h"

// This is the 'static' StarScream/MZ80 multitasker
// which detects which megadrive is active (by which star_mz80_on() has been called
// and forwards to the appropriate misc_read/writebyte/word/z80 function
// of the appropriate instance (grrrr - bloody C++)

static md* which=0;

int md::star_mz80_on()
{
#ifdef COMPILE_WITH_STAR
  s68000SetContext(&cpu);
#endif
  mz80SetContext(&z80);
  which=this;
  return 0;
}
int md::star_mz80_off()
{
  which=0;
#ifdef COMPILE_WITH_STAR
  s68000GetContext(&cpu);
#endif
  mz80GetContext(&z80);
  return 0;
}

extern "C"
{
  unsigned root_readbyte(unsigned a)
  { if (which) return which->misc_readbyte(a); else return 0x00; }
  unsigned root_readword(unsigned a)
  { if (which) return which->misc_readword(a); else return 0x0000; }
  
  unsigned root_readlong(unsigned a)
  {
    return (root_readword(a)<<16)+root_readword(a+2);
  }
  
  void root_writebyte(unsigned a,unsigned d)
  { if (which) which->misc_writebyte(a,d); }
  void root_writeword(unsigned a,unsigned d)
  { if (which) which->misc_writeword(a,d); }
  
  void root_writelong(unsigned a,unsigned d)
  {
    root_writeword(a,(d>>16)&0xffff);
    root_writeword(a+2,d&0xffff);
  }
}

#ifdef COMPILE_WITH_MUSA
// Okay: a bit for MUSASHI
extern "C"
{
  // read/write functions called by the CPU to access memory.
  // while values used are 32 bits, only the appropriate number
  // of bits are relevant (i.e. in write_memory_8, only the lower 8 bits
  // of value should be written to memory).
  // address will be a 24-bit value.
  
  /* Read from anywhere */
  int  m68k_read_memory_8(int address)  { return root_readbyte(address); }
  int  m68k_read_memory_16(int address) {
    return root_readword(address);
  }
  int  m68k_read_memory_32(int address) { return root_readlong(address); }
  
  /* Read data immediately following the PC */
  int  m68k_read_immediate_8(int address)  { return root_readbyte(address); }
  int  m68k_read_immediate_16(int address) { return root_readword(address); }
  int  m68k_read_immediate_32(int address) { return root_readlong(address); }
  
  /* Read an instruction (16-bit word immeditately after PC) */
  int  m68k_read_instruction(int address)  { return root_readword(address);  }
  
  /* Write to anywhere */
  void m68k_write_memory_8(int address, int value)  { root_writebyte(address,value);  }
  void m68k_write_memory_16(int address, int value) { root_writeword(address,value);  }
  void m68k_write_memory_32(int address, int value) { root_writelong(address,value);  }
}
#endif

// The same thing for 68KEM
#ifdef COMPILE_WITH_M68KEM
extern "C" {
  int cpu_readmem24(int address) { return root_readbyte(address); }
  int cpu_readmem24_word(int address) { return root_readword(address); }
  int cpu_readmem24_dword(int address) { return root_readlong(address); }

  void cpu_writemem24(int address, int val) { root_writebyte(address, val); }
  void cpu_writemem24_word(int address, int val) { root_writeword(address, val); }
  void cpu_writemem24_dword(int address, int val) { root_writelong(address, val); }

  unsigned char *OP_ROM, *OP_RAM;
  void cpu_setOPbase24(int pc) {
    if(!which) return; // star_mz80_on() wasn't called! :0
    if (pc < 0xa00000) // ROM area
      OP_ROM = which->rom;
    if (pc >= 0xe00000) // RAM
      OP_ROM = which->ram - (pc & 0xff0000);
  }
}
#endif

UINT8 root_z80_read(UINT32 a,struct MemoryReadByte *huh)
{
  (void)(huh);
  if (which) return which->z80_read(a); return 0x00;
}
void root_z80_write(UINT32 a,UINT8 d,struct MemoryWriteByte *huh)
{
  (void)(huh);
  if (which) which->z80_write(a,d);
}
UINT16 root_z80_port_read(UINT16 a, struct z80PortRead *huh)
{
  (void)(huh);
  if (which) return which->z80_port_read(a); return 0x0000;
}
void root_z80_port_write(UINT16 a,UINT8 d, struct z80PortWrite *huh)
{
  (void)(huh);
  if (which) which->z80_port_write(a,d);
}


extern FILE *debug_log;

extern "C"
{
  int mega_dacout=0,mega_dacen=0;
}

#ifdef COMPILE_WITH_STAR
int md::memory_map()
{
  int i=0,j=0;
  int rommax=romlen;

  if (rommax>0xa00000) rommax=0xa00000;
  if (rommax<0) rommax=0;

// FETCH: Set up 2 or 3 FETCH sections
  i=0;
  if (rommax>0)
  { fetch[i].lowaddr=0x000000; fetch[i].highaddr=rommax-1; fetch[i].offset=(unsigned)rom-0x000000; i++; }
  fetch[i].lowaddr=0xff0000; fetch[i].highaddr=0xffffff; fetch[i].offset=(unsigned)ram-  0xff0000; i++;
// Testing
  fetch[i].lowaddr=0xffff0000; fetch[i].highaddr=0xffffffff; fetch[i].offset=(unsigned)ram-0xffff0000; i++;
// Testing 2
  fetch[i].lowaddr=0xff000000; fetch[i].highaddr=0xff000000+rommax-1; fetch[i].offset=(unsigned)rom-0xff000000; i++;
  fetch[i].lowaddr=fetch[i].highaddr=0xffffffff; fetch[i].offset=0; i++;

  if (debug_log!=NULL)
    fprintf (debug_log,"StarScream memory_map has %d fetch sections\n",i);

  i=0; j=0;

#if 0
// Simple version ***************
  readbyte[i].lowaddr=   readword[i].lowaddr=
  writebyte[j].lowaddr=  writeword[j].lowaddr=   0;
  readbyte[i].highaddr=  readword[i].highaddr=
  writebyte[j].highaddr= writeword[j].highaddr=  0xffffffff;

  readbyte[i].memorycall=(void *) root_readbyte;
  readword[i].memorycall=(void *) root_readword;
  writebyte[j].memorycall=(void *)root_writebyte;
  writeword[j].memorycall=(void *)root_writeword;

  readbyte[i].userdata=  readword[i].userdata=
  writebyte[j].userdata= writeword[j].userdata=  NULL;
  i++; j++;
// Simple version end ***************

#else
// Faster version ***************
// IO: Set up 3/4 read sections, and 2/3 write sections
  if (rommax>0)
  {
// Cartridge save RAM memory
    if(save_len) {
      readbyte[i].lowaddr=    readword[i].lowaddr=
      writebyte[j].lowaddr=   writeword[j].lowaddr=   save_start;
      readbyte[i].highaddr=   readword[i].highaddr=
      writebyte[j].highaddr=  writeword[j].highaddr=  save_start+save_len-1;
      readbyte[i].memorycall= (void*)root_readbyte;
      readword[j].memorycall= (void*)root_readword;
      writebyte[i].memorycall=(void*)root_writebyte;
      writeword[j].memorycall=(void*)root_writeword;
      readbyte[i].userdata=   readword[i].userdata=
      writebyte[j].userdata=  writeword[j].userdata=  NULL;
      i++; j++;
    }
// Cartridge ROM memory (read only)
    readbyte[i].lowaddr=   readword[i].lowaddr=   0x000000;
    readbyte[i].highaddr=  readword[i].highaddr=  rommax-1;
    readbyte[i].memorycall=readword[i].memorycall=NULL;
    readbyte[i].userdata=  readword[i].userdata=  rom;
    i++;
// misc memory (e.g. aoo and coo) through root_rw
    readbyte[i].lowaddr=   readword[i].lowaddr=
    writebyte[j].lowaddr=  writeword[j].lowaddr=   rommax;
  }
  else
    readbyte[i].lowaddr=   readword[i].lowaddr=
    writebyte[j].lowaddr=  writeword[j].lowaddr=   0;

  readbyte[i].highaddr=  readword[i].highaddr=
  writebyte[j].highaddr= writeword[j].highaddr=  0xfeffff;

  readbyte[i].memorycall=(void *) root_readbyte;
  readword[i].memorycall=(void *) root_readword;
  writebyte[j].memorycall=(void *)root_writebyte;
  writeword[j].memorycall=(void *)root_writeword;

  readbyte[i].userdata=  readword[i].userdata=
  writebyte[j].userdata= writeword[j].userdata=  NULL;
  i++; j++;

// scratch RAM memory
  readbyte[i].lowaddr =   readword[i].lowaddr =
  writebyte[j].lowaddr =  writeword[j].lowaddr =   0xff0000;
  readbyte[i].highaddr=   readword[i].highaddr=
  writebyte[j].highaddr=  writeword[j].highaddr=   0xffffff;
  readbyte[i].memorycall= readword[i].memorycall=
  writebyte[j].memorycall=writeword[j].memorycall= NULL;
  readbyte[i].userdata=  readword[i].userdata =
  writebyte[j].userdata= writeword[j].userdata =   ram;
  i++; j++;
// Faster version end ***************
#endif

// The end
   readbyte[i].lowaddr  =   readword[i].lowaddr  =
  writebyte[j].lowaddr  =  writeword[j].lowaddr  =
   readbyte[i].highaddr =   readword[i].highaddr =
  writebyte[j].highaddr =  writeword[j].highaddr = 0xffffffff;

   readbyte[i].memorycall=  readword[i].memorycall=
  writebyte[j].memorycall= writeword[j].memorycall=
   readbyte[i].userdata=    readword[i].userdata =
  writebyte[j].userdata=   writeword[j].userdata = NULL;
  i++; j++;

  if (debug_log!=NULL)
    fprintf (debug_log,"StarScream memory_map has %d read sections and %d write sections\n",i,j);

  return 0;
}
#endif

int md::reset()
{
  star_mz80_on();
#ifdef COMPILE_WITH_STAR
  if (cpu_emu==0) s68000reset();
#endif
#ifdef COMPILE_WITH_MUSA
  if (cpu_emu==1) m68k_pulse_reset(NULL);
#endif
#ifdef COMPILE_WITH_M68KEM
  if (cpu_emu==2) m68000_reset(NULL);
#endif
  if (debug_log) fprintf (debug_log,"reset()\n");
  mz80reset();

  // zero = natural state of select line?

  z80_bank68k=z80_online=z80_extra_cycles
    =coo_waiting=coo_cmd=aoo3_toggle=aoo5_toggle=aoo3_six=aoo5_six
    =aoo3_six_timeout=aoo5_six_timeout
    =coo4=coo5=pause=0;
  pad[0]=pad[1]=0xf303f; // Untouched pad

  // Reset FM registers
  {
    int s, r;
    for(s=0;s<2;s++)
      for(r=0;r<0x100;r++)
        fm_reg[s][r]=0;
  }
  fm_sel[0] = fm_sel[1] = fm_tover[0] = fm_tover[1] = 0;
  dac_init();

  odo=odo_line_start=odo_line_end=ras=0;
  //odo_frame_max=0;
  hint_countdown=0;
  z80_int_pending=0;

  star_mz80_off();
  return 0;
}

static struct MemoryReadByte mem_read[]=
{
  {0x2000,0xffff,root_z80_read},
  {(UINT32) -1,(UINT32) -1,NULL}
};
static struct MemoryWriteByte mem_write[]=
{
  {0x2000,0xffff,root_z80_write},
  {(UINT32) -1,(UINT32) -1,NULL}
};
static struct z80PortRead io_read[] ={
  {0x00,0x00ff,root_z80_port_read},
  {(UINT16) -1,(UINT16) -1,NULL}
};
static struct z80PortWrite io_write[]={
  {0x00,0x00ff,root_z80_port_write},
  {(UINT16) -1,(UINT16) -1,NULL}
};

int md::z80_init()
{
  // Set up the z80
  star_mz80_on();
  mz80reset();
  // Modify the default context
  mz80GetContext(&z80);

  // point mz80 stuff
  z80.z80Base=z80ram;
  z80.z80MemRead=mem_read;
  z80.z80MemWrite=mem_write;
  z80.z80IoRead=io_read;
  z80.z80IoWrite=io_write;

  mz80SetContext(&z80);
  mz80reset();
  star_mz80_off();
  return 0;
}


md::md()
{
  romlen=0; 
  mem=rom=ram=z80ram=saveram=NULL;
  save_start=save_len=save_prot=save_active=0;

  pal = frame = 0;
  fm_sel[0]=fm_sel[1]=fm_tover[0]=fm_tover[1]=0;
  snd_mute=0;
  memset(&fm_reg,0,sizeof(fm_reg));
  memset(&ras_fm_ticker,0,sizeof(ras_fm_ticker));

#ifdef COMPILE_WITH_STAR
  fetch=NULL;
  readbyte=readword=writebyte=writeword=NULL;
  memset(&cpu,0,sizeof(cpu));
#endif

  memset(&z80,0,sizeof(z80));
  romfilename[0]=0;
  country_ver=0xff0; layer_sel = 0xff;

  memset(romfilename,0,sizeof(romfilename));

  ok=0;
  if (!vdp.okay()) return;
  vdp.belongs=this;

  //  Format of pad is: __SA____ UDLRBC__

  rom=mem=ram=z80ram=NULL;
  mem=(unsigned char *)malloc(0x20000);
  if (mem==NULL) return;
  memset(mem,0,0x20000);
  ram=   mem+0x00000;
  z80ram=mem+0x10000;

  romlen=0;

  star_mz80_on();  // VERY IMPORTANT - Must call before using stars/mz80!!

#ifdef COMPILE_WITH_STAR
  if (s68000init()!=0) { printf ("s68000init failed!\n"); return; }
#endif

  star_mz80_off(); // VERY IMPORTANT - Must call after using stars/mz80!!

  cpu_emu=-1; // Do we have a cpu emu?
#ifdef COMPILE_WITH_STAR
// Dave: Rich said doing point star stuff is done after s68000init
// in Asgard68000, so just in case...
  fetch= new STARSCREAM_PROGRAMREGION[6]; if (!fetch)     return;
  readbyte= new STARSCREAM_DATAREGION[5]; if (!readbyte)  return;
  readword= new STARSCREAM_DATAREGION[5]; if (!readword)  return;
  writebyte=new STARSCREAM_DATAREGION[5]; if (!writebyte) return;
  writeword=new STARSCREAM_DATAREGION[5]; if (!writeword) return;
  memory_map();

  // point star stuff
  cpu.s_fetch     = cpu.u_fetch     =     fetch;
  cpu.s_readbyte  = cpu.u_readbyte  =  readbyte;
  cpu.s_readword  = cpu.u_readword  =  readword;
  cpu.s_writebyte = cpu.u_writebyte = writebyte;
  cpu.s_writeword = cpu.u_writeword = writeword;
  cpu_emu=0; // zero=starscream, one=musashi, two=68kem
#else
#ifdef COMPILE_WITH_MUSA
  cpu_emu=1; // zero=starscream, one=musash, two=68kemi
#endif
#ifdef COMPILE_WITH_M68KEM
  cpu_emu=2; // zero=starscream, one=musash, two=68kemi
#endif
#endif

#ifdef COMPILE_WITH_MUSA
   m68k_pulse_reset(NULL);
#endif 

#ifdef COMPILE_WITH_M68KEM
  m68000_reset(NULL);
#endif

  z80_init();

  reset(); // reset megadrive

  ok=1;
}


md::~md()
{
  romfilename[0]=0;
  if (rom!=NULL) unplug();

  free(mem);
  rom=mem=ram=z80ram=NULL;

#ifdef COMPILE_WITH_STAR
  if (fetch)     delete[] fetch;
  if (readbyte)  delete[] readbyte;
  if (readword)  delete[] readword;
  if (writebyte) delete[] writebyte;
  if (writeword) delete[] writeword;
#endif

  ok=0;
}

// Byteswaps memory
int byteswap_memory(unsigned char *start,int len)
{ int i; unsigned char tmp;
  for (i=0;i<len;i+=2)
  { tmp=start[i+0]; start[i+0]=start[i+1]; start[i+1]=tmp; }
  return 0;
}

int md::plug_in(unsigned char *cart,int len)
{
  // Plug in the cartridge specified by the uchar *
  // NB - The megadrive will free() it if unplug() is called, or it exits
  // So it must be a single piece of malloced data
  if (cart==NULL) return 1; if (len<=0) return 1;
  byteswap_memory(cart,len); // for starscream
  romlen=len;
  rom=cart;
  // Get saveram start, length (remember byteswapping)
  // First check magic, if there is saveram
  if(rom[0x1b1] == 'R' && rom[0x1b0] == 'A')
    {
      save_start = rom[0x1b5] << 24 | rom[0x1b4] << 16 | 
                   rom[0x1b7] << 8  | rom[0x1b6];
      save_len = rom[0x1b9] << 24 | rom[0x1b8] << 16 |
  	         rom[0x1bb] << 8  | rom[0x1ba];
      // Make sure start is even, end is odd, for alignment
// A ROM that I came across had the start and end bytes of
// the save ram the same and wouldn't work.  Fix this as seen
// fit, I know it could probably use some work. [PKH]
      if(save_start != save_len) {
        if(save_start & 1) --save_start;
        if(!(save_len & 1)) ++save_len;
        save_len -= (save_start - 1);
        saveram = (unsigned char*)malloc(save_len);
	// If save RAM does not overlap main ROM, set it active by default since
	// a few games can't manage to properly switch it on/off.
	if(save_start >= romlen)
	  save_active = 1;
      }
      else {
        save_start = save_len = 0;
        saveram = NULL;
      }
    }
  else
    {
      save_start = save_len = 0;
      saveram = NULL;
    }
#ifdef COMPILE_WITH_STAR
  memory_map(); // Update memory map to include this cartridge
#endif
  reset(); // Reset megadrive
  return 0;
}

int md::unplug()
{
  if (rom==NULL) return 1; if (romlen<=0) return 1;
  free(rom); free(saveram); romlen = save_start = save_len = 0;
#ifdef COMPILE_WITH_STAR
  memory_map(); // Update memory map to include no rom
#endif
  memset(romfilename,0,sizeof(romfilename));
  reset();

  return 0;
}

extern "C" int load_rom_into(char *name,unsigned char *into);

int md::load(char *name)
{
  // Convenience function - calls romload.c functions
  unsigned char *temp=NULL; int len=0;
  if (name==NULL) return 1;
 
  len=load_rom_into(name,NULL);
  if (len<=0) return 1;
  temp=(unsigned char *)malloc(len);
  if (temp==NULL) return 1;
  load_rom_into(name,temp);
  // Register name
  strncpy(romfilename,name,255);
  // Fill the header with ROM info (god this is ugly)
  memcpy((void*)cart_head.system_name,  (void*)(temp + 0x100), 0x10);
  memcpy((void*)cart_head.copyright,    (void*)(temp + 0x110), 0x10);
  memcpy((void*)cart_head.domestic_name,(void*)(temp + 0x120), 0x30);
  memcpy((void*)cart_head.overseas_name,(void*)(temp + 0x150), 0x30);
  memcpy((void*)cart_head.product_no,   (void*)(temp + 0x180), 0x0e);
  cart_head.checksum = temp[0x18e]<<8 | temp[0x18f]; // ugly, but endian-neutral
  memcpy((void*)cart_head.control_data, (void*)(temp + 0x190), 0x10);
  cart_head.rom_start  = temp[0x1a0]<<24 | temp[0x1a1]<<16 | temp[0x1a2]<<8 | temp[0x1a3];
  cart_head.rom_end    = temp[0x1a4]<<24 | temp[0x1a5]<<16 | temp[0x1a6]<<8 | temp[0x1a7];
  cart_head.ram_start  = temp[0x1a8]<<24 | temp[0x1a9]<<16 | temp[0x1aa]<<8 | temp[0x1ab];
  cart_head.ram_end    = temp[0x1ac]<<24 | temp[0x1ad]<<16 | temp[0x1ae]<<8 | temp[0x1af];
  cart_head.save_magic = temp[0x1b0]<<8 | temp[0x1b1];
  cart_head.save_flags = temp[0x1b2]<<8 | temp[0x1b3];
  cart_head.save_start = temp[0x1b4]<<24 | temp[0x1b5]<<16 | temp[0x1b6]<<8 | temp[0x1b7];
  cart_head.save_end   = temp[0x1b8]<<24 | temp[0x1b9]<<16 | temp[0x1ba]<<8 | temp[0x1bb];
  memcpy((void*)cart_head.memo,       (void*)(temp + 0x1c8), 0x28);
  memcpy((void*)cart_head.countries,  (void*)(temp + 0x1f0), 0x10);
  // Plug it into the memory map
  plug_in(temp,len); // md then deallocates it when it's done

  return 0;
}

int md::change_cpu_emu(int to)
{
  // Note - stars/mz80 isn't run here, so star_mz80_on() not necessary
#ifdef COMPILE_WITH_STAR
#ifdef COMPILE_WITH_MUSA
  if (cpu_emu==0 && to==1)
  {
    int i;
    for (i=0;i<8;i++) m68k_poke_dr(i,cpu.dreg[i]);
    for (i=0;i<8;i++) m68k_poke_ar(i,cpu.areg[i]);
    m68k_poke_pc(cpu.pc);
    m68k_poke_sr(cpu.sr);
  }
  if (cpu_emu==1 && to==0)
  {
    int i;
    for (i=0;i<8;i++) cpu.dreg[i]=m68k_peek_dr(i);
    for (i=0;i<8;i++) cpu.areg[i]=m68k_peek_ar(i);
    cpu.pc=m68k_peek_pc();
    cpu.sr=m68k_peek_sr();
  }
#endif
#endif

  cpu_emu=to;
  return 0;
}

int md::z80dump()
{
  FILE *hand;
  hand=fopen("dgz80ram","wb");
  if (hand!=NULL)
  { fwrite(z80ram,1,0x10000,hand); fclose(hand); }
  return 0;
}
