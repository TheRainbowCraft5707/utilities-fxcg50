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

#include "constantsProvider.hpp"
#include "timeProvider.hpp"
#include "chronoGUI.hpp"
#include "settingsProvider.hpp"
#include "graphicsProvider.hpp"
#include "selectorGUI.hpp"
#include "calendarGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "calendarProvider.hpp" 
#include "keyboardProvider.hpp"
#include "unixtimeExternal.hpp"

int bufmonth = 0; //this is global so that it's easier to make the events count refresh
void viewCalendar() {
  int y = getCurrentYear();
  int m = getCurrentMonth();
  int d = getCurrentDay();
  
  int menu = 1;
  int iresult;
  int eventcount[32];
  //int bufmonth = 0;
  int bufyear = 0;
  while (1)
  {
    if(y>9999||y<0) { y=getCurrentYear(); m=getCurrentMonth(); d=getCurrentDay(); } //protection: reset to today's date if somehow we're trying to access the calendar for an year after 9999, or before 0.
    drawCalendar(y,m,d, GetSetting(SETTING_SHOW_CALENDAR_EVENTS_COUNT), eventcount, &bufmonth, &bufyear);
    switch (menu)
    {
    case 1:
        GetFKeyPtr(0x01FC, &iresult); // JUMP
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x049F, &iresult); // VIEW
        FKey_Display(1, (int*)iresult);
        GetFKeyPtr(0x03B4, &iresult); // INSERT
        FKey_Display(2, (int*)iresult);
        GetFKeyPtr(0x0104, &iresult); // DEL-ALL
        FKey_Display(3, (int*)iresult);
        break;
    case 2:
        GetFKeyPtr(0x0408, &iresult); // |<<
        FKey_Display(0, (int*)iresult);
        GetFKeyPtr(0x0409, &iresult); // <<
        FKey_Display(1, (int*)iresult);
        GetFKeyPtr(0x040B, &iresult); // >>
        FKey_Display(2, (int*)iresult);
        GetFKeyPtr(0x040C, &iresult); // >>|
        FKey_Display(3, (int*)iresult);
        GetFKeyPtr(0x0238, &iresult); // ORIGINAL
        FKey_Display(4, (int*)iresult);
        GetFKeyPtr(0x015F, &iresult); // DATE
        FKey_Display(5, (int*)iresult);
        break;
    }
    int key;
    mGetKey(&key);
    int ny=0, nm=0, nd=0;
    switch(key) {
      case KEY_CTRL_F1:
        if (menu == 1) { menu = 2; }
        else if (menu == 2) { if (y > 0) y--; }
        break;
      case KEY_CTRL_F2:
        if (menu == 2) {
          if (y != 0)
              m--;
          else { if (m!=1) m--; } 
          if (m == 0)
          {
              m = 12;
              y--;
          }
        } else if (menu == 1) {
          viewEvents(y, m, d);
        }
        break;
      case KEY_CTRL_F3:
        if (menu == 2) {
          if (y != 9999)
              m++;
          else { if (m!=12) m++; }
          if (m == 13)
          {
              m = 1;
              y++;
          }
        } else if (menu == 1) {
          if(eventcount[d]+1 > MAX_DAY_EVENTS) {
            AUX_DisplayErrorMessage( 0x2E );
          } else {
            eventEditor(y, m, d);
            bufmonth=0;//force calendar events to reload
          }
        }
        break;
      case KEY_CTRL_F4:
        if (menu == 2) {
          if (y != 9999) y++;
        } else if (menu == 1) {
          deleteAllEventUI(y, m, d);
          bufmonth=0;//force calendar events to reload
        }
        break;
      case KEY_CTRL_F5:
        if (menu == 2) {
          y = getCurrentYear();
          m = getCurrentMonth();
          d = getCurrentDay();
        }
        break;       
      case KEY_CTRL_F6:
        if (menu == 2) { if (0 == chooseCalendarDate(&ny, &nm, &nd, (char*)"  Jump to specific date", (char*)"")) { y=ny;m=nm;d=nd; } } //only update calendar if selection was clean
        break;     
      case KEY_CTRL_UP:
        d-=7;
        if (d < 1)
        {
            m--;
            if (m == 0)
            {
                m = 12;
                y--;
            }
            d = getMonthDays(m) + d + ((m == 2 && isLeap(y)) ? 1 : 0);
        }
        break;
      case KEY_CTRL_DOWN:
        d+=7;
        if (d > getMonthDays(m) + ((m == 2 && isLeap(y)) ? 1 : 0))
        {
            d = d - getMonthDays(m) - ((m == 2 && isLeap(y)) ? 1 : 0);
            m++;
            if (m == 13)
            {
                m = 1;
                y++;
            }
        }
        break;
      case KEY_CTRL_LEFT:
        d--;
        if (d == 0)
        {
            m--;
            if (m == 0)
            {
                m = 12;
                y--;
            }
            d = getMonthDays(m)+((m == 2 && isLeap(y)) ? 1 : 0);
        }
        break;
      case KEY_CTRL_RIGHT:
        d++;
        if (d > getMonthDays(m) + ((m == 2 && isLeap(y)) ? 1 : 0))
        {
            d = 1;
            m++;
            if (m == 13)
            {
                m = 1;
                y++;
            }
        }
        break;
      case KEY_CTRL_EXE:
        viewEvents(y, m, d);
        break;
      case KEY_CTRL_EXIT:
        if (menu == 2) { menu = 1; } else { return; }
        break;
    }
    if (menu == 2) {
      if (d > getMonthDays(m) + ((m == 2 && isLeap(y)) ? 1 : 0))
      {
          d = getMonthDays(m) +((m == 2 && isLeap(y)) ? 1 : 0);
      }
    } 
  }
}

void viewEvents(int y, int m, int d) {
  int res=1;
  Menu menu;
  
  menu.scrollbar=1;
  menu.scrollout=1;
  menu.showtitle=1;
  menu.selection=1;
  menu.scroll=0;
  menu.height=7;
  menu.type=MENUTYPE_FKEYS;
  strcpy(menu.nodatamsg, "No Events - press F2");
  char buffer[15] = "";
  dateToString(buffer, y, m, d, GetSetting(SETTING_DATEFORMAT));
  strcpy(menu.title, "Events for ");
  strcat(menu.title, buffer);
  strcpy(menu.statusText, "");
  while(res) {
    res = viewEventsSub(&menu, y, m, d);
  }
}

int viewEventsSub(Menu* menu, int y, int m, int d) {
  //returns 1 when it wants to be restarted (refresh tasks)
  //returns 0 if the idea really is to exit the screen
  EventDate thisday;
  thisday.day = d; thisday.month = m; thisday.year = y;  
  
  menu->numitems = GetEventsForDate(&thisday, CALENDARFOLDER, NULL); //get event count only so we know how much to alloc
  CalendarEvent* events = (CalendarEvent*)alloca(menu->numitems*sizeof(CalendarEvent));
  MenuItem* menuitems = (MenuItem*)alloca(menu->numitems*sizeof(MenuItem));
  menu->numitems = GetEventsForDate(&thisday, CALENDARFOLDER, events);
  int curitem = 0;
  while(curitem <= menu->numitems-1) {
    strcpy(menuitems[curitem].text, (char*)events[curitem].title);
    menuitems[curitem].type = MENUITEM_NORMAL;
    menuitems[curitem].value = events[curitem].repeat;
    menuitems[curitem].color = events[curitem].category-1;
    curitem++;
  }
  menu->items=menuitems; 

  Bdisp_AllClr_VRAM();
  DisplayStatusArea(); 
  
  int iresult;
  if (menu->fkeypage == 0) {
    if(menu->numitems>0) {
      GetFKeyPtr(0x049F, &iresult); // VIEW
      FKey_Display(0, (int*)iresult);
    }
    GetFKeyPtr(0x03B4, &iresult); // INSERT
    FKey_Display(1, (int*)iresult);
    if(menu->numitems>0) {
      GetFKeyPtr(0x0185, &iresult); // EDIT
      FKey_Display(2, (int*)iresult);
      GetFKeyPtr(0x0038, &iresult); // DELETE
      FKey_Display(3, (int*)iresult);
      GetFKeyPtr(0x0104, &iresult); // DEL-ALL
      FKey_Display(4, (int*)iresult);
    }
  } else if (menu->fkeypage == 1) {
    if(menu->numitems>0) {
      GetFKeyPtr(0x038D, &iresult); // COPY
      FKey_Display(0, (int*)iresult);
      GetFKeyPtr(0x04D2, &iresult); // MOVE
      FKey_Display(1, (int*)iresult);
      GetFKeyPtr(0x0470, &iresult); // MEMO
      FKey_Display(2, (int*)iresult);
    } else { menu = 0; } //because if there are no events, 2nd menu is empty.
  }
  if(menu->numitems>0) {
    GetFKeyPtr(0x0006, &iresult); // Rotate FKey menu arrow
    FKey_Display(5, (int*)iresult);
  }
  if(menu->selection > menu->numitems) menu->selection = menu->numitems;
  if(menu->selection < 1) menu->selection = 1;
  int res = doMenu(menu);
  switch(res) {
    case MENU_RETURN_EXIT:
      return 0;
      break;
    case KEY_CTRL_F1:
      if(menu->fkeypage == 0) { if(menu->numitems > 0) viewEvent(&events[menu->selection-1]); }
      else if (menu->fkeypage == 1) {
        copyEvent(&events[menu->selection-1]);
        bufmonth = 0;
      }
      break;
    case MENU_RETURN_SELECTION:
      if(menu->numitems > 0) viewEvent(&events[menu->selection-1]);
      break;
    case KEY_CTRL_F2:
      if(menu->fkeypage == 0) {
        if(menu->numitems >= MAX_DAY_EVENTS) {
          AUX_DisplayErrorMessage( 0x2E );
        } else {
          eventEditor(y, m, d);
          bufmonth=0;
        }
      } else if (menu->fkeypage == 1) {
        moveEvent(&events[menu->selection-1], menu->selection-1);
#ifdef WAITMSG
        PrintXY(1,8,(char*)"  Please wait.....      ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); Bdisp_PutDisp_DD();
#endif
        bufmonth=0;
        return 0;
      }
      break;
    case KEY_CTRL_F3:
      if(menu->fkeypage == 0) {
        if(menu->numitems > 0) {
          if(eventEditor(y, m, d, EVENTEDITORTYPE_EDIT, &events[menu->selection-1]) == EVENTEDITOR_RETURN_CONFIRM) {
            ReplaceEventFile(&events[menu->selection-1].startdate, events, CALENDARFOLDER, menu->numitems);
          }
        }
      } else if (menu->fkeypage == 1) {
        setEventChrono(&events[menu->selection-1]);
      }
      break;
    case KEY_CTRL_F4:
      if(menu->fkeypage == 0) {
        if(menu->numitems > 0) { deleteEventUI(y,m,d,menu->selection-1, 0); bufmonth=0; }
      }
      break;
    case KEY_CTRL_F5:
      if(menu->fkeypage == 0) {
        if(menu->numitems > 0) { deleteAllEventUI(y, m, d, 0); bufmonth=0; }
      }
      break;
    case KEY_CTRL_F6:
      if (menu->fkeypage == 0) {
        if(menu->numitems > 0) menu->fkeypage = 1;
      } else menu->fkeypage = 0;
      break;
  }
  return 1;
}

void viewEvent(CalendarEvent* event) {
  char buffer[15];
  textArea text;
  strcpy(text.title, (char*)event->title);
  
  textElement elem[10];
  text.elements = elem;
  
  elem[0].text = (char*)"Location:";
  elem[0].color=COLOR_LIGHTGRAY;
  elem[0].spaceAtEnd=1;
  
  elem[1].text = (char*)event->location;
  
  elem[2].text = (char*)"Starts on";
  elem[2].newLine = 1;
  elem[2].lineSpacing = 8;
  elem[2].spaceAtEnd=1;
  elem[2].color=COLOR_LIGHTGRAY; 
  
  unsigned char startson[50] = "";
  strcpy(buffer, (char*)"");
  dateToString((char*)buffer, event->startdate.year, event->startdate.month, event->startdate.day, GetSetting(SETTING_DATEFORMAT));
  strcpy((char*)startson, buffer);
  strcat((char*)startson, (char*)" ");
  
  if(event->timed) {
    strcpy(buffer, (char*)"");
    timeToString((char*)buffer, event->starttime.hour, event->starttime.minute, event->starttime.second, GetSetting(SETTING_TIMEFORMAT));
    strcat((char*)startson, (char*)buffer);
  } else {
    strcat((char*)startson, (char*)"(all day)");
  }
  
  elem[3].text = (char*)startson;
  
  elem[4].text = (char*)"Ends on";
  elem[4].newLine = 1;
  elem[4].spaceAtEnd=1;
  elem[4].color=COLOR_LIGHTGRAY; 
  
  unsigned char endson[50] = "";
  strcpy(buffer, (char*)"");
  dateToString((char*)buffer, event->enddate.year, event->enddate.month, event->enddate.day, GetSetting(SETTING_DATEFORMAT));
  strcpy((char*)endson, buffer);
  strcat((char*)endson, (char*)" ");
  
  if(event->timed) {
    strcpy(buffer, (char*)"");
    timeToString((char*)buffer, event->endtime.hour, event->endtime.minute, event->endtime.second, GetSetting(SETTING_TIMEFORMAT));
    strcat((char*)endson, (char*)buffer);
  }
  
  elem[5].text = (char*)endson;
  
  elem[6].text = (char*)"Category:";
  elem[6].color=COLOR_LIGHTGRAY;
  elem[6].newLine = 1;
  elem[6].spaceAtEnd=1;
  
  strcpy(buffer, (char*)"");
  itoa(event->category, (unsigned char*)buffer);
  elem[7].text = (char*)buffer;
  elem[7].color = textColorToFullColor((event->category == 0 ? 7 : event->category-1));
  
  elem[8].text = (char*)"Description:";
  elem[8].newLine = 1;
  elem[8].lineSpacing = 8;
  elem[8].color=COLOR_LIGHTGRAY;
  
  elem[9].text = (char*) event->description;
  elem[9].newLine = 1;
  
  text.numelements = 10;
  doTextArea(&text);
}

void fillInputDate(int yr, int m, int d, char* buffer) {
  buffer[0] = '\0'; // This sets the first character to \0, also represented by "", an empty string
  char buffer2[8] = "";
  if(yr != 0 || m != 0 || d != 0) {
    switch(GetSetting(SETTING_DATEFORMAT)) {
      case 0:
        if (d < 10) { strcat(buffer, "0"); }
        itoa(d, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (m < 10) { strcat(buffer, "0"); }
        itoa(m, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (yr < 1000) { strcat(buffer, "0"); }
        if (yr < 100) { strcat(buffer, "0"); }
        if (yr < 10) { strcat(buffer, "0"); }
        itoa(yr, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        break;
      case 1:
        if (m < 10) { strcat(buffer, "0"); }
        itoa(m, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (d < 10) { strcat(buffer, "0"); }
        itoa(d, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (yr < 1000) { strcat(buffer, "0"); }
        if (yr < 100) { strcat(buffer, "0"); }
        if (yr < 10) { strcat(buffer, "0"); }
        itoa(yr, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        break;
      case 2:
        if (yr < 1000) { strcat(buffer, "0"); }
        if (yr < 100) { strcat(buffer, "0"); }
        if (yr < 10) { strcat(buffer, "0"); }
        itoa(yr, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
    
        if (m < 10) { strcat(buffer, "0"); }
        itoa(m, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        
        if (d < 10) { strcat(buffer, "0"); }
        itoa(d, (unsigned char*) buffer2);
        strcat(buffer, buffer2);
        break;
    }
  }
}

void fillInputTime(int h, int m, int s, char* buffer) {
  buffer[0] = '\0'; // This sets the first character to \0, also represented by "", an empty string
  char buffer2[8] = ""; // Some string length
  if(h != 0 || m != 0 || s != 0) {
    if (h < 10) { strcat(buffer, "0"); }
    itoa(h, (unsigned char*) buffer2);
    strcat(buffer, buffer2);

    if (m < 10) { strcat(buffer, "0"); }
    itoa(m, (unsigned char*) buffer2);
    strcat(buffer, buffer2);
    
    if (s < 10) { strcat(buffer, "0"); }
    itoa(s, (unsigned char*) buffer2);
    strcat(buffer, buffer2);
  }
}
int eventEditor(int y, int m, int d, int type, CalendarEvent* event, int istask) {
  // returns 0 if success, 1
  if(type == EVENTEDITORTYPE_ADD) {
    event = (CalendarEvent*)alloca(sizeof(CalendarEvent));
    
    event->startdate.day = d;
    event->startdate.month = m;
    event->startdate.year = y;
    
    event->enddate.day = 0;
    event->enddate.month = 0;
    event->enddate.year = 0;
    
    event->starttime.hour = 0;
    event->starttime.minute = 0;
    event->starttime.second = 0;
    
    event->endtime.hour = 0;
    event->endtime.minute = 0;
    event->endtime.second = 0;
    
    //clean buffers:
    strcpy((char*)event->title, "");
    strcpy((char*)event->location, "");
    strcpy((char*)event->description, "");
  }
  if(istask) event->timed=0;
  int curstep = 0;
  while(1) {
    Bdisp_AllClr_VRAM();
    if (type == EVENTEDITORTYPE_ADD) {
      SetBackGround(0x0A);
      if(istask) PrintXY(1, 1, (char*)"  Add New Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
      else PrintXY(1, 1, (char*)"  Add New Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    } else {
      SetBackGround(6);
      if(istask) PrintXY(1, 1, (char*)"  Edit Task", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
      else PrintXY(1, 1, (char*)"  Edit Event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
    }
    DisplayStatusArea();
    switch(curstep) {
      case 0:
        if(1) { // this allows for declaring things inside the switch case without the compiler complaining
          PrintXY(1, 2, (char*)"  Title:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          int iresult;
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          PrintXY(1, 3, (char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); // remove aestethically unpleasing bit of background at the end of the field
          
          textInput input;
          input.x=1;
          input.y=3;
          input.forcetext=1; //force text so title must be at least one char.
          input.charlimit=21;
          input.acceptF6=1;
          input.buffer = (char*)event->title;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) { curstep=curstep+1; break;} // continue to next step
          }
        }
        break;
      case 1:
        if(1) {
          PrintXY(1, 2, (char*)"  Location:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          PrintXY(1, 3, (char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
          
          textInput input;
          input.x=1;
          input.y=3;
          input.charlimit=128;
          input.acceptF6=1;
          input.buffer = (char*)event->location;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) { curstep=curstep+1; break;} // continue to next step
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
          }
        }
        break;
      case 2:
        if(1) {
          PrintXY(1, 2, (char*)"  Description:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          PrintXY(1, 3, (char*)"                        ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
          
          textInput input;
          input.x=1;
          input.y=3;
          input.charlimit=1024;
          input.acceptF6=1;
          input.buffer = (char*)event->description;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) { curstep=curstep+1; break;} // continue to next step
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
          }
        }
        break;
      case 3:
        if(!istask) {
          PrintXY(1, 2, (char*)"  Start time:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(1, 3, (char*)"  Time: ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(6, 4, (char*)"  HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(1, 5, (char*)"  F2: All-day event", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          
          textInput input;
          input.x=6;
          input.y=3;
          input.width=6;
          input.charlimit=6;
          input.acceptF6=1;
          input.type=INPUTTYPE_TIME;
          char stbuffer[15] = "";
          strcpy(stbuffer, (char*)"");
          if(type == EVENTEDITORTYPE_EDIT) fillInputTime(event->starttime.hour, event->starttime.minute, event->starttime.second, stbuffer);
          input.buffer = (char*)stbuffer;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) {
              char hour[3] = "";
              char minute[3] = "";
              char second[3] = "";
              hour[0] = stbuffer[0]; hour[1] = stbuffer[1]; hour[2] = '\0';
              minute[0] = stbuffer[2]; minute[1] = stbuffer[3]; minute[2] = '\0';
              second[0] = stbuffer[4]; second[1] = stbuffer[5]; second[2] = '\0';

              int h = sys_atoi(hour);
              int m = sys_atoi(minute);
              int s = sys_atoi(second);
              if(isTimeValid(h, m, s) && (int)strlen(stbuffer) == input.charlimit) {
                event->timed = 1;
                event->starttime.hour = h;
                event->starttime.minute = m;
                event->starttime.second = s;
                curstep=curstep+1; break; // continue to next step
              } else {
                invalidFieldMsg(1);
              }
            } 
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F2) {
              // user wants all-day event
              event->timed = 0;
              curstep=curstep+1; break; // next step
            }
          }
        } else {
          curstep=curstep+1;
        }
        break;
      case 4:
        if(!istask) {
          PrintXY(1, 2, (char*)"  End date:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(1, 3, (char*)"  Date: ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          switch(GetSetting(SETTING_DATEFORMAT)) {
            case 0:
              PrintXY(6, 4, (char*)"  DDMMYYYY", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              break;
            case 1:
              PrintXY(6, 4, (char*)"  MMDDYYYY", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              break;
            case 2:
              PrintXY(6, 4, (char*)"  YYYYMMDD", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              break;
          }
          PrintXY(1, 5, (char*)"  F2:Same as start date", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          
          textInput input;
          input.x=6;
          input.y=3;
          input.width=8;
          input.charlimit=8;
          input.acceptF6=1;
          input.type=INPUTTYPE_DATE;
          char edbuffer[15] = "";
          strcpy(edbuffer, (char*)"");
          if(type == EVENTEDITORTYPE_EDIT) fillInputDate(event->enddate.year, event->enddate.month, event->enddate.day, edbuffer);
          input.buffer = (char*)edbuffer;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) {
              char year[6] = "";
              char month[3] = "";
              char day[3] = "";
              switch(GetSetting(SETTING_DATEFORMAT)) {
                case 0:
                  day[0] = edbuffer[0]; day[1] = edbuffer[1]; day[2] = '\0';
                  month[0] = edbuffer[2]; month[1] = edbuffer[3]; month[2] = '\0';
                  year[0] = edbuffer[4]; year[1] = edbuffer[5]; year[2] = edbuffer[6]; year[3] = edbuffer[7]; year[4] = '\0';
                  break;
                case 1:
                  day[0] = edbuffer[2]; day[1] = edbuffer[3]; day[2] = '\0';
                  month[0] = edbuffer[0]; month[1] = edbuffer[1]; month[2] = '\0';
                  year[0] = edbuffer[4]; year[1] = edbuffer[5]; year[2] = edbuffer[6]; year[3] = edbuffer[7]; year[4] = '\0';
                  break;
                case 2:
                  day[0] = edbuffer[6]; day[1] = edbuffer[7]; day[2] = '\0';
                  month[0] = edbuffer[4]; month[1] = edbuffer[5]; month[2] = '\0';
                  year[0] = edbuffer[0]; year[1] = edbuffer[1]; year[2] = edbuffer[2]; year[3] = edbuffer[3]; year[4] = '\0';
                  break;
              }

              int yr = sys_atoi(year);
              int m = sys_atoi(month);
              int d = sys_atoi(day);
              if(isDateValid(yr, m, d) && (int)strlen(edbuffer) == input.charlimit) {
                event->enddate.year = yr;
                event->enddate.month = m;
                event->enddate.day = d;
                curstep=curstep+1; break; // continue to next step
              } else {
                invalidFieldMsg(0);
              }
            } 
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F2) {
              // user wants end date to be the same as the start date
              event->enddate.year = event->startdate.year;
              event->enddate.month = event->startdate.month;
              event->enddate.day = event->startdate.day;
              curstep=curstep+1; break; // next step
            }
          }
        } else {
          curstep=curstep+1;
        }
        break;
      case 5:
        if(event->timed) {
          PrintXY(1, 2, (char*)"  End time:", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(1, 3, (char*)"  Time: ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(6, 4, (char*)"  HHMMSS", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          int iresult;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A3, &iresult); // Next
          FKey_Display(5, (int*)iresult);
          
          textInput input;
          input.x=6;
          input.y=3;
          input.width=6;
          input.charlimit=6;
          input.acceptF6=1;
          input.type=INPUTTYPE_TIME;
          char etbuffer[15] = "";
          strcpy(etbuffer, (char*)"");
          if(type == EVENTEDITORTYPE_EDIT) fillInputTime(event->endtime.hour, event->endtime.minute, event->endtime.second, etbuffer);
          input.buffer = (char*)etbuffer;
          while(1) {
            input.key=0;
            int res = doTextInput(&input);
            if (res==INPUT_RETURN_EXIT) return EVENTEDITOR_RETURN_EXIT; // user aborted
            else if (res==INPUT_RETURN_CONFIRM) {
              char hour[3] = "";
              char minute[3] = "";
              char second[3] = "";
              hour[0] = etbuffer[0]; hour[1] = etbuffer[1]; hour[2] = '\0';
              minute[0] = etbuffer[2]; minute[1] = etbuffer[3]; minute[2] = '\0';
              second[0] = etbuffer[4]; second[1] = etbuffer[5]; second[2] = '\0';

              int h = sys_atoi(hour);
              int m = sys_atoi(minute);
              int s = sys_atoi(second);
              if(isTimeValid(h, m, s) && (int)strlen(etbuffer) == input.charlimit) {
                event->endtime.hour = h;
                event->endtime.minute = m;
                event->endtime.second = s;
                curstep=curstep+1; break; // continue to next step
              } else {
                invalidFieldMsg(1);
              }
            } 
            else if (res==INPUT_RETURN_KEYCODE && input.key==KEY_CTRL_F1) { curstep=curstep-1; break; }
          }
        } else { curstep=curstep+1; break; } //skip this step
        break;
      case 6:
        if(1) {
          PrintXY(1, 2, (char*)"  Select category", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
          PrintXY(5, 4, (char*)"  \xe6\x92", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow up
          PrintXY(5, 6, (char*)"  \xe6\x93", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_PURPLE); //arrow down
          int iresult, inscreen=1; int key = 0;
          GetFKeyPtr(0x036F, &iresult); // <
          FKey_Display(0, (int*)iresult);
          GetFKeyPtr(0x04A4, &iresult); // Finish
          FKey_Display(5, (int*)iresult);
          char buffer1[20];
          char buffer2[20];
          if(type == EVENTEDITORTYPE_ADD) event->category = 1;
          while(inscreen)
          {
            PrintXY(5, 5, (char*)"   ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); //clear line
            strcpy(buffer1, "  ");
            itoa(event->category, (unsigned char*)buffer2);
            strcat(buffer1, buffer2);
            PrintXY(5, 5, buffer1, TEXT_MODE_TRANSPARENT_BACKGROUND, (event->category <= 6 ? event->category-1 : TEXT_COLOR_YELLOW));
            mGetKey(&key);
            switch(key)
            {
              case KEY_CTRL_DOWN:
                if (event->category == 0) {
                  event->category = 7;
                } else {
                  event->category--;
                }
                break;
              case KEY_CTRL_UP:
                if (event->category == 7) {
                  event->category = 0;
                } else {
                  event->category++;
                }
                break;
              case KEY_CTRL_F1:
                if(istask) curstep = 2; // go to description
                else curstep=curstep-(event->timed?1:2);
                goto endofloop;
                break;
              case KEY_CTRL_EXE:
              case KEY_CTRL_F6:
                inscreen = 0;
                break;
              case KEY_CTRL_EXIT: return EVENTEDITOR_RETURN_EXIT;
            }
          }
          event->daterange = 0;
          event->repeat = 0;
          event->dayofweek = dow(event->startdate.day, event->startdate.month, event->startdate.year);
          if(type == EVENTEDITORTYPE_ADD) {
            int res = AddEvent(event, CALENDARFOLDER);
          
            if(res > 0) {
              MsgBoxPush(4);
              if (res == 4) {
                PrintXY(3, 2, (char*)"  Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              } else {
                if(istask) PrintXY(3, 2, (char*)"  Task add. ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
                else PrintXY(3, 2, (char*)"  Event add. ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              }
              if(istask) PrintXY(3, 3, (char*)"  Task could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              else PrintXY(3, 3, (char*)"  Event could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              PrintXY(3, 4, (char*)"  be added.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
              PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
              int key,inscreen=1;
              while(inscreen) {
                mGetKey(&key);
                switch(key)
                {
                  case KEY_CTRL_EXIT:
                    inscreen=0;
                    break;
                }
              }
              MsgBoxPop();
            }
            return EVENTEDITOR_RETURN_EXIT; // error is like exit
          } else {
            // if we're editing an event, do not save anything to storage and just act as an editor for what's in RAM.
            // saving to storage is something that must be dealt with after this function returns.
            return EVENTEDITOR_RETURN_CONFIRM;
          }
        }
        endofloop:
        break;
      default:
        return EVENTEDITOR_RETURN_EXIT; // non-existent step specified, return
    }
  }
}


void drawCalendar(int year, int month, int d, int show_event_count, int* eventcount, int* bufmonth, int* bufyear) {
    Bdisp_AllClr_VRAM();
    DisplayStatusArea();
    int textX = 0;
    int textY = 0;
#define TOP 25
#define BOTTOM (LCD_HEIGHT_PX - TOP-2)
#define LEFT 25
#define RIGHT (LCD_WIDTH_PX - 26)
#define THICKNESS 20
#define TOPOFFSET 22
#define WIDTH 47
    int myyellow = drawRGB24toRGB565(249, 248, 199);
    drawLine(LEFT,TOP,RIGHT-1,TOP,COLOR_BLACK);
    drawLine(LEFT,TOP,LEFT,BOTTOM-1,COLOR_BLACK);
    drawLine(LEFT,BOTTOM-1,RIGHT-1,BOTTOM-1,COLOR_BLACK);
    drawLine(RIGHT-1,BOTTOM-1,RIGHT-1,TOP,COLOR_BLACK);
    drawRectangle(LEFT+2,TOP+2,RIGHT-2-2-LEFT,THICKNESS,COLOR_BLACK);
    textX=LEFT+5; textY= TOP+2-TOPOFFSET;
    PrintMini(&textX, &textY, (unsigned char*)getMonthAsString(month), 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
    int x,y = 0; int k = 1;
    drawRectangle(LEFT+2,TOP+2+THICKNESS,RIGHT-2-2-LEFT,THICKNESS,COLOR_LIME);
    drawRectangle(RIGHT-2-WIDTH,TOP+2+2*THICKNESS,WIDTH,THICKNESS*6,COLOR_AQUA);
    drawRectangle(LEFT+2,TOP+2+2*THICKNESS,WIDTH,THICKNESS*6,COLOR_AQUA);
    drawLine(LEFT+2,TOP+2+THICKNESS,RIGHT-3,TOP+2+THICKNESS,COLOR_BLACK);
    for (x = LEFT + 2+WIDTH; x < RIGHT - WIDTH;x+=WIDTH)
    {
        drawLine(x,TOP+2+THICKNESS,x,BOTTOM-3, COLOR_BLACK);
    }
    for (y = TOP+2+2*THICKNESS; y < BOTTOM - THICKNESS; y+=THICKNESS)
    {
        drawLine(LEFT+2,y,RIGHT-3,y,COLOR_BLACK);
    }
    for (x = LEFT+2; x < RIGHT - WIDTH; x+= WIDTH)
    {
      textX = x+2;
      textY = TOP+2+THICKNESS-TOPOFFSET;
      PrintMini(&textX, &textY, (unsigned char*)getDOWAsShortString(k++), 0, 0xFFFFFFFF, 0, 0, COLOR_BLUE, COLOR_LIME, 1, 0);
    }

    int startingday = dow(year,month,1),day = 1;
    int prevstart = getMonthDays((month == 1 ? 12 : month - 1)) - (startingday == 0 ? 7 : startingday) + ((month == 3 && isLeap(year)) ? 2 : 1);
    //int prevstart = monthDays[(month == 1 ? 11 : month - 2)] - (startingday == 0 ? 7 : startingday) + ((month == 3 && isLeap(year)) ? 2 : 1);
    char buffer[10];
    if (startingday != 0) { //solve "overlapping days on months started on a sunday" bug
      for (x = 0; x < (startingday == 0 ? 7 : startingday); x++)
      {
          itoa(prevstart++,(unsigned char*)buffer);
          PrintMiniFix(LEFT+2+x*WIDTH+2,TOP+2+2*THICKNESS-TOPOFFSET,buffer,0,(x == 0 ? COLOR_LIGHTSLATEGRAY : COLOR_LIGHTGRAY),(x == 0 ? COLOR_AQUA : COLOR_WHITE));
      }
    }
    x = startingday;
    y = 2;
    while (day <= getMonthDays(month) + (month == 2 && isLeap(year)) ? 1 : 0)
    {
        itoa(day,(unsigned char*)buffer);
        if (day == d) {
          drawRectangle(LEFT+2+WIDTH*x+1,TOP+1+2+y*THICKNESS,WIDTH-1,THICKNESS-1,COLOR_RED);
          if(x == 0) drawRectangle(LEFT+2+WIDTH*x,TOP+1+2+y*THICKNESS,WIDTH,THICKNESS-1,COLOR_RED); //make sure the little pixels row on sundays is filled
        }
        PrintMiniFix(LEFT+2+WIDTH*x+2,TOP+2+y*THICKNESS-TOPOFFSET,buffer,0,(day == d ? COLOR_WHITE : COLOR_BLACK),(day == d ? COLOR_RED : (x == 0 || x == 6) ? COLOR_AQUA : COLOR_WHITE));
        //events indicator:            
        if (show_event_count) {
          if (*bufmonth!=month || *bufyear!=year) { //events in buffer are not for this month, refresh.
            GetEventCountsForMonth(year, month, eventcount);
            *bufmonth = month; //update which month is now in buffer
            *bufyear = year; //update which year is now in buffer
          }
          if(eventcount[day] > 0) {
            int textX = LEFT+2+WIDTH*x+2+12*2+2; //12+2 to have space to write the day and some padding
            int textY = TOP+2+y*THICKNESS-TOPOFFSET+2; //+2 to have some padding
            unsigned char eventstr[10] = "";
            if (eventcount[day] < 100) {
              itoa(eventcount[day], (unsigned char*)eventstr); 
            } else {
              strcpy((char*)eventstr, (char*)"++");
            }
            PrintMiniMini( &textX, &textY, (unsigned char*)eventstr, 0, TEXT_COLOR_BLACK, 0 );   
            if(day == d) {
              // color replacements for when background is red
              VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+2+24, 8*2, 12, COLOR_WHITE, COLOR_RED);
              VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+2+24, 8*2, 12, COLOR_BLACK, COLOR_WHITE);
            }
            else if(x == 0 || x == 6) {
              VRAMReplaceColorInRect(LEFT+2+WIDTH*x+2+12*2+2, TOP+2+y*THICKNESS-TOPOFFSET+2+24, 8*2, 12, COLOR_WHITE, myyellow);
              VRAMReplaceColorInRect(LEFT+2+WIDTH*x, TOP+1+2+y*THICKNESS, WIDTH, THICKNESS-1, COLOR_AQUA, myyellow);
            } else {
              VRAMReplaceColorInRect(LEFT+2+WIDTH*x, TOP+1+2+y*THICKNESS, WIDTH, THICKNESS-1, COLOR_WHITE, myyellow);
            }
          }
        }
        //end of events indicator
        
        x++;
        day++;
        if (x == 7)
        {
            x = 0;
            y++;
        }
    }
    day = 1;
    while (y != 8)
    {
        itoa(day++,(unsigned char*)buffer);
        PrintMiniFix(LEFT+2+WIDTH*x+2,TOP+2+y*THICKNESS-TOPOFFSET,buffer,0,((x == 0) | (x == 6)) ? COLOR_LIGHTSLATEGRAY : COLOR_LIGHTGRAY,(x == 0 || x == 6) ? COLOR_AQUA : COLOR_WHITE);
        x++;
        if (x == 7)
        {
            x = 0;
            y++;
        }
    }
    itoa(year,(unsigned char*)buffer);
    textX = 0; //RIGHT-5-50;
    textY = TOP+2-TOPOFFSET;
    PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 0, 0);
    textX = RIGHT-4-textX;
    PrintMini(&textX, &textY, (unsigned char*)buffer, 0, 0xFFFFFFFF, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
}


void deleteEventUI(int y, int m, int d, int pos, int istask) {
  EventDate date; date.day = d; date.month = m; date.year = y;
  MsgBoxPush(4);
  PrintXY(3, 2, (char*)"  Delete the", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  if(istask) {
    PrintXY(3, 3, (char*)"  Selected Task?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    PrintXY(3, 3, (char*)"  Selected Event?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  PrintXY_2(TEXT_MODE_NORMAL, 1, 4, 3, TEXT_COLOR_BLACK); // yes, F1
  PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 4, TEXT_COLOR_BLACK); // no, F6
  int key,inscreen=1;
  while(inscreen) {
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_F1:
        RemoveEvent(&date, pos, CALENDARFOLDER);
        inscreen=0;
        break;
      case KEY_CTRL_F6:
      case KEY_CTRL_EXIT:
        inscreen=0;
        break;
    }
  }
  MsgBoxPop();
}

void deleteAllEventUI(int y, int m, int d, int istask) {
  EventDate date; date.day = d; date.month = m; date.year = y;
  MsgBoxPush(4);
  if (istask) {
    PrintXY(3, 2, (char*)"  Delete All Tasks?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  } else {
    PrintXY(3, 2, (char*)"  Delete All Events", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY(3, 3, (char*)"  on Selected Day?", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  PrintXY_2(TEXT_MODE_NORMAL, 1, 4, 3, TEXT_COLOR_BLACK); // yes, F1
  PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 4, TEXT_COLOR_BLACK); // no, F6
  int key,inscreen=1;
  while(inscreen) {
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_F1:
        RemoveDay(&date, CALENDARFOLDER);
        inscreen=0;
        break;
      case KEY_CTRL_F6:
      case KEY_CTRL_EXIT:
        inscreen=0;
        break;
    }
  }
  MsgBoxPop();
}

int chooseCalendarDate(int *yr, int *m, int *d, char* message, char* message2)
{ //returns 0 on success, 1 on user abort
  Bdisp_AllClr_VRAM();
  SetSetupSetting( (unsigned int)0x14, 0); //we only accept numbers, so switch off alpha/shift
  DisplayStatusArea();
  PrintXY(1, 1, (char*)message, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLUE);
  PrintXY(1, 2, (char*)message2, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY(1, 3, (char*)"  Date: ", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);  
  switch(GetSetting(SETTING_DATEFORMAT)) {
    case 0:
      PrintXY(6, 4, (char*)"  DDMMYYYY", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      break;
    case 1:
      PrintXY(6, 4, (char*)"  MMDDYYYY", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      break;
    case 2:
      PrintXY(6, 4, (char*)"  YYYYMMDD", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      break;
  }
  
  textInput input;
  input.x=6;
  input.y=3;
  input.width=8;
  input.charlimit=8;
  input.acceptF6=0;
  input.type=INPUTTYPE_DATE;
  char buffer[15] = "";
  strcpy(buffer, (char*)"");
  fillInputDate(*yr, *m, *d, buffer);
  input.buffer = (char*)buffer;
  while(1) {
    input.key=0;
    int res = doTextInput(&input);
    if (res==INPUT_RETURN_EXIT) return 1; // user aborted
    else if (res==INPUT_RETURN_CONFIRM) {
      char year[6] = "";
      char month[3] = "";
      char day[3] = "";
      switch(GetSetting(SETTING_DATEFORMAT)) {
        case 0:
          day[0] = buffer[0]; day[1] = buffer[1]; day[2] = '\0';
          month[0] = buffer[2]; month[1] = buffer[3]; month[2] = '\0';
          year[0] = buffer[4]; year[1] = buffer[5]; year[2] = buffer[6]; year[3] = buffer[7]; year[4] = '\0';
          break;
        case 1:
          day[0] = buffer[2]; day[1] = buffer[3]; day[2] = '\0';
          month[0] = buffer[0]; month[1] = buffer[1]; month[2] = '\0';
          year[0] = buffer[4]; year[1] = buffer[5]; year[2] = buffer[6]; year[3] = buffer[7]; year[4] = '\0';
          break;
        case 2:
          day[0] = buffer[6]; day[1] = buffer[7]; day[2] = '\0';
          month[0] = buffer[4]; month[1] = buffer[5]; month[2] = '\0';
          year[0] = buffer[0]; year[1] = buffer[1]; year[2] = buffer[2]; year[3] = buffer[3]; year[4] = '\0';
          break;
      }

      *yr = sys_atoi(year);
      *m = sys_atoi(month);
      *d = sys_atoi(day);
      if(isDateValid(*yr, *m, *d) && (int)strlen(buffer) == input.charlimit) {
        return 0;
      } else {
        invalidFieldMsg(0);
      }
    } 
  }
}

void copyEvent(CalendarEvent* event) {
  int ey=0, em=0, ed=0;
  int chooseres = chooseCalendarDate(&ey, &em, &ed, (char*) "  Copy Event", (char*)"  To following day:");
  if(chooseres == 0) {
    if(ey == (signed)event->startdate.year && em == (signed)event->startdate.month && ed == (signed)event->startdate.day) {
      return; //destination date is same as current event date
    }
    event->startdate.day = ed;
    event->startdate.month = em;
    event->startdate.year = ey;
    if(GetEventsForDate(&event->startdate, CALENDARFOLDER, NULL)+1 > MAX_DAY_EVENTS) {
      AUX_DisplayErrorMessage( 0x2E );
    } else {
      //already checked if passes num limit
      int res = AddEvent(event, CALENDARFOLDER);
      if(res > 0) {
        MsgBoxPush(4);
        if (res == 4) { //error on size check
          PrintXY(3, 2, (char*)"  Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        } else {
          PrintXY(3, 2, (char*)"  Event copy ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        }
        PrintXY(3, 3, (char*)"  Event could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY(3, 4, (char*)"  be copied.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
        int key,inscreen=1;
        while(inscreen) {
          mGetKey(&key);
          switch(key)
          {
            case KEY_CTRL_EXIT:
              inscreen=0;
              break;
          }
        }
        MsgBoxPop();
      }
    }
    return;
  }  
}

void moveEvent(CalendarEvent* event, int pos) {
  int ey=0, em=0, ed=0;
  int chooseres = chooseCalendarDate(&ey, &em, &ed, (char*) "  Move Event", (char*)"  To following day:");
  if(chooseres == 0) {
    if(ey == (signed)event->startdate.year && em == (signed)event->startdate.month && ed == (signed)event->startdate.day) {
      return; //destination date is same as current event date
    }
    EventDate oldstartdate;
    oldstartdate.day = event->startdate.day;
    oldstartdate.month = event->startdate.month;
    oldstartdate.year = event->startdate.year;
    event->startdate.day = ed;
    event->startdate.month = em;
    event->startdate.year = ey;
    // add event on new day
    if(GetEventsForDate(&event->startdate, CALENDARFOLDER, NULL)+1 > MAX_DAY_EVENTS) {
      AUX_DisplayErrorMessage( 0x2E );
    } else {
      //already checked if passes num limit
      int res = AddEvent(event, CALENDARFOLDER);
      if(res > 0) {
        MsgBoxPush(4);
        if (res == 4) { //error on size check
          PrintXY(3, 2, (char*)"  Filesize ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        } else {
          PrintXY(3, 2, (char*)"  Event move ERROR", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        }
        PrintXY(3, 3, (char*)"  Event could not", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY(3, 4, (char*)"  be moved.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
        PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
        int key,inscreen=1;
        while(inscreen) {
          mGetKey(&key);
          switch(key)
          {
            case KEY_CTRL_EXIT:
              inscreen=0;
              break;
          }
        }
        MsgBoxPop();
      }
    }
    // delete event on current (old) day
    RemoveEvent(&oldstartdate, pos, CALENDARFOLDER);
#ifdef WAITMSG
    PrintXY(1,8,(char*)"  Please wait....       ", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK); Bdisp_PutDisp_DD();
#endif
  } 
}

void invalidFieldMsg(int istime) {
  MsgBoxPush(3);
  if(istime) PrintXY(3, 3, (char*)"  Invalid time.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  else PrintXY(3, 3, (char*)"  Invalid date.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
  int key,inscreen=1;
  while(inscreen) {
    mGetKey(&key);
    switch(key)
    {
      case KEY_CTRL_EXIT:
      case KEY_CTRL_AC:
        inscreen=0;
        break;
    }
  }
  MsgBoxPop(); 
}

void setEventChrono(CalendarEvent* event) {
  // set a downwards chrono that ends on the start time of the event
  // like a event reminder
  
  // ask user which chrono to set
  Selector sel;
  strcpy(sel.title, "Set event reminder");
  strcpy(sel.subtitle, "Select chrono");
  sel.value = 1;
  sel.min = 1;
  sel.max = NUMBER_OF_CHRONO;
  sel.cycle = 1;
  sel.type = SELECTORTYPE_NORMAL;
  int res = doSelector(&sel);
  if (res == SELECTOR_RETURN_EXIT) return;
  // get unix*1000 time of the event's start time/date
  long long int estart = DateTime2Unix(event->startdate.year, event->startdate.month, event->startdate.day, event->starttime.hour, event->starttime.minute, event->starttime.second, 0);
  // get chrono duration (difference between event start time and current time)
  
  long long int duration = estart - currentUnixTime();
  if(duration < 0) {
    // event is in the past, abort
    MsgBoxPush(3);
    PrintXY(3, 3, (char*)"  Event starts in", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY(3, 4, (char*)"  the past.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
    int key,inscreen=1;
    while(inscreen) {
      mGetKey(&key);
      switch(key)
      {
        case KEY_CTRL_EXIT:
        case KEY_CTRL_AC:
          inscreen=0;
          break;
      }
    }
    MsgBoxPop();
  } else {
    // set downwards chrono with the calculated duration
    res = setChronoExternal(sel.value-1, duration, CHRONO_TYPE_DOWN);
    if(res) {
      // success setting a chrono
      MsgBoxPush(3);
      PrintXY(3, 3, (char*)"  Event reminder", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      PrintXY(3, 4, (char*)"  set successfully.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
      int key,inscreen=1;
      while(inscreen) {
        mGetKey(&key);
        switch(key)
        {
          case KEY_CTRL_EXIT:
          case KEY_CTRL_AC:
            inscreen=0;
            break;
        }
      }
      MsgBoxPop();
    } else {
      // timer is busy
      MsgBoxPush(3);
      PrintXY(3, 3, (char*)"  Selected chrono", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      PrintXY(3, 4, (char*)"  is not clear.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
      PrintXY_2(TEXT_MODE_NORMAL, 1, 5, 2, TEXT_COLOR_BLACK); // press exit message
      int key,inscreen=1;
      while(inscreen) {
        mGetKey(&key);
        switch(key)
        {
          case KEY_CTRL_EXIT:
          case KEY_CTRL_AC:
            inscreen=0;
            break;
        }
      }
      MsgBoxPop();
    }
  }
}