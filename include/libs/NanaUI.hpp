/*
  [ NanaUI ] by K-Nana
  A simple UI system for M5Stack with touch panel.
  Multilingual support.
  MIT License https://opensource.org/license/mit
*/

#pragma once
#include <M5Unified.h>
#include <M5GFX.h>
#include <map>
#include <list>
#include "libs/NanaTools.hpp"

typedef void (*pFunc)(void);

typedef void (*pArgFunc)(String);

class UI {
  public:
    void setTitle(String title);
    void setTitle(String title, String lang);
    void makeUI();
    void makeUI(String lang);
    void update(m5::rtc_datetime_t dateTime, uint8_t battery);
    void addItem(String id);
    void addItem(String id, String defName);
    void addItem(String id, String defName, String defLang);
    void addItem(String id, pFunc func);
    void addItem(String id, pFunc func, String defName);
    void addItem(String id, pFunc func, String defName, String defLang);
    void addItem(String id, pArgFunc func);
    void addItem(String id, pArgFunc func, String defName);
    void addItem(String id, pArgFunc func, String defName, String defLang);
    void addLocaleToItem(String id, String lang, String name);
    void addRightLocaleToItem(String id, String lang, String name);
    void addLocaleToTitle(String lang, String name);
    void setItemColor(String id, uint32_t color);
    void setItemRightColor(String id, uint32_t color);
    void setLocaleFont(String lang, uint8_t font);
    void setSpace(int space);
    void linkFunctionToBack(pFunc func);
    void linkFunctionToItem(String id, pFunc func);
    void linkArgFunctionToItem(String id, pArgFunc func);
    void setUseArgFunctionToItem(String id, bool useArgFunction);
    void setTransparentMode(bool mode);
  private:
    int scroll = 0;
    int16_t scrollAccel[3] = {0, 0, 0};
    int prevX;
    int prevY;
    int firstX;
    int firstY;
    int scrollCount;
    int space = 20;
    bool transparentMode = false;
    pFunc backFunction;
    std::list<String> items;
    std::map<String, std::map<String, String>> itemLocale;
    std::map<String, std::map<String, String>> itemRightLocale;
    std::map<String, uint32_t> itemColor;
    std::map<String, uint32_t> itemRightColor;
    std::map<String, pFunc> itemFunction;
    std::map<String, pArgFunc> itemArgFunction;
    std::map<String, bool> itemUseArgFunction;
    std::map<String, String> titleLocale;
    std::map<String, uint8_t> localeFont;
    M5Canvas disp = M5Canvas(&M5.Display);
    M5Canvas canv = M5Canvas(&disp);
    M5Canvas top = M5Canvas(&disp);
    M5Canvas top_dtime_bat = M5Canvas(&top);
};

void UI::setTitle(String title) {
  titleLocale["en"] = title;
}

void UI::setTitle(String title, String lang) {
  titleLocale[lang] = title;
}

void UI::makeUI() {
  makeUI("en");
}

void UI::makeUI(String lang) {
  int width = M5.Display.width();
  int height = M5.Display.height();
  int UIHeight;
  int rowSize = (space*2)+26;
  if ((items.size()*rowSize)+rowSize > height) {
    UIHeight = (items.size()*rowSize);
  } else {
    UIHeight = height-48;
  }
  disp.createSprite(width, height);
  disp.fillScreen(TFT_BLACK);
  canv.createSprite(width, UIHeight);
  if (transparentMode) {
    canv.fillScreen(M5.Lcd.color565(16, 8, 16));
  } else {
    canv.fillScreen(TFT_BLACK);
  }
  top.createSprite(width, 48);
  top_dtime_bat.createSprite(width, 17);
  top.fillRect(0, 0, width, rowSize, WHITE);
  top.setTextColor(BLACK, WHITE);
  if (titleLocale.count(lang) > 0) {
    if (localeFont[lang] == 1) top.drawCenterString(titleLocale[lang], width/2, 18, &fonts::efontJA_24);
    else if (localeFont[lang] == 2) top.drawCenterString(titleLocale[lang], width/2, 18, &fonts::efontCN_24);
    else top.drawCenterString(titleLocale[lang], width/2, 18, &fonts::Font4);
  } else {
    if (localeFont["en"] == 1) top.drawCenterString(titleLocale["en"], width/2, 18, &fonts::efontJA_24);
    else if (localeFont["en"] == 2) top.drawCenterString(titleLocale["en"], width/2, 18, &fonts::efontCN_24);
    else top.drawCenterString(titleLocale["en"], width/2, 18, &fonts::Font4);
  }
  uint8_t cnt = 0;
  for(auto i = items.begin(); i != items.end(); i++ ) {
    canv.setTextColor(itemColor[*i], BLACK);
    if (itemLocale[*i].count(lang) > 0) {
      if (localeFont[lang] == 1) canv.drawString(itemLocale[*i][lang], 10, (cnt*rowSize)+space, &fonts::efontJA_24);
      else if (localeFont[lang] == 2) canv.drawString(itemLocale[*i][lang], 10, (cnt*rowSize)+space, &fonts::efontCN_24);
      else canv.drawString(itemLocale[*i][lang], 10, (cnt*rowSize)+space, &fonts::Font4);
    } else {
      if (localeFont[lang] == 1) canv.drawString(itemLocale[*i]["en"], 10, (cnt*rowSize)+space, &fonts::efontJA_24);
      else if (localeFont[lang] == 2) canv.drawString(itemLocale[*i]["en"], 10, (cnt*rowSize)+space, &fonts::efontCN_24);
      else canv.drawString(itemLocale[*i]["en"], 10, (cnt*rowSize)+space, &fonts::Font4);
    }
    canv.setTextColor(itemRightColor[*i], BLACK);
    if (itemRightLocale[*i].count(lang) > 0) {
      if (localeFont[lang] == 1) canv.drawRightString(itemRightLocale[*i][lang], width-10, (cnt*rowSize)+space, &fonts::efontJA_24);
      else if (localeFont[lang] == 2) canv.drawRightString(itemRightLocale[*i][lang], width-10, (cnt*rowSize)+space, &fonts::efontCN_24);
      else canv.drawRightString(itemRightLocale[*i][lang], width-10, (cnt*rowSize)+space, &fonts::Font4);
    } else {
      if (localeFont[lang] == 1) canv.drawRightString(itemRightLocale[*i]["en"], width-10, (cnt*rowSize)+space, &fonts::efontJA_24);
      else if (localeFont[lang] == 2) canv.drawRightString(itemRightLocale[*i]["en"], width-10, (cnt*rowSize)+space, &fonts::efontCN_24);
      else canv.drawRightString(itemRightLocale[*i]["en"], width-10, (cnt*rowSize)+space, &fonts::Font4);
    }
    canv.drawFastHLine(0, (cnt*rowSize), width, DARKGREY);
    canv.drawFastHLine(0, (cnt*rowSize)+rowSize-1, width, DARKGREY);
    cnt++;
  }
}

void UI::update(m5::rtc_datetime_t dateTime, uint8_t battery) {
  uint8_t rowSize = (space*2)+26;
  uint16_t height = M5.Display.height();
  uint16_t width = M5.Display.width();
  uint16_t cHeight = canv.height();
  uint8_t tHeight = top.height();
  uint8_t tdHeight = top_dtime_bat.height();
  top_dtime_bat.clear(BLACK);
  top_dtime_bat.setTextColor(WHITE, BLACK);
  top_dtime_bat.drawString(forceDigits(dateTime.time.hours, 2)+":"+forceDigits(dateTime.time.minutes, 2)+" "+forceDigits(dateTime.time.seconds, 2), 0, 0, &fonts::Font2);
  if (M5.Power.Axp2101.isVBUS()) { top_dtime_bat.setTextColor(CYAN, BLACK); }
  top_dtime_bat.drawRightString(String(battery)+"%", width, 0, &fonts::Font2);
  top_dtime_bat.pushSprite(0, 0);
  canv.pushSprite(0, tHeight-scroll);
  top.pushSprite(0, 0);
  disp.pushSprite(0, 0, M5.Lcd.color565(16, 8, 16));
  if (M5.BtnA.wasPressed()) {
    backFunction();
  } else if (M5.Touch.getCount() > 0) {
    auto detail = M5.Touch.getDetail();
    if (detail.y < 240) {
      if (firstX == -1) {
        firstX = detail.x;
        firstY = detail.y;
      }
      uint8_t accelArrSize = (sizeof(scrollAccel)/sizeof(int16_t));
      if (prevY != -1) {
        for (int i = 0; i < accelArrSize-1; i++) {
          scrollAccel[i] = scrollAccel[i+1];
        }
        scrollAccel[accelArrSize-1] = prevY-detail.y;
        scroll += scrollAccel[accelArrSize-1];
        scrollCount += scrollAccel[accelArrSize-1];
      }
      prevX = detail.x;
      prevY = detail.y;
    }
  } else {
    scrollAccel[0] = floor(scrollAccel[0]*0.8);
    scroll += scrollAccel[0];
    if (prevX != -1) {
      int touched = floor((prevY+scroll)/rowSize)-1;
      if (scrollCount == 0 && items.size() > touched && touched >= 0) {
        String touchedID = *std::next(items.begin(), touched);
        if (itemUseArgFunction[touchedID]) {
          itemArgFunction[touchedID](touchedID);
        } else {
          itemFunction[touchedID]();
        }
      }
    }
    prevX = -1;
    prevY = -1;
    firstX = -1;
    firstY = -1;
    scrollCount = 0;
  }
  if (scroll < 0) {
    scroll = 0;
  } else if (scroll > cHeight-height+tHeight) {
    scroll = cHeight-height+tHeight;
  }
}

void UI::addItem(String id) {
  addItem(id, id);
}

void UI::addItem(String id, String defName) {
  addItem(id, defName, "en");
}

void UI::addItem(String id, String defName, String defLang) {
  items.push_back(id);
  itemLocale[id][defLang] = defName;
  itemRightLocale[id][defLang] = "";
  itemColor[id] = 16777215;
  itemRightColor[id] = 16776960;
  itemUseArgFunction[id] = false;
}

void UI::addItem(String id, pFunc func) {
  addItem(id);
  itemFunction[id] = func;
}

void UI::addItem(String id, pFunc func, String defName) {
  addItem(id, defName);
  itemFunction[id] = func;
}

void UI::addItem(String id, pFunc func, String defName, String defLang) {
  addItem(id, defName, defLang);
  itemFunction[id] = func;
}

void UI::addItem(String id, pArgFunc func) {
  addItem(id);
  itemArgFunction[id] = func;
  itemUseArgFunction[id] = true;
}

void UI::addItem(String id, pArgFunc func, String defName) {
  addItem(id, defName);
  itemArgFunction[id] = func;
  itemUseArgFunction[id] = true;
}

void UI::addItem(String id, pArgFunc func, String defName, String defLang) {
  addItem(id, defName, defLang);
  itemArgFunction[id] = func;
  itemUseArgFunction[id] = true;
}

void UI::addLocaleToItem(String id, String lang, String name) {
  itemLocale[id][lang] = name;
}

void UI::addRightLocaleToItem(String id, String lang, String name) {
  itemRightLocale[id][lang] = name;
}

void UI::addLocaleToTitle(String lang, String name) {
  titleLocale[lang] = name;
}

void UI::setItemColor(String id, uint32_t color) {
  itemColor[id] = color;
}

void UI::setItemRightColor(String id, uint32_t color) {
  itemRightColor[id] = color;
}

void UI::setLocaleFont(String lang, uint8_t font) {
  localeFont[lang] = font;
}

void UI::setSpace(int space) {
  this->space = space;
}

void UI::linkFunctionToBack(pFunc func) {
  backFunction = func;
}

void UI::linkFunctionToItem(String id, pFunc func) {
  itemFunction[id] = func;
}

void UI::linkArgFunctionToItem(String id, pArgFunc func) {
  itemArgFunction[id] = func;
}

void UI::setUseArgFunctionToItem(String id, bool useArgFunction) {
  itemUseArgFunction[id] = useArgFunction;
}

void UI::setTransparentMode(bool mode) {
  transparentMode = mode;
}