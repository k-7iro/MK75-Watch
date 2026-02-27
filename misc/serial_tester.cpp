#include <M5Unified.h>
#include <M5GFX.h>

void setup() {
  auto config = M5.config();
  config.serial_baudrate = 115200;
  M5.begin(config);
  M5.Lcd.println("Serial Testor");
}

void loop() {
  M5.update();
  while (Serial.available()) {
    M5.Lcd.print((char) Serial.read());
  }
  if (M5.Touch.getDetail().wasClicked() > 0) {
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Serial Testor");
  }
}