#include <M5Unified.h>
#include <M5GFX.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

uint8_t alarm_toConfig = 0;
uint8_t alarm_count = 0;
bool alarm_saved = true;

void alarm_add();
void alarm_remove();
void alarm_config(String name);
void alarm_configNow();
void alarm_chooseTime();
void alarm_setTime();
void alarm_switchWeekday();
void alarm_switchWeekend();
void alarm_switchOnetime();
void alarm_save();

void alarm_init() {
  appUI.reset();
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Alarm");
  appUI.addLocaleToTitle("ja", "アラーム");
  appUI.addItem("save", alarm_save, "Save");
  appUI.addLocaleToItem("save", "ja", "保存");
  if (alarm_saved) {
    appUI.addRightLocaleToItem("save", "en", "Saved");
    appUI.addRightLocaleToItem("save", "ja", "保存済み");
    appUI.setItemRightColor("save", TFT_GREEN);
  } else {
    appUI.addRightLocaleToItem("save", "en", "Not Saved");
    appUI.addRightLocaleToItem("save", "ja", "未保存");
    appUI.setItemRightColor("save", TFT_RED);
  }
  appUI.addItem("add", alarm_add, "+ Add Alarm");
  appUI.addLocaleToItem("add", "ja", "+ アラームを追加");
  uint8_t cnt = 0;
  for( JsonObject loopAlarm : alarmJson.as<JsonArray>() ) {
    int32_t hour = loopAlarm["hour"];
    int32_t min = loopAlarm["min"];
    bool enabled = loopAlarm["weekday"] || loopAlarm["weekend"];
    appUI.addItem(String(cnt), alarm_config, forceDigits(hour, 2)+":"+forceDigits(min, 2));
    if (enabled) appUI.setItemColor(String(cnt), TFT_GRAY);
    cnt++;
  }
  alarm_count = cnt;
  appUI.linkFunctionToBack(appEnd);
  appUI.makeUI(lang);
}

void alarm_add() {
  JsonDocument newAlarm;
  alarm_saved = false;
  newAlarm["hour"] = 6;
  newAlarm["min"] = 0;
  newAlarm["weekday"] = true;
  newAlarm["weekend"] = true;
  newAlarm["onetime"] = true;
  alarmJson.add(newAlarm);
  setRTCAlarmIRQ();
  alarm_config((String) alarm_count);
}

void alarm_remove() {
  alarm_saved = false;
  //Serial.println(alarm_toConfig);
  alarmJson.remove((size_t) alarm_toConfig);
  setRTCAlarmIRQ();
  alarm_init();
}

void alarm_config(String name) {
  alarm_toConfig = name.toInt();
  appUI.reset();
  appUI.setTitle("Edit Alarm");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "アラームを編集");
  appUI.addItem("time", alarm_chooseTime, "Time");
  appUI.addLocaleToItem("time", "ja", "時刻");
  appUI.addRightLocaleToItem("time", "en", forceDigits(alarmJson[alarm_toConfig]["hour"], 2)+":"+forceDigits(alarmJson[alarm_toConfig]["min"], 2));
  appUI.addItem("weekday", alarm_switchWeekday, "Weekday");
  appUI.addLocaleToItem("weekday", "ja", "平日");
  appUI.addRightLocaleToItem("weekday", "en", boolStr(alarmJson[alarm_toConfig]["weekday"], "Enable", "Disable"));
  appUI.addRightLocaleToItem("weekday", "ja", boolStr(alarmJson[alarm_toConfig]["weekday"], "有効", "無効"));
  appUI.addItem("weekend", alarm_switchWeekend, "Weekend");
  appUI.addLocaleToItem("weekend", "ja", "休日");
  appUI.addRightLocaleToItem("weekend", "en", boolStr(alarmJson[alarm_toConfig]["weekend"], "Enable", "Disable"));
  appUI.addRightLocaleToItem("weekend", "ja", boolStr(alarmJson[alarm_toConfig]["weekend"], "有効", "無効"));
  appUI.addItem("onetime", alarm_switchOnetime, "One-time");
  appUI.addLocaleToItem("onetime", "ja", "ワンタイム");
  appUI.addRightLocaleToItem("onetime", "en", boolStr(alarmJson[alarm_toConfig]["onetime"], "Enable", "Disable"));
  appUI.addRightLocaleToItem("onetime", "ja", boolStr(alarmJson[alarm_toConfig]["onetime"], "有効", "無効"));
  appUI.addItem("remove", alarm_remove, "Remove");
  appUI.addLocaleToItem("remove", "ja", "削除");
  appUI.linkFunctionToBack(alarm_init);
  appUI.makeUI(lang);
}

void alarm_configNow() {
  alarm_config((String) alarm_toConfig);
}

void alarm_save() {
  if (!alarm_saved) {
    alarm_saved = true;
    appUI.addRightLocaleToItem("save", "en", "Saved");
    appUI.addRightLocaleToItem("save", "ja", "保存済み");
    appUI.setItemRightColor("save", TFT_GREEN);
    writeSPIJson("/alarm.json", &alarmJson);
    appUI.makeUI(lang);
  }
}

void alarm_setTime() {
  UIAddtional = UI_NOTHING;
  alarm_saved = false;
  cv_uiadditional.deleteSprite();
  alarmJson[alarm_toConfig]["hour"] = UItimeLeft;
  alarmJson[alarm_toConfig]["min"] = UItimeRight;
  setRTCAlarmIRQ();
  alarm_configNow();
}

void alarm_chooseTime() {
  appUI.reset();
  appUI.setTitle("Time");
  appUI.setTransparentMode(true);
  appUI.linkFunctionToBack(alarm_setTime);
  UIAddtional = UI_TIME;
  UIAddtionalSettings = TIMEUI_MODE_HOURMIN;
  cv_uiadditional.createSprite(300, 165);
  UItimeLeft = alarmJson[alarm_toConfig]["hour"];
  UItimeRight = alarmJson[alarm_toConfig]["min"];
  M5.Display.fillRect(0, 64, sizeX, sizeY, TFT_BLACK);
  drawTimeUI();
  appUI.makeUI(lang);
}

void alarm_switchWeekday() {
  alarm_saved = false;
  alarmJson[alarm_toConfig]["weekday"] = !alarmJson[alarm_toConfig]["weekday"];
  appUI.addRightLocaleToItem("weekday", "en", boolStr(alarmJson[alarm_toConfig]["weekday"], "Enable", "Disable"));
  appUI.addRightLocaleToItem("weekday", "ja", boolStr(alarmJson[alarm_toConfig]["weekday"], "有効", "無効"));
  appUI.makeUI(lang);
}

void alarm_switchWeekend() {
  alarm_saved = false;
  alarmJson[alarm_toConfig]["weekend"] = !alarmJson[alarm_toConfig]["weekend"];
  appUI.addRightLocaleToItem("weekend", "en", boolStr(alarmJson[alarm_toConfig]["weekend"], "Enable", "Disable"));
  appUI.addRightLocaleToItem("weekend", "ja", boolStr(alarmJson[alarm_toConfig]["weekend"], "有効", "無効"));
  appUI.makeUI(lang);
}

void alarm_switchOnetime() {
  alarm_saved = false;
  alarmJson[alarm_toConfig]["onetime"] = !alarmJson[alarm_toConfig]["onetime"];
  appUI.addRightLocaleToItem("onetime", "en", boolStr(alarmJson[alarm_toConfig]["onetime"], "Enable", "Disable"));
  appUI.addRightLocaleToItem("onetime", "ja", boolStr(alarmJson[alarm_toConfig]["onetime"], "有効", "無効"));
  appUI.makeUI(lang);
}

void alarm_loop() {
  appUI.update(dateTime, battery, getBatteryVoltage());
}