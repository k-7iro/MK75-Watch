#include <M5Unified.h>
#include <M5GFX.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

bool timer_isMain = true;
uint8_t timer_editing = 0;
long timer_editingMillis = 0;

void timer_init();
void timer_make();
void timer_start();
void timer_edit(String id);
void timer_remove();
void timer_loop();

void timer_init() {
  timer_editing = 0;
  appUI.reset();
  appUI.setTitle("Timer");
  appUI.addLocaleToTitle("ja", "タイマー");
  appUI.setLocaleFont("en", 1);
  appUI.setLocaleFont("ja", 1);
  appUI.addItem("add", timer_make, "+ Add Timer");
  appUI.addLocaleToItem("add", "ja", "+ タイマーを追加");
  uint8_t cnt = 1;
  for(auto i = timers.begin(); i != timers.end(); i++ ) {
    uint32_t remains = (*i - millis())/1000;
    appUI.addItem(String(cnt), timer_edit, "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
    cnt++;
  }
  appUI.linkFunctionToBack(appEnd);
  appUI.makeUI(lang);
}

void timer_make() {
  appUI.reset();
  appUI.setTitle("Set Timer");
  appUI.setLocaleFont("en", 1);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "タイマーをセット");
  appUI.setTransparentMode(true);
  UIAddtional = UI_TIME;
  UIAddtionalSettings = TIMEUI_MODE_MINSEC;
  cv_uiadditional.createSprite(300, 165);
  UItimeLeft = 0;
  UItimeRight = 0;
  M5.Display.fillRect(0, 64, sizeX, sizeY, TFT_BLACK);
  drawTimeUI();
  appUI.linkFunctionToBack(timer_start);
  appUI.makeUI(lang);
}

void timer_start() {
  UIAddtional = UI_NOTHING;
  cv_uiadditional.deleteSprite();
  uint16_t timerTime = ((UItimeLeft*60) + (UItimeRight));
  timers.push_back((timerTime*1000)+millis());
  timer_init();
}

void timer_edit(String id) {
  appUI.reset();
  timer_editing = id.toInt();
  timer_editingMillis = *std::next(timers.begin(), timer_editing-1);
  uint32_t remains = (timer_editingMillis - millis())/1000;
  appUI.setTitle("Edit Timer");
  appUI.setLocaleFont("en", 1);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "タイマーを編集");
  appUI.addItem("timer", nothing, String((uint8_t) floor(remains/60))+":"+String(remains%60));
  appUI.addItem("remove", timer_remove, "Remove Timer");
  appUI.addLocaleToItem("remove", "ja", "タイマーを削除");
  appUI.linkFunctionToBack(timer_init);
  appUI.makeUI(lang);
}

void timer_remove() {
  timers.erase(std::next(timers.begin(), timer_editing-1));
  timer_init();
}

void timer_loop() {
  if (!timers.empty() && timer_isMain) {
    uint8_t cnt = 1;
    for(auto i = timers.begin(); i != timers.end(); i++ ) {
      uint32_t remains = (*i - millis())/1000;
      appUI.addLocaleToItem(String(cnt), "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
      cnt++;
    }
    appUI.makeUI(lang);
  } else if (timer_editing != 0) {
    uint32_t remains = (timer_editingMillis - millis())/1000;
    appUI.addLocaleToItem("timer", "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
    appUI.makeUI(lang);
  }
  appUI.update(dateTime, battery, getBatteryVoltage());
}