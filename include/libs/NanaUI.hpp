/*
  [ NanaUI ] by K-Nana
  A simple UI system for M5Stack with touch panel.
  Multilingual support.
*/

#pragma once
#include <M5Unified.h>
#include <M5GFX.h>
#include <map>
#include <list>

typedef void (*pFunc)(void);

typedef void (*pArgFunc)(String);

class UI {
  public:
    void setTitle(String title);
    void setTitle(String title, String lang);
    void makeUI();
    void makeUI(String lang);
    void update();
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
    void setItemColor(String id, int color);
    void setItemRightColor(String id, int color);
    void setLocaleFont(String lang, uint8_t font);
    void setSpace(int space);
    void linkFunctionToBack(pFunc func);
    void linkFunctionToItem(String id, pFunc func);
    void linkArgFunctionToItem(String id, pArgFunc func);
    void setUseArgFunctionToItem(String id, bool useArgFunction);
  private:
    int scroll = 0;
    int16_t scrollAccel[3] = {0, 0, 0};
    int prevX;
    int prevY;
    int firstX;
    int firstY;
    int scrollCount;
    int space = 10;
    pFunc backFunction;
    std::list<String> items;
    std::map<String, std::map<String, String>> itemLocale;
    std::map<String, std::map<String, String>> itemRightLocale;
    std::map<String, int> itemColor;
    std::map<String, int> itemRightColor;
    std::map<String, pFunc> itemFunction;
    std::map<String, pArgFunc> itemArgFunction;
    std::map<String, bool> itemUseArgFunction;
    std::map<String, String> titleLocale;
    std::map<String, uint8_t> localeFont;
    M5Canvas canv = M5Canvas(&M5.Display);
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
  //canv.createSprite(width, (items.size()*rowSize)+rowSize);
  if ((items.size()*rowSize)+rowSize > height) {
    UIHeight = (items.size()*rowSize)+rowSize;
  } else {
    UIHeight = height;
  }
  canv.createSprite(width, UIHeight);
  canv.fillRect(1, 1, width, rowSize, WHITE);
  canv.setTextColor(BLACK, WHITE);
  if (localeFont[lang] == 1) canv.drawCenterString(titleLocale[lang], width/2, space, &fonts::efontJA_24);
  else if (localeFont[lang] == 2) canv.drawCenterString(titleLocale[lang], width/2, space, &fonts::efontCN_24);
  else canv.drawCenterString(titleLocale[lang], width/2, space, &fonts::Font4);
  canv.drawString("<", 10, space, &fonts::Font4);
  int cnt = 0;
  for(auto i = items.begin(); i != items.end(); i++ ) {
    cnt++;
    canv.setTextColor(itemColor[*i], BLACK);
    if (localeFont[lang] == 1) canv.drawString(itemLocale[*i][lang], 10, (cnt*rowSize)+space, &fonts::efontJA_24);
    else if (localeFont[lang] == 2) canv.drawString(itemLocale[*i][lang], 10, (cnt*rowSize)+space, &fonts::efontCN_24);
    else canv.drawString(itemLocale[*i][lang], 10, (cnt*rowSize)+space, &fonts::Font4);
    canv.setTextColor(itemColor[*i], BLACK);
    if (localeFont[lang] == 1) canv.drawRightString(itemRightLocale[*i][lang], width-10, (cnt*rowSize)+space, &fonts::efontJA_24);
    else if (localeFont[lang] == 2) canv.drawRightString(itemRightLocale[*i][lang], width-10, (cnt*rowSize)+space, &fonts::efontCN_24);
    else canv.drawRightString(itemRightLocale[*i][lang], width-10, (cnt*rowSize)+space, &fonts::Font4);
    canv.drawFastHLine(0, (cnt*rowSize), width, DARKGREY);
    canv.drawFastHLine(0, (cnt*rowSize)+rowSize-1, width, DARKGREY);
  }
}

void UI::update() {
  int rowSize = (space*2)+26;
  int height = M5.Display.height();
  int cHeight = canv.height();
  canv.pushSprite(0, 0-scroll);
  if (M5.Touch.getCount() > 0) {
    auto detail = M5.Touch.getDetail();
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
  } else {
    scrollAccel[0] = floor(scrollAccel[0]*0.8);
    scroll += scrollAccel[0];
    if (prevX != -1) {
      int touched = floor((prevY+scroll)/rowSize);
      if (scrollCount == 0) {
        if (touched == 0) {
          if (prevX < 50) backFunction();
        } else {
          String touchedID = *std::next(items.begin(), touched-1);
          if (itemUseArgFunction[touchedID]) {
            itemArgFunction[touchedID](touchedID);
          } else {
            itemFunction[touchedID]();
          }
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
  } else if (scroll > cHeight-height) {
    scroll = cHeight-height;
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
  itemColor[id] = WHITE;
  itemRightColor[id] = LIGHTGREY;
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

void UI::setItemColor(String id, int color) {
  itemColor[id] = color;
}

void UI::setItemRightColor(String id, int color) {
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
