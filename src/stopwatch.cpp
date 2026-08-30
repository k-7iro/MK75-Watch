#include <M5Unified.h>
#include <M5GFX.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

long stwts[5] = {0, 0, 0, 0, 0};
long stwts_stop[5] = {0, 0, 0, 0, 0};
int32_t stwt_swipe = 0;
uint8_t stwt_touchedID = 0;
uint16_t stwt_touchedTime = 0;
bool stwt_afterReset = false;
bool stwt_wasTouched = false;
uint8_t stwt_chosenAtScroll = 0;
uint8_t stwt_nextChosenAtScroll = 0;

void stopwatch_redraw(M5Canvas& target, uint8_t id) {
  uint8_t min;
  uint8_t sec;
  uint16_t milli;
  if (stwts[id] == 0) {
    min = 0;
    sec = 0;
    milli = 0;
  } else if (stwts_stop[id] == 0) {
    min = floor((millis()-stwts[id])/60000);
    sec = floor((millis()-stwts[id])/1000);
    sec = sec%60;
    milli = (millis()-stwts[id])%1000;
  } else {
    min = floor((stwts_stop[id]-stwts[id])/60000);
    sec = floor((stwts_stop[id]-stwts[id])/1000);
    sec = sec%60;
    milli = (stwts_stop[id]-stwts[id])%1000;
  }
  target.clear();
  if (sec%2 == 0){
    target.fillCircle(85, 85, 80, TFT_WHITE);
    target.fillCircle(85, 85, 75, TFT_BLACK);
    if (milli != 0) target.fillArc(85, 85, 80, 75, 270, (milli*0.36)-90, MAGENTA);
  } else {
    target.fillCircle(85, 85, 80, MAGENTA);
    target.fillCircle(85, 85, 75, TFT_BLACK);
    if (milli != 0) target.fillArc(85, 85, 80, 75, 270, (milli*0.36)-90, TFT_WHITE);
  }
  if (stwt_touchedTime != 0) {
    target.fillArc(85, 85, 80, 75, 270, (stwt_touchedTime*0.36)-90, BLUE);
  }
  target.setFont(&fonts::Font4);
  target.setTextColor(TFT_WHITE, TFT_BLACK);
  target.drawCenterString(forceDigits(min, 2)+":"+forceDigits(sec, 2)+"."+forceDigits(milli, 3), 85, 65);
  target.drawCenterString((String) (id+1), 85, 95);
}

void stopwatch_make(M5Canvas& target, uint8_t id) {
  target.createSprite(170, 170);
  stopwatch_redraw(target, id);
}

void stopwatch_init() {
  cv_display.clear();
  cv_stwt_top.createSprite(sizeX, 47);
  cv_stwt_top.fillRect(0, 0, sizeX, 47, TFT_WHITE);
  cv_stwt_top.setTextColor(TFT_BLACK, TFT_WHITE);
  if (strcmp(lang, "ja") == 0) {
    cv_stwt_top.drawCenterString("ストップウォッチ", sizeX/2, 9, &fonts::efontJA_24);
  } else {
    cv_stwt_top.drawCenterString("Stopwatch", sizeX/2, 9, &fonts::Font4);
  }
  uint16_t lightRed = M5.Display.color565(0xff, 0xaa, 0xaa);
  cv_stwt_top.fillRect(0, 0, 54, 56, lightRed);
  cv_stwt_top.setTextColor(BLACK, lightRed);
  cv_stwt_top.drawString("<", 20, 10, &fonts::Font4);
  cv_stwt_top.pushSprite(0, 17);
  stwt_swipe = 0;
  stwt_chosenAtScroll = 0;
  stopwatch_make(cv_stwt1, 0);
  stopwatch_make(cv_stwt2, 1);
  stopwatch_make(cv_stwt3, 2);
  stopwatch_make(cv_stwt4, 3);
  stopwatch_make(cv_stwt5, 4);
}

void stopwatch_loop() {
  updateDateTimeBat();
  cv_dtime_bat.clear(TFT_BLACK);
  cv_dtime_bat.setTextColor(TFT_WHITE, TFT_BLACK);
  cv_dtime_bat.drawString(forceDigits(dateTime.time.hours, 2)+":"+forceDigits(dateTime.time.minutes, 2)+" "+forceDigits(dateTime.time.seconds, 2), 0, 0, &fonts::Font2);
  if (M5.Power.Axp2101.isVBUS()) { cv_dtime_bat.setTextColor(CYAN, TFT_BLACK); }
  cv_dtime_bat.drawRightString(String(battery)+"%", sizeX, 0, &fonts::Font2);
  cv_dtime_bat.pushSprite(0, 0);
  cv_stwt_top.pushSprite(0, 17);
  m5::touch_detail_t detail = M5.Touch.getDetail();
  if (M5.BtnA.wasPressed()) {
    appEnd();
  } else if (M5.Touch.getCount() > 0 or detail.wasClicked()) {
    stwt_chosenAtScroll = stwt_nextChosenAtScroll;
    if (detail.y > 64) {
      scrollsWhenTouch(detail, &stwt_swipe);
      if (inLimit(detail.x, (sizeX/2)-85, (sizeX/2)+85) and inLimit(detail.y, (sizeY/2)-60, (sizeY/2)+110) and !stwt_afterReset and (stwt_swipe % sizeX) == 0) {
        stwt_touchedID = round(stwt_swipe/sizeX);
        stwt_touchedTime += prevLoopTime;
        if (!stwt_wasTouched) {
          if (stwts[stwt_touchedID] == 0) {
            stwts[stwt_touchedID] = millis();
          } else if (stwts_stop[stwt_touchedID] == 0) {
            stwts_stop[stwt_touchedID] = millis();
          } else {
            stwts[stwt_touchedID] += millis()-stwts_stop[stwt_touchedID];
            stwts_stop[stwt_touchedID] = 0;
          }
        }
        stwt_wasTouched = true;
        if (stwt_touchedTime > 1000) {
          stwts[stwt_touchedID] = 0;
          stwts_stop[stwt_touchedID] = 0;
          stwt_touchedTime = 0;
          stwt_afterReset = true;
        }
      }
    } else if ((detail.y > 16) && (detail.wasClicked())) {
      if (detail.x <= 34) {
        appEnd();
      }
    }
  } else {
    scrollsWhenNotTouch(&stwt_swipe, 5, sizeX);
    stwt_wasTouched = false;
    stwt_afterReset = false;
    stwt_touchedTime = 0;
  }
  if (inLimit(stwt_swipe, (sizeX*-1)+1, (sizeX*1)-1)) {
    stopwatch_redraw(cv_stwt1, 0);
    cv_stwt1.pushSprite((sizeX/2)-85-stwt_swipe, (sizeY/2)-50);
  }
  if (inLimit(stwt_swipe, (sizeX*0)+1, (sizeX*2)-1)) {
    stopwatch_redraw(cv_stwt2, 1);
    cv_stwt2.pushSprite((sizeX/2)+235-stwt_swipe, (sizeY/2)-50);
  }
  if (inLimit(stwt_swipe, (sizeX*1)+1, (sizeX*3)-1)) {
    stopwatch_redraw(cv_stwt3, 2);
    cv_stwt3.pushSprite((sizeX/2)+555-stwt_swipe, (sizeY/2)-50);
  }
  if (inLimit(stwt_swipe, (sizeX*2)+1, (sizeX*4)-1)) {
    stopwatch_redraw(cv_stwt4, 3);
    cv_stwt4.pushSprite((sizeX/2)+875-stwt_swipe, (sizeY/2)-50);
  }
  if (inLimit(stwt_swipe, (sizeX*3)+1, (sizeX*5)-1)) {
    stopwatch_redraw(cv_stwt5, 4);
    cv_stwt5.pushSprite((sizeX/2)+1195-stwt_swipe, (sizeY/2)-50);
  }
  cv_display.pushSprite(0, 0);
}