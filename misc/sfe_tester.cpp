#include <M5Unified.h>
#include <M5GFX.h>
#include <LittleFS.h>
#include "libs/SerialFileEdit.hpp"

SerialFileEdit SFE(&Serial, &LittleFS);

void setup() {
  auto config = M5.config();
  config.serial_baudrate = 115200;
  M5.begin(config);
  LittleFS.begin(true);
  SFE.begin();
}

void loop() {
  M5.update();
  SFE.update();
}