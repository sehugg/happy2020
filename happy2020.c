
//#resource "astrocade.inc"
#include "aclib.h"
//#link "aclib.s"
//#link "hdr_autostart.s"
#include "acbios.h"
//#link "acbios.s"

#include <stdlib.h>
#include <string.h>

/*{pal:"astrocade",layout:"astrocade"}*/
const byte palette[8] = {
  0x77, 0xD4, 0x07, 0x01,
  0x66, 0xD4, 0x07, 0x01,
};

// fast random numbers
unsigned long rnd = 1;
word rand16(void) {
  unsigned long x = rnd;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return rnd = x;
}

// snowflake functions

#define MAX_FLAKES 80

word flakes[MAX_FLAKES];

void drawflake(word pos) {
  byte* dest = &vmagic[0][pos>>2]; // destination address
  hw_magic = M_SHIFT(pos) | M_XOR; // set magic register
  *dest = (1 << 6); // color = 1
}

const byte READMASK[4] = {
  0b11000000,
  0b00110000, 
  0b00001100, 
  0b00000011, 
};

byte readflake(word pos) {
  byte* dest = &vidmem[0][pos>>2]; // destination address
  return *dest & READMASK[pos&3];
}

void animate(void) {
  byte created = 0;
  for (byte i=0; i<MAX_FLAKES; i++) {
    // get old position in array
    word oldpos = flakes[i];
    // is there a valid snowflake?
    if (oldpos) {
      byte bg;
      // move down one pixel
      word newpos = oldpos + 160; 
      // read pixel at new pos
      bg = readflake(newpos);
      if (bg) {
        // slide pixel left or right to empty space
        if (!readflake(newpos+1))
          newpos++;
        else if (!readflake(newpos-1))
          newpos--;
        else
          newpos = 0;	// get rid of snowflake
      }
      // if we didn't get rid of it, erase and redraw
      if (newpos) {
        drawflake(newpos);
        drawflake(oldpos);
      }
      // set new position in array
      flakes[i] = newpos;
    } else if (!created) {
      // create a new random snowfake (only once per loop)
      drawflake(flakes[i] = 160 + rand16() % 160);
      created++;
    }
  }
}

// erase random snowflakes
void melt(void) {
  for (byte i=0; i<5; i++) {
    word pos = rand16();
    if (pos < 84*160) {			// is in bounds?
      // look for a lit pixel that isn't color 2 or 3 (must be 1)
      // and has no neighbor on either left or right
      byte flake = readflake(pos);
      if (flake && !(flake & 0b10101010)) {
        if (readflake(pos-1) == 0	// dark pixel on left
         || readflake(pos+1) == 0	// dark pixel on right
        ) {
          drawflake(pos);		// erase snowflake
        }
      }
    }
  }
}

void main(void) {
  // setup palette
  set_palette(palette);
  // set screen height
  // set horizontal color split (position / 4)
  // set interrupt status
  // use SYS_SETOUT macro
  SYS_SETOUT(89*2, 20, 0);
  // clear screen w/ SYS_FILL macro
  SYS_FILL(0x4000, 89*40, 0);
  // 4x4 must have X coordinate multiple of 4
  display_string(4, 12, OPT_4x4|OPT_ON(2), "HAPPY");
  display_string(20, 45, OPT_4x4|OPT_ON(3), "2020");
  // more compact macro
  SYS_RECTAN(0, 84, 160, 4, 0xaa);
  display_string(32, 75, OPT_OR|OPT_ON(3), "8BITWORKSHOP");
  // infinite loop
  activate_interrupts();
  // make sure screen doesn't black out
  RESET_TIMEOUT();
  // initialize snowflake array  
  memset(flakes, 0, sizeof(flakes));
  // endless loop
  while (1) {
    animate();
    melt();
  }
}
