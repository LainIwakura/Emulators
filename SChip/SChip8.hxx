#ifndef __CHIP8_HXX
#define __CHIP8_HXX

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

#define X (opcode & 0x0F00) >> 8
#define Y (opcode & 0x00F0) >> 4

/* This looks complicated, but if you take
 * one of the numbers like 6 for example:
 * 0xF0 1111 **** 
 * 0x80 1000 *
 * 0xF0 1111 ****
 * 0x90 1001 *  *
 * 0xF0 1111 ****
 *
 * If you can tell..it's a 6 =)
 */
static unsigned char chip8_fontset[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, //0 
  0x20, 0x60, 0x20, 0x20, 0x70, //1 
  0xF0, 0x10, 0xF0, 0x80, 0xF0, //2 
  0xF0, 0x10, 0xF0, 0x10, 0xF0, //3 
  0x90, 0x90, 0xF0, 0x10, 0x10, //4 
  0xF0, 0x80, 0xF0, 0x10, 0xF0, //5 
  0xF0, 0x80, 0xF0, 0x90, 0xF0, //6 
  0xF0, 0x10, 0x20, 0x40, 0x40, //7 
  0xF0, 0x90, 0xF0, 0x90, 0xF0, //8 
  0xF0, 0x90, 0xF0, 0x10, 0xF0, //9 
  0xF0, 0x90, 0xF0, 0x90, 0x90, //A 
  0xE0, 0x90, 0xE0, 0x90, 0xE0, //B 
  0xF0, 0x80, 0x80, 0x80, 0xF0, //C 
  0xE0, 0x90, 0x90, 0x90, 0xE0, //D 
  0xF0, 0x80, 0xF0, 0x80, 0xF0, //E 
  0xF0, 0x80, 0xF0, 0x80, 0x80  //F 
};

class chip8
{
  public:
    bool loadGame(const char *);
    void emulateCycle();

    bool drawFlag;
    unsigned char gfx[128 * 64];   // The screen
    unsigned short key[16];       // Keypad
    
  private:
    unsigned short I;             // Index register
    unsigned short pc;            // program counter
    unsigned short sp;            // Stack pointer
    unsigned short opcode;

    unsigned char V[16];         // Register go from V0 - VF
    unsigned short stack[16];
    unsigned char memory[4096];   // 4096 bytes of memory
    
    unsigned char delay_timer;
    unsigned char sound_timer;

    bool schip_gfx;
    
    void initialize();
};

void chip8::initialize()
{
  pc     = 0x200;
  opcode = 0;
  I      = 0;
  sp     = 0;

  // Set all of our arrays to zero
  memset(gfx, 0, 8192);
  memset(stack, 0, 16);
  memset(key, 0, 16);
  memset(V, 0, 16);
  memset(memory, 0, 4096);

  // Install the font
  for (int i = 0; i < 80; ++i)
    memory[i] = chip8_fontset[i];

  delay_timer = 0;
  sound_timer = 0;

  // Clear screen
  drawFlag = true;

  srand(time(NULL));
}

// Reads a game into memory starting at 0x200 (512)
bool chip8::loadGame(const char* gameName)
{
  initialize();
  printf("Loading: %s\n", gameName);

  FILE *fp = fopen(gameName, "rb");
  if (fp == NULL)
  {
    perror("fopen");
    return false;
  }

  fseek(fp, 0, SEEK_END);
  long lSize = ftell(fp);
  rewind(fp);
  printf("Filesize: %d\n", (int)lSize);

  char *buffer = (char *)malloc(sizeof(char) * lSize);
  if (buffer == NULL)
  {
    perror("malloc");
    return false;
  }

  // Put our file into a buffer
  size_t result = fread(buffer, 1, lSize, fp);
  if (result != lSize)
  {
    perror("Reading error");
    return false;
  }

  if ((4096 - 512) > lSize)
  {
    for (int i = 0; i < lSize; ++i)
      memory[i + 512] = buffer[i];
  }
  else
    printf("Error: ROM too big for memory.");
  
  fclose(fp);
  free(buffer);

  return true;
}

void chip8::emulateCycle()
{
  // Fetch opcode
  opcode = memory[pc] << 8 | memory[pc + 1];
  //printf("Processing opcode: 0x%X\tSwitched version: 0x%X\n", opcode, opcode & 0xF000);

  // Decode
  switch (opcode & 0xF000)
  {
    case 0x0000: // Two opcodes have codes starting with 0x00
      switch (opcode & 0x00FF)
      {
        case 0x00E0: // 0x00E0 clears screen
          for (int i = 0; i < 2048; ++i)
            gfx[i] = 0x0;
          drawFlag = true;
          pc += 2;
          break;

        case 0x00EE: // 0x00EE returns from subroutine
          --sp;
          pc = stack[sp];
          pc += 2;
          break;

        case 0x00FE:
          schip_gfx = false;
          pc += 2;
          break;

        case 0x00FF:
          schip_gfx = true;
          pc += 2;
          break;

        default:
          printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
          break;
      }
      break;

    case 0x1000: // Jumps to address NNN
      pc = opcode & 0x0FFF;
      break;

    case 0x2000: // Calls subroutine at address NNN.
      stack[sp] = pc;
      ++sp;
      pc = opcode & 0x0FFF;
      break;

    case 0x3000: // Skips the next instruction if VX == NN
      pc += ((V[X] == (opcode & 0x00FF)) ? 4 : 2);
      break;

    case 0x4000: // Skips the next instruction if VX != NN
      pc += ((V[X] != (opcode & 0x00FF)) ? 4 : 2);
      break;

    case 0x5000: // Skips the next instruction if VX == VY
      pc += ((V[X] == V[Y]) ? 4 : 2);
      break;

    case 0x6000: // Opcode 0x6XNN Sets VX to NN
      V[X] = opcode & 0x00FF;
      pc += 2;
      break;

    case 0x7000: // Adds NN to VX
      V[X] += opcode & 0x00FF;
      pc += 2;
      break;

    case 0x8000: // Whole bunch of these codes too..uguu~ guess we better switch case..
      {
        switch (opcode & 0x000F)
        {
          case 0x0000: // 0x8XY0: VX is set to VY
            V[X] = V[Y];
            pc += 2;
            break;

          case 0x0001: // 0x8XY1: VX is set to VX | VY
            V[X] |= V[Y];
            pc += 2;
            break;

          case 0x0002: // 0x8XY2: VX is set to VX & VY
            V[X] &= V[Y];
            pc += 2;
            break;

          case 0x0003: // 0x8XY3: VX is set to VX ^ VY
            V[X] ^= V[Y];
            pc += 2;
            break;

          case 0x0004: // Adds VY to VX, sets the carry if there is one.
            V[0xF] = ((V[Y] > (0xFF - V[X])) ? 1 : 0); 
            V[X] += V[Y];
            pc += 2;
            break;

          case 0x0005: // VY is subtracted from VX. Sets VF to 0 if there is a carry.
            V[0xF] = ((V[Y] > V[X]) ? 0 : 1);
            V[X] -= V[Y];
            pc += 2;
            break;

          case 0x0006: // Shifts VX to the right by 1. VF is the value of the LSB of VX prior to the shift
            V[0xF] = V[X] & 0x1;
            V[X] >>= 1;
            pc += 2;
            break;

          case 0x0007: // Sets VX to VY minus VX. VF is 0 when there is a borrow, 1 when not.
            V[0xF] = ((V[X] > V[Y]) ? 0 : 1);
            V[X] = V[Y] - V[X];
            pc += 2;
            break;

          case 0x000E:
            V[0xF] = V[X] >> 7;
            V[X] <<= 1;
            pc += 2;
            break;

          default:
            printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
            break;
        }

        break;
      }

    case 0x9000: // Skips the next instruction if VX != VY
      pc += ((V[X] != V[Y]) ? 4 : 2);
      break;

    case 0xA000: // Sets I to address NNN
      I = opcode & 0x0FFF;
      pc += 2;
      break;

    case 0xB000: // Jumps to address NNN + V0
      pc = V[0] + (opcode & 0x0FFF);
      break;

    case 0xC000:
      V[X] = (rand() % 0xFF) & (opcode & 0x00FF);
      pc += 2;
      break;

    // DXYN draws a pixel at VX, VY that is N pixels high.
    case 0xD000:
      {
        unsigned short x = V[X];
        unsigned short y = V[Y];
        unsigned short height = opcode & 0x000F;
        unsigned short pixel;

        V[0xF] = 0;

        for (int yline = 0; yline < height; yline++)
        {
          pixel = memory[I + yline];
          for (int xline = 0; xline < 8; xline++)
          {
            if ((pixel & (0x80 >> xline)) != 0)
            {
              if (gfx[(x + xline + ((y + yline) * 64))] == 1)
                V[0xF] = 1;
              gfx[x + xline + ((y + yline) * 64)] ^= 1;
            }
          }
        }

        drawFlag = true;
        pc += 2;
      }
      break;

    case 0xE000:
      switch (opcode & 0x00FF)
      {
        // EX9E: Skips the next instruction
        case 0x009E:
          pc += ((key[V[X]] != 0) ? 4 : 2);
          break;

        case 0x00A1:
          pc += ((key[V[X]] == 0) ? 4 : 2);
          break;

        default:
          printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
          break;
      }
      break;

    case 0xF000: // Handling opcodes in the F range..there are a few.
        switch (opcode & 0x00FF)
        {
          case 0x0007:
            V[X] = delay_timer;
            pc += 2;
            break;

          case 0x000A:
            {
              bool keyPress = false;

              for (int i = 0; i < 16; ++i)
              {
                if (key[i] != 0)
                {
                  V[X] = i;
                  keyPress = true;
                }
              }

              if (!keyPress)
                return;

              pc += 2;
            }
            break;

          case 0x0015:
            delay_timer = V[X];
            pc += 2;
            break;

          case 0x0018:
            sound_timer = V[X];
            pc += 2;
            break;

          case 0x001E: // FX1E: Adds VX to I
            // An overflow check, I didn't think of this but it is necessary
            V[0xF] = ((I + V[X] > 0xFFF) ? 1 : 0);
            I += V[X];
            pc += 2;
            break;

          case 0x0029: // Sets I to the location of the sprite for the char in VX.
            I = V[X] * 0x5;
            pc += 2;
            break;
          
          case 0x0033: // FX33: Stores BCD representation of VX in I, I + 1 and I + 2
            memory[I]     = V[X] / 100;
            memory[I + 1] = (V[X] / 10) % 10;
            memory[I + 2] = (V[X] % 100) % 10;
            pc += 2;
            break;

          case 0x0055:
            for (int i = 0; i <= (X); ++i)
              memory[I + i] = V[i];

            I += (X) + 1;
            pc += 2;

          case 0x0065: // Fills V0 to VX with values from memory starting at I
            for (int i = 0; i <= (X); ++i)
              V[i] = memory[I + i];

            I += (X) + 1;
            pc += 2;
            break;

          default:
            printf("Unknown opcode: [0x000F]: 0x%X\n", opcode);
            break;
        }
        break;
    
    default:
      printf("Unknown opcode: 0x%X\n", opcode);
      break;
  }

  if (delay_timer > 0) --delay_timer;

  if (sound_timer > 0)
  {
    if (sound_timer == 1) printf("BEEP!\n");
    --sound_timer;
  }
}

#endif
