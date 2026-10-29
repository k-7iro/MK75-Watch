#include <M5Unified.h>
#include <M5GFX.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

uint8_t random_number;
uint8_t random_maxNumber;
uint8_t random_prevNumber;
uint8_t random_countdown = 0;

void random_init();
void random_loop();
void random_wheel(String number);

void random_init() {
  appUI.reset();
  UIAddtional = UI_NOTHING;
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Random");
  appUI.addLocaleToTitle("ja", "ランダム");
  for (uint8_t i = 2; i < 11; i++) {
    appUI.addItem(String(i), random_wheel, "1d"+String(i));
  }
  appUI.addItem("100", random_wheel, "1d100");
  appUI.linkFunctionToBack(appEnd);
  appUI.makeUI(lang);
}

void random_loop() {
  appUI.update(dateTime, battery, getBatteryVoltage());
  
  if (UIAddtional == UI_RANDOM) {
    cv_uiadditional.clear();
    cv_uiadditional.setTextColor(TFT_WHITE, TFT_BLACK);
    cv_uiadditional.drawCenterString("1d"+String(random_maxNumber), 150, 10, &fonts::Font4);
    if (M5.Touch.getDetail().wasClicked()) {
      random_number = random(1, random_maxNumber+1);
      random_countdown = 10;
    } else if (random_countdown > 0) {
      random_countdown--;
      uint8_t CDNumber = random(1, random_maxNumber);
      if (CDNumber >= random_prevNumber) CDNumber++;
      cv_uiadditional.setTextColor(TFT_GRAY, TFT_BLACK);
      cv_uiadditional.drawCenterString(String(CDNumber), 150, 80, &fonts::Font6);
      random_prevNumber = CDNumber;
    } else {
      cv_uiadditional.drawCenterString(String(random_number), 150, 80, &fonts::Font6);
    }
    cv_uiadditional.pushSprite(centerX-150, centerY-50);
  }
}

void random_wheel(String number) {
  appUI.reset();
  random_number = number.toInt();
  appUI.setTitle("1d"+number);
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTransparentMode(true);
  UIAddtional = UI_RANDOM;
  random_maxNumber = number.toInt();
  random_number = random(1, random_maxNumber+1);
  random_prevNumber = random(1, random_maxNumber+1);
  random_countdown = 10;
  M5.Display.fillRect(0, 64, sizeX, sizeY, TFT_BLACK);
  cv_uiadditional.createSprite(300, 165);
  appUI.linkFunctionToBack(random_init);
  appUI.makeUI(lang);
}