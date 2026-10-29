#include <M5Unified.h>
#include <M5GFX.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

void edev_init();
void edev_loop();

void edev_init() {
  appUI.reset();
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("External Devices");
  appUI.addLocaleToTitle("ja", "外部デバイス");
  appUI.addItem("reload", nothing, "WIP");
  appUI.addLocaleToItem("reload", "ja", "開発中");
  appUI.linkFunctionToBack(appEnd);
  appUI.makeUI(lang);
}

void edev_loop() {
  appUI.update(dateTime, battery, getBatteryVoltage());
}