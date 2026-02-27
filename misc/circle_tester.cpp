#include <M5Unified.h>
#include <M5GFX.h>
#include "libs/NanaDrawPlus.hpp"

void setup() {
  M5.begin();
  M5.Display.setBrightness(63);
  Serial.begin(115200);
  int timer = millis();
  M5.Display.fillCircle(160, 120, 100, WHITE);
  Serial.print("Default Circle: ");
  Serial.print(millis()-timer);
  Serial.println("ms");
  M5.Display.clear();
  M5.Display.fillRect(0, 0, 50, 50, mixColor(WHITE, BLACK, 0.5));
  timer = millis();
  drawCircleWithAAOld(&M5.Lcd, 160, 120, 100, BLUE, BLACK);
  Serial.print("Circle with AA (Old): ");
  Serial.print(millis()-timer);
  Serial.println("ms");
  timer = millis();
  drawCircleWithAA(&M5.Lcd, 160, 120, 100, WHITE, BLACK);
  Serial.print("Circle with AA: ");
  Serial.print(millis()-timer);
  Serial.println("ms");
}

void loop() {
  M5.update();
}
