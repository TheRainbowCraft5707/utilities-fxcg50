#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
#include <fxcg/rtc.h>
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hardwareProvider.hpp"
#include "settingsProvider.hpp"

// START OF POWER MANAGEMENT CODE
#define LCDC *(unsigned int*)(0xB4000000)
int getRawBacklightSubLevel()
{
  Bdisp_DDRegisterSelect(0x5a1);
  return (LCDC & 0xFF) - 6;
}
void setRawBacklightSubLevel(int level)
{
  Bdisp_DDRegisterSelect(0x5a1);
  LCDC = (level & 0xFF) + 6;
}
// END OF POWER MANAGEMENT CODE 

void getHardwareID(char* buffer) {
  // Gets the calculator's unique ID (as told by Simon Lothar): 8 bytes starting at 0x8001FFD0
  // NOTE buffer must be at least 9 bytes long!
  for(int i = 0; i < 8; i++) buffer[i] = *((char*)0x8001FFD0+i);
  buffer[8] = 0;
}

int getHardwareModel() {
  /* Returns calculator model. There is a syscall for this, but with this we know exactly what we
   * are checking. Not to be confused with the "PCB model", which on the Prizm series appears to be
   * always 755D for CG 20 or 755A for CG 10.
   * Returns 0 on unknown/inconclusive model, 1 on fx-CG 10, 2 on fx-CG 20 and 3 on fx-CG 50
   */
  char* byte1 = (char*)0x80000303;
  char* byte2 = (char*)0x80000305;
  if((*byte1 == '\x41') && (*byte2 == '\x5A')) return 1;
  if((*byte1 == '\x44') && (*byte2 == '\xAA')) return 2;
  if((*byte1 == '\x41') && (*byte2 == '\xAA')) return 3;
  return 0;
}

int getIsEmulated() {
  /* returns 1 if the calculator is emulated
   * uses the power source information to detect the emulator
   * obviously, if an emulator comes that emulates this info better, this function will not work.
   */
  int k = *(unsigned char*)P11DR;
  return !k;
}

void setBrightnessToStartupSetting() {
  if (getSetting(SETTING_STARTUP_BRIGHTNESS) <= 249) {
    setRawBacklightSubLevel(getSetting(SETTING_STARTUP_BRIGHTNESS));
  }
}

// CPU CLOCKING CODE (used only from PicoC scripts)

void CPU_change_freq(int mult) { 
   __asm__( 
      "mov r4, r0\n\t" 
      "and #0x3F, r0\n\t"  
      "shll16 r0\n\t"  
      "shll8 r0\n\t"  
      "mov.l frqcr, r1\n\t"   
      "mov.l pll_mask, r3\n\t"   
      "mov.l @r1, r2\n\t"   
      "and r3, r2\n\t"   
      "or r0, r2\n\t"  
      "mov.l r2, @r1\n\t"  
      "mov.l frqcr_kick_bit, r0\n\t"  
      "mov.l @r1, r2\n\t" 
      "or r0, r2\n\t" 
      "rts\n\t" 
      "mov.l r2, @r1\n\t" 
      ".align 4\n\t" 
      "frqcr_kick_bit: .long 0x80000000\n\t" 
      "pll_mask: .long 0xC0FFFFFF\n\t"  
      "frqcr: .long 0xA4150000\n\t" 
   ); 
}