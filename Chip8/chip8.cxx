/* Chip8 emulator written by Zach Easterbrook
 * AKA Lain Iwakura.
 *
 * A memory map:
 * 0x000 - 0x1FF
 * 0x050 - 0x0A0
 * 0x200 - 0xFFF
 */

#include <cstdio>
#include <GL/glut.h>
#include <iostream>
#include "chip8.hxx"

chip8 myChip8;

int main()
{
  /*
  setupGraphics();
  setupInput();
  */

  myChip8.initialize();
  myChip8.loadGame("INVADERS");

  for(;;)
  {
    myChip8.emulateCycle();

    /*
    if (myChip8.drawFlag)
      drawGraphics();
    */

    //myChip8.setKeys();
  }

  return 0;
}
