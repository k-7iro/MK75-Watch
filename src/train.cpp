#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

String timetableName = "";
bool train_isMainUI = false;
int32_t train_refleshTimer;
bool isWeekend;
bool isRemainingMode = false;

void train_config();
void train_switch_week();
void train_switch_mode();

void train_init() {
  train_isMainUI = true;
  train_refleshTimer = 0;
  appUI.reset();
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Train");
  appUI.addLocaleToTitle("ja", "時刻表");
  appUI.addItem("config", train_config, "Set Timetable");
  appUI.addLocaleToItem("config", "ja", "時刻表を設定");
  appUI.addItem("switch_week", train_switch_week);
  appUI.addItem("switch_mode", train_switch_mode);
  if (isRemainingMode) {
    appUI.addLocaleToItem("switch_mode", "en", "Rem. Minutes");
    appUI.addLocaleToItem("switch_mode", "ja", "残り時間（分）");
  } else {
    appUI.addLocaleToItem("switch_mode", "en", "Time");
    appUI.addLocaleToItem("switch_mode", "ja", "時刻");
  }
  if (dateTime.date.weekDay == 0 || dateTime.date.weekDay == 6 || isHoliday(dateTime.date.month, dateTime.date.date, dateTime.date.year, dateTime.date.weekDay)) {
    isWeekend = true;
    appUI.addLocaleToItem("switch_week", "en", "Weekend");
    appUI.addLocaleToItem("switch_week", "ja", "休日");
  } else {
    isWeekend = false;
    appUI.addLocaleToItem("switch_week", "en", "Weekday");
    appUI.addLocaleToItem("switch_week", "ja", "平日");
  }
  appUI.addItem("train1", nothing);
  appUI.addItem("train2", nothing);
  appUI.addItem("train3", nothing);
  appUI.linkFunctionToBack(appEnd);
  appUI.makeUI(lang);
}

void train_switch_week() {
  if (isWeekend) {
    isWeekend = false;
    appUI.addLocaleToItem("switch_week", "en", "Weekday");
    appUI.addLocaleToItem("switch_week", "ja", "平日");
  } else {
    isWeekend = true;
    appUI.addLocaleToItem("switch_week", "en", "Weekend");
    appUI.addLocaleToItem("switch_week", "ja", "休日");
  }
  appUI.makeUI(lang);
}

void train_switch_mode() {
  if (isRemainingMode) {
    isRemainingMode = false;
    appUI.addLocaleToItem("switch_mode", "en", "Time");
    appUI.addLocaleToItem("switch_mode", "ja", "時刻");
  } else {
    isRemainingMode = true;
    appUI.addLocaleToItem("switch_mode", "en", "Rem. Minutes");
    appUI.addLocaleToItem("switch_mode", "ja", "残り時間（分）");
  }
  appUI.makeUI(lang);
}

void train_setTimetable(String name) {
  timetableName = name;
  train_init();
  train_isMainUI = true;
}

void train_config() {
  train_isMainUI = false;
  appUI.reset();
  appUI.setLocaleFont("en", 1);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Set Timetable");
  appUI.addLocaleToTitle("ja", "時刻表を設定");
  for ( JsonPair loopTimetableName : trainJson["timetable"].as<JsonObject>() ) {
    const char* nameBuffer = loopTimetableName.key().c_str();
    appUI.addItem(nameBuffer, train_setTimetable);
  }
  appUI.linkFunctionToBack(train_init);
  appUI.makeUI(lang);
}

void train_loop() {
  if (train_refleshTimer == 0 && train_isMainUI) {
    if (timetableName == "") {
      appUI.addLocaleToItem("train1", "en", "If a timetable is set, ");
      appUI.addLocaleToItem("train2", "en", "the next train or");
      appUI.addLocaleToItem("train3", "en", "bus appears here.");
      appUI.addLocaleToItem("train1", "ja", "時刻表を設定すると");
      appUI.addLocaleToItem("train2", "ja", "ここに次の電車やバスを");
      appUI.addLocaleToItem("train3", "ja", "表示できます。");
      appUI.setItemColor("train1", TFT_GRAY);
      appUI.setItemColor("train2", TFT_GRAY);
      appUI.setItemColor("train3", TFT_GRAY);
      appUI.addRightLocaleToItem("train1", "en", "");
      appUI.addRightLocaleToItem("train2", "en", "");
      appUI.addRightLocaleToItem("train3", "en", "");
    } else {
      uint8_t min[3] = {60, 60, 60};
      String dest[3] = {"?", "?", "?"};
      String type[3] = {"?", "?", "?"};
      int8_t hourAdd[3] = {-1, -1, -1};
      JsonDocument timetable;
      if (isWeekend) {
        timetable = trainJson["timetable"][timetableName]["weekends"];
      } else {
        timetable = trainJson["timetable"][timetableName]["weekdays"];
      }
      String StrJson = timetable[5];
      for ( const JsonObject loopTimetable : timetable[String(dateTime.time.hours)].as<JsonArray>() ) {
        uint8_t timetableMin = loopTimetable["m"];
        if (timetableMin > dateTime.time.minutes) {
          String dest_buffer = loopTimetable["d"];
          String type_buffer = loopTimetable["t"];
          if (timetableMin < min[0]) {
            min[0] = timetableMin;
            dest[0] = dest_buffer;
            type[0] = type_buffer;
            hourAdd[0] = 0;
          } else if (timetableMin < min[1]) {
            min[1] = timetableMin;
            dest[1] = dest_buffer;
            type[1] = type_buffer;
            hourAdd[1] = 0;
          } else if (timetableMin < min[2]) {
            min[2] = timetableMin;
            dest[2] = dest_buffer;
            type[2] = type_buffer;
            hourAdd[2] = 0;
          }
        }
      }
      int32_t loopHourAdd = 0;
      while (min[2] == 60) {
        loopHourAdd = (loopHourAdd+1)%24;
        for ( const JsonObject loopTimetable : timetable[String(dateTime.time.hours+loopHourAdd)].as<JsonArray>() ) {
          uint8_t timetableMin = loopTimetable["m"];
          String dest_buffer = loopTimetable["d"];
          String type_buffer = loopTimetable["t"];
          if (timetableMin < min[0] && (hourAdd[0] == -1 || hourAdd[0] == loopHourAdd)) {
            min[0] = timetableMin;
            dest[0] = dest_buffer;
            type[0] = type_buffer;
            hourAdd[0] = loopHourAdd;
          } else if (timetableMin < min[1] && (hourAdd[1] == -1 || hourAdd[1] == loopHourAdd)) {
            min[1] = timetableMin;
            dest[1] = dest_buffer;
            type[1] = type_buffer;
            hourAdd[1] = loopHourAdd;
          } else if (timetableMin < min[2] && (hourAdd[2] == -1 || hourAdd[2] == loopHourAdd)) {
            min[2] = timetableMin;
            dest[2] = dest_buffer;
            type[2] = type_buffer;
            hourAdd[2] = loopHourAdd;
          }
        }
      }
      uint8_t colors[3][3] = {{255, 255, 255}, {255, 255, 255}, {255, 255, 255}};
      for (uint8_t i = 0; i < 3; i++) {
        if (!trainJson["color"][type[i]].isNull()) {
          for (uint8_t j = 0; j < 3; j++) {
            colors[i][j] = trainJson["color"][type[i]][j];
          }
        }
      }
      for (uint8_t i = 0; i < 3; i++) {
        uint16_t color = M5.Display.color565(colors[i][0], colors[i][1], colors[i][2]);
        appUI.setItemColor("train" + String(i + 1), color);
        appUI.addLocaleToItem("train" + String(i + 1), "en", type[i]+" "+dest[i]);
        if (isRemainingMode) {
          appUI.addRightLocaleToItem("train" + String(i + 1), "en", String(min[i]-dateTime.time.minutes+(hourAdd[i]*60)));
        } else {
          appUI.addRightLocaleToItem("train" + String(i + 1), "en", String(dateTime.time.hours+hourAdd[i])+":"+forceDigits(min[i], 2));
        }
      }
    }
    appUI.makeUI(lang);
  }
  train_refleshTimer++;
  if (train_refleshTimer >= 10) {
    train_refleshTimer = 0;
  }
  appUI.update(dateTime, battery, getBatteryVoltage());
}