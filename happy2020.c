
//#resource "astrocade.inc"
#include "aclib.h"
//#link "aclib.s"
//#link "hdr_autostart.s"
#include "acbios.h"
//#link "acbios.s"

#include <stdlib.h>
#include <string.h>

#define MAX_Y 89

/*{pal:"astrocade",layout:"astrocade"}*/
const byte palette[8] = {
  0x6C, 0xD3, 0x07, 0x01,
  0x66, 0xD3, 0x07, 0x01,
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

#define MAX_FLAKES 80
word flakes[MAX_FLAKES];

// xor a snowflake pixel
void drawflake(word pos) {
  byte* dest = &vmagic[0][pos>>2]; // destination address
  hw_magic = M_SHIFT(pos) | M_XOR; // set magic register
  *dest = (1 << 6); // color = 1
}

// lookup table for the 4 pixel positions in a byte
const byte READMASK[4] = {
  0b11000000,
  0b00110000, 
  0b00001100, 
  0b00000011, 
};

// return the pixel color at a screen position (not shifted)
byte readflake(word pos) {
  byte* dest = &vidmem[0][pos>>2]; // destination address
  return *dest & READMASK[pos&3];
}

// is pixel lit, and is snow color (not 2 or 3)?
bool issnow(word pos) {
  byte bg = readflake(pos);
  return bg && !(bg & 0b10101010);
}

// just return the snow pixels in a byte
byte isolatesnow(byte p) {
  // isolate the static pixels (colors 2 and 3)
  byte bg = (p & 0b10101010);
  byte mask = bg | (bg >> 1);
  // return everything else (just color 1)
  return p & ~mask;
}

// animate snow one pixel downward
bool animate(bool created) {
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
        // cold mode, slide pixel left or right to empty space
        if (!readflake(newpos+1)) {
          newpos++;
        } else if (!readflake(newpos-1)) {
          newpos--;
        } else {
          newpos = 0;	// get rid of snowflake
        }
      }
      // if we didn't get rid of it, erase and redraw
      if (newpos) {
        drawflake(newpos);
        drawflake(oldpos);
      }
      // set new position in array
      flakes[i] = newpos;
    }
    // no valid snowflake in this slot, create one?
    else if (!created) {
      // create a new random snowfake at top (only once per loop)
      oldpos = 1 + rand16() % 158;
      // make sure snowdrift didn't pile to top of screen
      if (readflake(oldpos) && readflake(oldpos+160)) {
        // screen is full, exit
        return false;
      } else {
        // create new snowflake, draw it
        flakes[i] = oldpos;
        created = 1;
        drawflake(oldpos);
      }
    }
  }
  return true; // true if screen isn't full
}

// erase snow psuedo-randomly until it's all melted
bool melt(void) {
  bool melted = 0;
  word addr = 1;
  byte lsb;
  do {
    // use Galois LFSR to randomly select addresses
    lsb = addr & 1;
    addr >>= 1;
    if (lsb) addr ^= 0b100000101001; // period = 4095
    // is address within screen bounds?
    if (addr < MAX_Y*40) {
      // get 4 pixels at address
      byte p = vidmem[0][addr];
      // might there be snow? (at least one pixel = color 1)
      if (p & 0b01010101) {
        // get snow pixels above
        byte pup = addr >= 40 ? isolatesnow(vidmem[-1][addr]) : 0;
        p = isolatesnow(p);
        // melt if we have no snow above
        if (p && !pup) {
          // choose random pixels (if zero, we won't use it)
          byte p2 = p & rand16();
          // XOR snow pixels to erase
          vidmem[0][addr] ^= p2 ? p2 : p;
          melted = 1;
        }
      }
    }
  } while (addr != 1); // loop until LFSR is where we started
  return melted; // true if any pixels melted
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
  display_string(4, 13, OPT_4x4|M_OR|OPT_ON(2), "HAPPY");
  display_string(4, 14, OPT_4x4|M_OR|OPT_ON(2), "HAPPY");
  display_string(20, 47, OPT_4x4|M_OR|OPT_ON(3), "2020");
  display_string(20, 48, OPT_4x4|M_OR|OPT_ON(3), "2020");
  // more compact macro
  SYS_RECTAN(0, 87, 160, 2, 0xaa);
  display_string(32, 80, OPT_OR|OPT_ON(3), "8BITWORKSHOP");
  // infinite loop
  activate_interrupts();
  // make sure screen doesn't black out
  RESET_TIMEOUT();
  
  // loop forever
  while (true) {
    byte i;
    // initialize snowflake array  
    memset(flakes, 0, sizeof(flakes));
    // animate snowflakes until they pile up too high
    while (animate(false)) ;
    // finish falling snowflakes, don't create any
    for (i=0; i<100; i++) animate(true);
    // blue sky
    hw_col0r = hw_col0l = 0xe2;
    // melt the snow
    while (melt()) ;
    // night sky
    hw_col0r = hw_col0l = palette[3];
  }
}
