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
#include <alloca.h>

#include "memsysGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "hardwareProvider.hpp"
#include "graphicsProvider.hpp"
#include "stringsProvider.hpp"
#include "selectorGUI.hpp" 
#include "fileProvider.hpp"

// the following two functions are auxiliars to memoryCapacityScreen
void drawCapacityBar(int textY, long long int cur, long long int full) {
  long long int barwidthcpl = (long long int)(LCD_WIDTH_PX*cur)/(long long int)full;
  
  int newTextX = 0;
  textY += 5;
  char buffer[50];
  itoa(100*cur/full, (unsigned char*)buffer);
  strcat(buffer, "% used");
  PrintMiniMini( &newTextX, &textY, buffer, 0, TEXT_COLOR_CYAN, 1 ); //fake draw
  int textX = LCD_WIDTH_PX/2 - newTextX/2;
  PrintMiniMini( &textX, &textY, buffer, 0, TEXT_COLOR_CYAN, 0 ); //draw  
  
  textY += 19;
  replaceColorInArea(0, textY, LCD_WIDTH_PX, 20, COLOR_WHITE, COLOR_GRAY);  
  replaceColorInArea(0, textY, barwidthcpl, 20, COLOR_GRAY, COLOR_BLUE);
  replaceColorInArea(0, textY, LCD_WIDTH_PX, 20, COLOR_CYAN, COLOR_WHITE);
}

void drawCapacityText(int* textY, const char* desc, int cur, int full) {
  char buffer[100];
  *textY=*textY+22; int textX = 0;
  sprintf(buffer, "%s: %d bytes free", desc, full-cur);
  PrintMini(&textX, textY, buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);

  *textY = *textY + 17; textX = 60;
  sprintf(buffer, "out of %d bytes (%d bytes used)", full, cur);
  PrintMiniMini( &textX, textY, buffer, 0, TEXT_COLOR_BLACK, 0 );
  *textY = *textY + 12;
}

void memoryCapacityScreen(int isPaneOffset) {
  if(!isPaneOffset) {
    Bdisp_AllClr_VRAM();
    drawScreenTitle("Memory Usage");
  }
  int smemfree = 0;
  unsigned short smemMedia[10]={'\\','\\','f','l','s','0',0};
  Bfile_GetMediaFree_OS( smemMedia, &smemfree );

  int textY = 8 + isPaneOffset;
  drawCapacityText(&textY, "Storage", TOTAL_SMEM-smemfree, TOTAL_SMEM);
  drawCapacityBar(textY, TOTAL_SMEM-smemfree, TOTAL_SMEM);
  
  int MCSmaxspace; int MCScurrentload; int MCSfreespace;  
  MCS_GetState( &MCSmaxspace, &MCScurrentload, &MCSfreespace );
  drawCapacityText(&textY, "Main", MCScurrentload, MCSmaxspace);
  drawCapacityBar(textY, MCScurrentload, MCSmaxspace);

  char* flagpointer = (char*)0x80BE0000;
  int PWcurrentload = 0;
  while(*flagpointer == 0x0F) {
    flagpointer = flagpointer + 0x40;
    PWcurrentload += 0x40;
  }
  drawCapacityText(&textY, "Password", PWcurrentload, TOTAL_PASSWORD);
  drawCapacityBar(textY, PWcurrentload, TOTAL_PASSWORD);

  while(!isPaneOffset) {
    int key;
    mGetKey(&key);
    if(key == KEY_CTRL_EXIT) return;
  }
}

int getAddins(AddIn addins[]) {
  /*searches storage memory for active and inactive add-ins, returns their count*/
  unsigned short path[MAX_FILENAME_SIZE+1], found[MAX_FILENAME_SIZE+1];
  char buffer[MAX_FILENAME_SIZE+1];

  // make the buffer
  strcpy(buffer, SMEM_PREFIX"*.g3a");
  
  int curitem = 0;
  file_type_t fileinfo;
  int findhandle;
  Bfile_StrToName_ncpy(path, buffer, MAX_FILENAME_SIZE+1);
  int ret = Bfile_FindFirst_NON_SMEM((const char*)path, &findhandle, (char*)found, &fileinfo);
  while(!ret) {
    Bfile_NameToStr_ncpy(buffer, found, MAX_FILENAME_SIZE+1);
    if(strcmp(buffer, (char*)SELFFILE) && fileinfo.fsize) {
      strcpy(addins[curitem].filename, (char*)buffer);
      // get friendly add-in name from the file:
      char filename[MAX_FILENAME_SIZE];
      strcpy(filename, SMEM_PREFIX);
      strcat(filename, buffer);
      int hFile = fileOpen(filename);
      if(hFile >= 0) {
        Bfile_ReadFile_OS(hFile, buffer, 2, 0x0024);
        Bfile_ReadFile_OS(hFile, addins[curitem].name, 24, 0x006B); // read English friendly name
        Bfile_CloseFile_OS(hFile);
      } else strcpy(addins[curitem].name, (char*)buffer);
      addins[curitem].active = ((unsigned char)buffer[0] == 1);
      curitem++;
    }
    ret = Bfile_FindNext_NON_SMEM(findhandle, (char*)found, (char*)&fileinfo);
  }

  Bfile_FindClose(findhandle);  
  return curitem;
}

void addinManager() {
  Menu menu;
  
  menu.scrollout=1;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  menu.nodatamsg = (char*)"No Add-Ins";
  menu.title = (char*)"Add-In Manager";
  while(addinManagerChild(&menu));
}

int addinManagerChild(Menu* menu) {
  //returns 1 when it wants to be restarted (refresh addins)
  //returns 0 if the idea really is to exit the screen
  AddIn addins[200]; 
  
  menu->numitems = getAddins(addins);
  MenuItem* menuitems = (MenuItem*)alloca(menu->numitems*sizeof(MenuItem));
  menu->items = menuitems;
 
  Bdisp_AllClr_VRAM();
  for(int curaddin = 0; curaddin < menu->numitems; curaddin++) {
    menuitems[curaddin].text = (char*)addins[curaddin].name;
    menuitems[curaddin].color = (addins[curaddin].active ? TEXT_COLOR_BLACK : TEXT_COLOR_CYAN);
  }
  
  if(menu->numitems>0) drawFkeyLabels(0x0103, 0x0038); // CHANGE (white), DELETE
  
  char buffer[MAX_FILENAME_SIZE+1];
  unsigned short oldpath[MAX_FILENAME_SIZE+1];
  switch(doMenu(menu)) {
    case KEY_CTRL_F1:
      if(menu->numitems > 0) {
        strcpy(buffer, SMEM_PREFIX);
        strcat(buffer, addins[menu->selection-1].filename);
        int hFile = fileOpen(buffer);
        if(hFile >= 0) {
          Bfile_SeekFile_OS(hFile, 0x0024);
          // toggle two bits that the OS checks to determine if a g3a is valid
          buffer[0] = buffer[1] = 1 + addins[menu->selection-1].active;
          buffer[2] = 0;
          Bfile_WriteFile_OS(hFile, buffer, 2);
          Bfile_CloseFile_OS(hFile);
        }
        return 1; //reload list
      }
      break;
    case KEY_CTRL_F2:
    case KEY_CTRL_DEL:
      if(menu->numitems > 0) {
        mMsgBoxPush(4);
        multiPrintXY(3, 2, "Delete the\nSelected Add-In?",
                     TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        if(closeMsgBox(1, 4)) {
          strcpy(buffer, SMEM_PREFIX);
          strcat(buffer, addins[menu->selection-1].filename);
          Bfile_StrToName_ncpy(oldpath, buffer, MAX_FILENAME_SIZE+1);
          Bfile_DeleteEntry( oldpath );
        }
        return 1;
      }
      break;
    case MENU_RETURN_EXIT: return 0; break;
  }
  return 1;
}

void changeFKeyColor() {
  unsigned char*keycolor = (unsigned char*) 0xFD8013E4;
  char major, minor;
  short c, d;
  GlibGetOSVersionInfo(&major, &minor, &c, &d);
  // if OS 02.02 or later...
  if(major >= 2) {
    if(minor >= 2 || major > 2) {
      keycolor = (unsigned char*) 0xFD8016AC;
    }
  }

  if(getHardwareModel() == 3){
    keycolor = (unsigned char*) 0xFD8017CC;
  }

  Bdisp_AllClr_VRAM();
  DisplayStatusArea();
  drawScreenTitle("Function Key Color");
  textArea text;
  text.type = TEXTAREATYPE_INSTANT_RETURN;
  text.y = 132;
  text.lineHeight=19;
  textElement elem[1];
  text.elements = elem;
  text.scrollbar=0;
  
  elem[0].text = (char*)"Please note that only the " ADDIN_FRIENDLYNAME " add-in and a hidden "
                        "debug screen on your calculator are able to change this setting, which "
                        "persists across reboots. To reset it back to the black color you need to "
                        "use this add-in, the hidden debug screen or to reset the Main Memory.";
  elem[0].minimini = 1;
  text.numelements = 1;
  doTextArea(&text);
  unsigned char selcolor = ColorIndexDialog1( *keycolor, 0 );
  if(selcolor != (unsigned char)0xFF) {
    //user didn't press EXIT, QUIT or AC/ON. input is validated.
    *keycolor = selcolor;
  }
}

void systemInfo() {
  char OSname[12];
  for(int i = 0; i < 8; i++) OSname[i] = *((char*)0x80020000+i);
  OSname[8] = 0;
  
  char OSversion[12];
  for(int i = 0; i < 10; i++) OSversion[i] = *((char*)0x80020020+i);
  OSversion[10] = 0;
  
  char OSdate[20];
  for(int i = 0; i < 14; i++) OSdate[i] = *((char*)0x80B5FFE0+i);
  OSdate[14] = 0;
  
  char pcbModel[6];
  for(int i = 0; i < 4; i++) pcbModel[i] = *((char*)0x80000300+i);
  pcbModel[4] = 0;
  
  char ABSname[12];
  for(int i = 0; i < 8; i++) ABSname[i] = *((char*)0x80000338+i);
  ABSname[8] = 0;
  
  char ABSdate[20];
  for(int i = 0; i < 14; i++) ABSdate[i] = *((char*)0x8001FFB0+i);
  ABSdate[14] = 0;
  
  char devID[10];
  getHardwareID(devID);  
  
  long pvr = *(long *)0xFF000030;
  char pvrstr[9];
  for (int i = 0; i <= 7; i++) {
    int d = (pvr >> (i * 4)) & 0xF;
    pvrstr[7 - i] = d + ((d > 9)? 0x37 : 0x30);
  }
  pvrstr[8] = 0;
  
  long prr = *(long *)0xFF000044;
  char prrstr[9];
  for (int i = 0; i <= 7; i++) {
    int d = (prr >> (i * 4)) & 0xF;
    prrstr[7 - i] = d + ((d > 9)? 0x37 : 0x30);
  }
  prrstr[8] = 0;
  
  long cvr = *(long *)0xFF000040;
  char cvrstr[9];
  for (int i = 0; i <= 7; i++) {
    int d = (cvr >> (i * 4)) & 0xF;
    cvrstr[7 - i] = d + ((d > 9)? 0x37 : 0x30);
  }
  cvrstr[8] = 0;
  
  textArea text;
  text.title = (char*)"System Information";
  
  textElement elem[25];
  text.elements = elem;
  
  elem[0].text = (char*)"Operating System:";
  elem[0].spaceAtEnd=1;
  elem[1].text = OSname;
  
  elem[2].newLine = 1;
  elem[2].text = (char*)"OS version:";
  elem[2].spaceAtEnd=1;
  elem[3].text = OSversion;
  
  elem[4].newLine = 1;
  elem[4].text = (char*)"OS date:";
  elem[4].spaceAtEnd=1;
  elem[5].text = OSdate;
  
  elem[6].newLine = 1;
  elem[6].text = (char*)"PCB model:";
  elem[6].spaceAtEnd=1;
  elem[7].text = pcbModel;
  
  elem[8].newLine = 1;
  elem[8].text = (char*)"Calculator model:";
  elem[8].spaceAtEnd=1;
  if(getHardwareModel() == 1) {
    elem[9].text = (char*)"fx-CG 10";
  } else if(getHardwareModel() == 2) {
    elem[9].text = (char*)"fx-CG 20";
  } else if(getHardwareModel() == 3) {
    elem[9].text = (char*)"fx-CG 50";
  } else {
    elem[9].text = (char*)"Unknown";
  }
  
  elem[10].newLine = 1;
  elem[10].text = (char*)"Real hardware:";
  elem[10].spaceAtEnd=1;
  if(getIsEmulated()) {
    elem[11].text = (char*)"No";
  } else {
    elem[11].text = (char*)"Yes";
  }
  
  elem[12].newLine = 1;
  elem[12].text = (char*)"ABS:";
  elem[12].spaceAtEnd=1;
  elem[13].text = ABSname;
  
  elem[14].newLine = 1;
  elem[14].text = (char*)"ABS date:";
  elem[14].spaceAtEnd=1;
  elem[15].text = ABSdate;
    
  elem[16].newLine = 1;
  elem[16].text = (char*)"CPU PVR:";
  elem[16].spaceAtEnd=1;
  elem[17].text = (char*)pvrstr;
  
  elem[18].newLine = 1;
  elem[18].text = (char*)"CPU PRR:";
  elem[18].spaceAtEnd=1;
  elem[19].text = (char*)prrstr;
  
  elem[20].newLine = 1;
  elem[20].text = (char*)"CPU CVR:";
  elem[20].spaceAtEnd=1;
  elem[21].text = (char*)cvrstr;
  
  elem[22].newLine = 1;
  elem[22].text = (char*)"Device ID:";
  elem[22].spaceAtEnd=1;
  elem[23].text = (char*)devID;

  elem[24].lineSpacing = 5;
  elem[24].newLine = 1;
  elem[24].text = (char*)"Press F1 to see the registered user information log, or EXIT.";

  text.numelements = 25;
  text.allowF1=1;
  while(1) {
    if(doTextArea(&text) == TEXTAREA_RETURN_F1) {
      userInfo();
    } else return;
  }
}

void userInfo() {
  textArea text;
  text.title = (char*)"User Information";

  textElement elem[10240];
  text.elements = elem;
  text.numelements=0;
  elem[text.numelements].text = (char*)"Current information:";
  elem[text.numelements].color = COLOR_BLUE;
  text.numelements++;
  elem[text.numelements].newLine = 1;
  elem[text.numelements].text = (char*)"Username:";
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  // here goes the username field, to be set in the loop.
  int usernamefield = text.numelements;
  text.numelements++;
  elem[text.numelements].newLine = 1;
  elem[text.numelements].text = (char*)"Organization:";
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  // here goes the organization field, to be set in the loop.
  int organizationfield = text.numelements;
  text.numelements++;
  elem[text.numelements].newLine = 1;
  elem[text.numelements].text = (char*)"Password:";
  elem[text.numelements].spaceAtEnd=1;
  text.numelements++;
  // here goes the password field, to be set in the loop.
  int passwordfield = text.numelements;
  text.numelements++;
  elem[text.numelements].newLine = 1;
  elem[text.numelements].lineSpacing = 5;
  elem[text.numelements].text = (char*)"Historical information (oldest first):";
  elem[text.numelements].color = COLOR_BLUE;
  text.numelements++;

  char* flagpointer = (char*)0x80BE0000;
  int counter = 0;
  while(*flagpointer == 0x0F) {
    if(text.numelements < 10240-6) { // minus six because on every run we use six elements.
      elem[text.numelements].lineSpacing = 5;
      elem[text.numelements].newLine = 1;
      elem[text.numelements].text = (char*)"Username:";
      elem[text.numelements].spaceAtEnd=1;
      text.numelements++;
      if(*(flagpointer+0x18) != '\0') {
        elem[text.numelements].text = (flagpointer+0x18);
      } else {
        elem[text.numelements].text = (char*)"[Empty]";
        elem[text.numelements].color = COLOR_LIGHTGRAY;
      }
      text.numelements++;
      elem[text.numelements].newLine = 1;
      elem[text.numelements].text = (char*)"Organization:";
      elem[text.numelements].spaceAtEnd=1;
      text.numelements++;
      if(*(flagpointer+0x04) != '\0') {
        elem[text.numelements].text = (flagpointer+0x04);
      } else {
        elem[text.numelements].text = (char*)"[Empty]";
        elem[text.numelements].color = COLOR_LIGHTGRAY;
      }
      text.numelements++;
      elem[text.numelements].newLine = 1;
      elem[text.numelements].text = (char*)"Password:";
      elem[text.numelements].spaceAtEnd=1;
      text.numelements++;
      if(*(flagpointer+0x2C) != '\0') {
        elem[text.numelements].text = (flagpointer+0x2C);
      } else {
        elem[text.numelements].text = (char*)"[Empty]";
        elem[text.numelements].color = COLOR_LIGHTGRAY;
      }
      text.numelements++;
    }
    flagpointer = flagpointer + 0x40;
    counter++;
  }
  if(!counter) {
    elem[0].text = (char*)"No information has ever been registered.";
    elem[0].color = COLOR_BLACK;
    text.numelements = 1;
  } else {
    flagpointer = flagpointer - 0x40;
    if(*(flagpointer+0x18) != '\0') {
      elem[usernamefield].text = (flagpointer+0x18);
    } else {
      elem[usernamefield].text = (char*)"[Empty]";
      elem[usernamefield].color = COLOR_LIGHTGRAY;
    }
    if(*(flagpointer+0x04) != '\0') {
      elem[organizationfield].text = (flagpointer+0x04);
    } else {
      elem[organizationfield].text = (char*)"[Empty]";
      elem[organizationfield].color = COLOR_LIGHTGRAY;
    }
    if(*(flagpointer+0x2C) != '\0') {
      elem[passwordfield].text = (flagpointer+0x2C);
    } else {
      elem[passwordfield].text = (char*)"[Empty]";
      elem[passwordfield].color = COLOR_LIGHTGRAY;
    }
  }
  doTextArea(&text);
}