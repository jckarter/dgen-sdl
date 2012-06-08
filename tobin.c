// A little utility to convert SMDs to BINs

#include <stdio.h>
#include <stdlib.h>

// No C header for the function, so I have to prototype them
int load_rom_into(char *name, unsigned char *into);

int main(int argc, char *argv[])
{
  int size;
  FILE *out;
  unsigned char *rom;

  if(argc < 3) { printf("Usage: %s from.smd to.bin\n", argv[0]); return 1; }
  // Get size, so we can malloc it
  size = load_rom_into(argv[1], NULL);
  if(size == -1)
    { fprintf(stderr, "Couldn't open source rom %s!\n", argv[1]); return 1; }
  rom = malloc(size);

  // Get the rom
  load_rom_into(argv[1], rom);

  // Write it to disk
  if(!(out = fopen(argv[2], "w")))
    { fprintf(stderr, "Couldn't write destination rom %s!\n", argv[2]); 
      return 1; }
  fwrite(rom, 1, size, out);
  fclose(out);
  free(rom);
  // That was easy. ;)
  return 0;
}

