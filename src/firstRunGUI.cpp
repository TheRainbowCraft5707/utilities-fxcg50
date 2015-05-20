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

#include "powerGUI.hpp"
#include "menuGUI.hpp"
#include "selectorGUI.hpp"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "timeProvider.hpp"
#include "firstRunGUI.hpp"
#include "settingsProvider.hpp"
#include "debugGUI.hpp"

void firstRunWizard() {
  setmGetKeyMode(MGETKEY_MODE_RESTRICT_SETTINGS);
  Bdisp_AllClr_VRAM();
  DrawFrame(COLOR_WHITE);
  drawtnyimLogo( LCD_WIDTH_PX/2-138/2, LCD_HEIGHT_PX/2-42/2);
  int textY = LCD_HEIGHT_PX/2-42/2-24 - 20;
  int textX = 104;
  PrintMini(&textX, &textY, (char*)"brought to you by", 0, 0xFFFFFFFF, 0, 0, COLOR_GRAY, COLOR_WHITE, 1, 0);

  textY = LCD_HEIGHT_PX/2+42/2-24;
  textX = 94;
  PrintMini(&textX, &textY, (char*)"tny. internet media", 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
  textX = 146;
  PrintMini(&textX, &textY, (char*)"i", 0, 0xFFFFFFFF, 0, 0, TNYIM_ORANGE, COLOR_WHITE, 1, 0);
  textX = 235;
  PrintMini(&textX, &textY, (char*)"m", 0, 0xFFFFFFFF, 0, 0, TNYIM_ORANGE, COLOR_WHITE, 1, 0);
  Bdisp_PutDisp_DD();
  blockForMilliseconds(3500);
  
  textArea text;
  text.title = (char*)"Welcome to " ADDIN_FRIENDLYNAME;
  
  textElement elem[7];
  text.elements = elem;
  text.allowEXE = 1;
  
  elem[0].text = (char*)"This add-in provides functionality not originally present on Casio Prizm (fx-CG 10/20) calculators:";
  elem[1].newLine = 1;
  elem[1].lineSpacing = 5;
  elem[1].text = (char*)"- Clock and chronometer\n"
  "- File manager with file search\n"
  "- Text editor and JPEG viewer\n"
  "- Calendar with agenda and tasklist\n"
  "- Balance manager\n"
  "- Fine timeout&backlight adjustment\n"
  "...and more";
  
  elem[2].newLine = 1;
  elem[2].lineSpacing = 8;
  elem[2].color = COLOR_ORANGE;
  elem[2].text = (char*)"Important notes:";
  
  elem[3].newLine = 1;
  elem[3].text = (char*)"You can set the time, date and many settings from the Settings menu, accessible by pressing Shift+Menu (Set Up) at almost any point in the add-in.";
  
  elem[4].newLine = 1;
  elem[4].lineSpacing = 5;
  elem[4].text = (char*)"Tools for more advanced users are hidden, unless the \"Show advanced tools\" setting is turned on.";
  
  elem[5].newLine = 1;
  elem[5].lineSpacing = 8;
  elem[5].text = (char*)"Thanks for reading these notes.\nAfter pressing EXIT or EXE, you may be guided to adjust your calculator's clock.";
  
  elem[6].newLine = 1;
  elem[6].lineSpacing = 3;
  elem[6].text = (char*)"In case you need help with this software, contact info is on the \"About\" screen, to which you can get from the Settings menu.";
  
  text.numelements = 7;
  doTextArea(&text);
  setSetting(SETTING_ACTIVATION_STATUS, 1, 1);
  setmGetKeyMode(MGETKEY_MODE_NORMAL);
}
 
