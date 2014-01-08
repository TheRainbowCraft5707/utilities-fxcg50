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

#include "homeGUI.hpp"
#include "menuGUI.hpp"
#include "inputGUI.hpp"
#include "hardwareProvider.hpp"
#include "keyboardProvider.hpp"
#include "graphicsProvider.hpp"
#include "timeProvider.hpp"
#include "timeGUI.hpp"
#include "settingsProvider.hpp"
#include "settingsGUI.hpp"
#include "powerGUI.hpp"
#include "lightGUI.hpp"
#include "chronoProvider.hpp"
#include "chronoGUI.hpp"
#include "textGUI.hpp"
#include "tasksGUI.hpp"
#include "calendarGUI.hpp"
#include "toolsGUI.hpp"
#include "lockGUI.hpp"
#include "fileGUI.hpp"
#include "editorGUI.hpp"
#include "debugGUI.hpp"

int pane_keycache = 0; // TODO: see if it's possible not to have this being a global var

void showHome(chronometer* chrono) {
  unsigned short key = 0;
  unsigned short prevkey = 0;
  int keyCol; int keyRow; //these aren't actually used, but they are needed to hold different getkey-like results
  int fgcolor;
  int bgcolor;
  while (1) {
    Bdisp_AllClr_VRAM();

    //black theme, or not?
    if (GetSetting(SETTING_THEME) == 1) {
      Bdisp_Fill_VRAM( COLOR_BLACK, 3 );
      DrawFrame( 0x000000  );
      bgcolor = COLOR_BLACK;
      fgcolor = COLOR_WHITE;
    } else {
      DrawFrame( 0xFFFFFF  );
      bgcolor = COLOR_WHITE;
      fgcolor = COLOR_BLACK;
    }
    DefineStatusMessage((char*)"", 1, 0, 0);
    DisplayStatusArea();
    
    // Print time
    drawHomeClock(GetSetting(SETTING_CLOCK_TYPE), fgcolor, bgcolor);
    if (GetSetting(SETTING_THEME)) darkenStatusbar();

    //Show FKeys
    if (GetSetting(SETTING_DISPLAY_FKEYS)) {
      int iresult;
      GetFKeyPtr(0x043A, &iresult); // POWER
      FKey_Display(0, (int*)iresult);
      GetFKeyPtr(0x043E, &iresult); // LIGHT
      FKey_Display(1, (int*)iresult);
      GetFKeyPtr(0x012A, &iresult); // TIME
      FKey_Display(2, (int*)iresult);
      GetFKeyPtr(0x011C, &iresult); // TOOL
      FKey_Display(3, (int*)iresult);
      if (GetSetting(SETTING_ENABLE_LOCK)) {
        GetFKeyPtr(0x04D3, &iresult); // key icon (lock)
        FKey_Display(4, (int*)iresult);
      }
      if (GetSetting(SETTING_THEME)) {
        darkenFkeys((GetSetting(SETTING_ENABLE_LOCK) == 1 ? 5 : 4));
      }
    }
    Bdisp_PutDisp_DD();
    checkDownwardsChronoCompleteGUI(chrono, NUMBER_OF_CHRONO);
    if (0 != GetKeyWait_OS(&keyCol, &keyRow, 2, 0, 0, &key) || pane_keycache ) {
      if(!pane_keycache) {
        key = PRGM_GetKey();
      } else {
        switch(pane_keycache) {
          case KEY_CTRL_F1:
            key = KEY_PRGM_F1; break;
          case KEY_CTRL_F2:
            key = KEY_PRGM_F2; break;
          case KEY_CTRL_F3:
            key = KEY_PRGM_F3; break;
          case KEY_CTRL_F4:
            key = KEY_PRGM_F4; break;
          case KEY_CTRL_F5:
            key = KEY_PRGM_F5; break;
        }
        pane_keycache = 0;
      }
      switch (key) {
        case KEY_PRGM_SHIFT:
          //turn on/off shift manually because getkeywait doesn't do it
          if (GetSetupSetting( (unsigned int)0x14) == 0) { SetSetupSetting( (unsigned int)0x14, 1); }
          else { SetSetupSetting( (unsigned int)0x14, 0); }
          break;
        case KEY_PRGM_MENU:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            settingsMenu();
          }
        case KEY_PRGM_ACON:
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            DisplayStatusArea();
            PowerOff(1);
            SetSetupSetting( (unsigned int)0x14, 0);
            DisplayStatusArea();
          }
          break;
        case KEY_PRGM_F1:
          powerMenu();
          break;
        case KEY_PRGM_F2:
          lightMenu();
          break;
        case KEY_PRGM_F3:
          timeMenu(chrono);
          break;
        case KEY_PRGM_F4:
          toolsMenu();
          break;
        case KEY_PRGM_F5:
          lockApp();
          break;
        case KEY_PRGM_F6:
          break;
        case 71: //KEY_PRGM_0, which is not defined in the SDK and I'm too lazy to add it every time I update the includes folder...
          if (GetSetupSetting( (unsigned int)0x14) == 1) {
            SetSetupSetting( (unsigned int)0x14, 0);
            DisplayStatusArea();
            char code[25] = "";
            textInput input;
            input.y=8;
            input.charlimit=21;
            input.buffer = (char*)code;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_CONFIRM) {
              if(!strcmp(code, "qazedcol")) masterControl();
            }
          }
          break;
        case KEY_PRGM_RETURN:
          if(GetSetting(SETTING_LOCK_ON_EXE)) {
            lockApp();
          }
          break;
        case KEY_PRGM_RIGHT:
          if(GetSetting(SETTING_HOME_PANES)) eventsPane();
          break;
        case 76: //x-0-theta key
          currentTimeToBasicVar();
          break;
        default:
          break;
      }
      if (key!=prevkey && key!=KEY_PRGM_SHIFT) SetSetupSetting( (unsigned int)0x14, 0);
    }
  }
}

void powerMenu() {
  drawFkeyPopup(0, GetSetting(SETTING_THEME), 1);
  mPrintXY(2, 2, (char*)"Power options", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Auto power off");
  strcpy(menuitems[1].text, "Backlight duration");
  strcpy(menuitems[2].text, "Backlight level");
  strcpy(menuitems[3].text, "Power information");
  strcpy(menuitems[4].text, "CPU speed");
  
  Menu menu;
  menu.items=menuitems;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  if(GetSetting(SETTING_SHOW_ADVANCED)) {
    menu.numitems=5;
  } else {
    menu.numitems=4;
    menu.scrollbar=0;
  }
  menu.darken=GetSetting(SETTING_THEME);
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      DrawFrame(COLOR_WHITE);
      if(menu.selection == 1) {
        changePoweroffTimeout();
      } else if(menu.selection == 2) {
        changeBacklightTimeout();
      } else if(menu.selection == 3) {
        changeBacklightLevel();
      } else if(menu.selection == 4) {
        powerInformation();
      } else if(menu.selection == 5) {
        setCPUclock();
      }
      return;
    } else if(res >= KEY_CTRL_F2 && res <= KEY_CTRL_F6) {
      pane_keycache = res;
      return;
    } else if(res == MENU_RETURN_EXIT) return;
  }
}

void lightMenu() {
  drawFkeyPopup(1, GetSetting(SETTING_THEME), 1);
  mPrintXY(2, 2, (char*)"Light tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Lantern");  
  strcpy(menuitems[1].text, "Flashlight");
  strcpy(menuitems[2].text, "Morse light");
  strcpy(menuitems[3].text, "Color light");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=4;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.scrollbar=0;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=GetSetting(SETTING_THEME);
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      DrawFrame(COLOR_WHITE);
      if(menu.selection == 1) {
        lantern();
      } else if(menu.selection == 2) {
        flashLight();
      } else if(menu.selection == 3) {
        morseLight();
      } else if(menu.selection == 4) {
        colorLight();
      }
      return;
    } else if(res >= KEY_CTRL_F1 && res != KEY_CTRL_F2 && res <= KEY_CTRL_F6) {
      pane_keycache = res;
      return;
    } else if(res == MENU_RETURN_EXIT) return;
  }
}

void timeMenu(chronometer* chrono) {
  drawFkeyPopup(2, GetSetting(SETTING_THEME), 1);
  mPrintXY(2, 2, (char*)"Time tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[5];
  strcpy(menuitems[0].text, "Calendar");
  strcpy(menuitems[1].text, "Tasks");
  strcpy(menuitems[2].text, "Chronometer");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=3;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.scrollbar=0;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=GetSetting(SETTING_THEME);
  
  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      DrawFrame(COLOR_WHITE);
      if(menu.selection == 1) {
        viewCalendar();
      } else if(menu.selection == 2) {
        viewTasks();
      } else if(menu.selection == 3) {
        chronoScreen(chrono);
      }
      return;
    } else if(res >= KEY_CTRL_F1 && res != KEY_CTRL_F3 && res <= KEY_CTRL_F6) {
      pane_keycache = res;
      return;
    } else if(res == MENU_RETURN_EXIT) return;
  }
}

void toolsMenu() {
  drawFkeyPopup(3, GetSetting(SETTING_THEME), 1);
  mPrintXY(2, 2, (char*)"Tools", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  
  MenuItem menuitems[6];
  strcpy(menuitems[0].text, "File manager");
  strcpy(menuitems[1].text, "Text editor");
  strcpy(menuitems[2].text, "Memory usage");
  strcpy(menuitems[3].text, "Add-In manager");
  strcpy(menuitems[4].text, "Function key color");
  strcpy(menuitems[5].text, "System information");
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=6;
  menu.width=19;
  menu.height=4;
  menu.startX=2;
  menu.startY=3;
  menu.type=MENUTYPE_FKEYS;
  menu.darken=GetSetting(SETTING_THEME);

  while(1) {
    int res = doMenu(&menu);
    if(res == MENU_RETURN_SELECTION) {
      DrawFrame(COLOR_WHITE);
      if(menu.selection == 1) {
        fileManager();
      } else if(menu.selection == 2) {
        fileTextEditor();
      } else if(menu.selection == 3) {
        memoryCapacityViewer();
      } else if(menu.selection == 4) {
        addinManager();
      } else if(menu.selection == 5) {
        changeFKeyColor();
      } else if(menu.selection == 6) {
        systemInfo();
      }
      return;
    } else if(res >= KEY_CTRL_F1 && res != KEY_CTRL_F4 && res <= KEY_CTRL_F6) {
      pane_keycache = res;
      return;
    } else if(res == MENU_RETURN_EXIT) return;
  }
}
void pane_drawTodayEvents(CalendarEvent* calevents, int startx, int starty, int numevents, int maxevents) {
  color_t color_fg, color_bg, color_title;
  if (GetSetting(SETTING_THEME)) { color_fg = COLOR_WHITE; color_bg = COLOR_BLACK; color_title = COLOR_ORANGE; } else
  { color_fg = COLOR_BLACK; color_bg = COLOR_WHITE; color_title = COLOR_BLUE; }
  
  int textX = startx;
  int textY = starty;
  if (numevents>0) {
    int curevent = 0; //current processing event
    unsigned char itemtext[25] = "";
    PrintMini(&textX, &textY, (unsigned char*)"Events starting today", 0, 0xFFFFFFFF, 0, 0, color_title, color_bg, 1, 0); //draw
    while(curevent < numevents && curevent < maxevents) {
      textX = startx + 5;
      textY = textY + 18;
      strcpy((char*)itemtext, "- ");
      strcat((char*)itemtext, (char*)calevents[curevent].title);
      int tcolor = calevents[curevent].category-1;
      if (GetSetting(SETTING_THEME) && tcolor == TEXT_COLOR_BLACK) tcolor = TEXT_COLOR_WHITE;
      color_t tfcolor = textColorToFullColor(tcolor);
      PrintMini(&textX, &textY, (unsigned char*)itemtext, 0, 0xFFFFFFFF, 0, 0, tfcolor, color_bg, 1, 0); //draw
      curevent++;
    }
    if(numevents>maxevents) {
      textX = startx + 5;
      textY = textY + 20;
      strcpy((char*)itemtext, (char*)"  ...and ");
      unsigned char buffer[10] = "";
      itoa(numevents-maxevents, buffer);
      strcat((char*)itemtext, (char*)buffer);
      strcat((char*)itemtext, (char*)" more event");
      if(numevents-maxevents > 1) strcat((char*)itemtext, (char*)"s");
      PrintMini(&textX, &textY, (unsigned char*)itemtext, 0, 0xFFFFFFFF, 0, 0, color_fg, color_bg, 1, 0); //draw
    }
  } else {
    PrintMini(&textX, &textY, (unsigned char*)"  No events starting today", 0, 0xFFFFFFFF, 0, 0, color_fg, color_bg, 1, 0); //draw
  } 
}
void eventsPane() {
  int key;
  EventDate thisday;
  const int eventsToDisplayInFull=6;
  thisday.day = getCurrentDay(); thisday.month = getCurrentMonth(); thisday.year = getCurrentYear();
  int numevents = GetEventsForDate(&thisday, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* calevents;
  if (numevents > 0) {
    int getnum = (numevents>eventsToDisplayInFull?eventsToDisplayInFull:numevents); // number of events to parse.
    calevents = (CalendarEvent*)alloca(getnum*sizeof(CalendarEvent)); // we don't want to allocate more than what we need for the few events we want to display in full.
    GetEventsForDate(&thisday, CALENDARFOLDER, calevents, eventsToDisplayInFull);
  } else {
    calevents = NULL;
  }
  int inscreen = 1;
  if (GetSetting(SETTING_THEME)) {
    Bdisp_Fill_VRAM( COLOR_BLACK, 2 ); //fill between the status area and f-key area
    DrawFrame( 0x000000  );
  } else {
    Bdisp_Fill_VRAM( COLOR_WHITE, 2 ); //fill between the status area and f-key area
    DrawFrame( 0xfffff  );
  }
  pane_drawTodayEvents(calevents, 0, 0, numevents, eventsToDisplayInFull);
  if(GetSetting(SETTING_SHOW_CALENDAR_BUSY_MAP)) drawDayBusyMap(&thisday, 0, LCD_HEIGHT_PX-44, LCD_WIDTH_PX, 15, 1,0,0);
  while (inscreen) {
    if (GetSetting(SETTING_THEME)) {
      DisplayStatusArea();
      darkenStatusbar();
      DrawFrame( 0x000000  );
    }
    mGetKey(&key);
    switch(key) {
      case KEY_CTRL_F1:
      case KEY_CTRL_F2:
      case KEY_CTRL_F3:
      case KEY_CTRL_F4:
        pane_keycache = key;
        return; //exit to main pane
      case KEY_CTRL_F5:
        if(GetSetting(SETTING_ENABLE_LOCK)) { pane_keycache = key; return; }
        break;
      case KEY_CTRL_LEFT:
      case KEY_CTRL_EXIT:
        return; //return to the pane to the left (main)
    }
  }
}