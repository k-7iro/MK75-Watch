#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <Preferences.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

void settings_init();
void settings_chooseLang();
void settings_setLang(String langTarget);
void settings_dateTime();
void settings_chooseTime();
void settings_setTime();
void settings_chooseDate();
void settings_setDate();
void settings_chooseYear();
void settings_powerBack();
void settings_powerChoose();
void settings_display();
void settings_chooseBrightness();
void settings_setBrightness(String brightness);
void settings_chooseStyle();
void settings_setStyle(String style);
void settings_notice();
void settings_chooseVolume();
void settings_setVolume(String volume);
void settings_switchVibration();
void settings_testNotice();
void settings_power(String chargeOrBattery);
void settings_chooseVoltage();
void settings_switchShowVoltage();
void settings_setVoltage(String voltage);
void settings_chooseCurrent();
void settings_setCurrent(String current);
void settings_switchSleep();
void settings_chooseExtPower();
void settings_setExtPower(String extMode);
void settings_chooseSleep(String touchOrGyro);
void settings_setSleep(String slpTime);
void settings_setAutoShutdown();
void settings_chooseStyle();
void settings_setStyle(String style);
void settings_info();
void settings_setYear(String year);
void settings_save();
void settings_syncTime();
void settings_loop();

bool settings_saved = true;
bool settings_choseBat = false;
bool settings_choseGyro = false;

void settings_init() {
  appUI.reset();
  appUI.setTitle("Settings");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "設定");
  appUI.addItem("save", settings_save, "Save");
  appUI.addLocaleToItem("save", "ja", "保存");
  if (settings_saved) {
    appUI.addRightLocaleToItem("save", "en", "Saved");
    appUI.addRightLocaleToItem("save", "ja", "保存済み");
    appUI.setItemRightColor("save", TFT_GREEN);
  } else {
    appUI.addRightLocaleToItem("save", "en", "Not Saved");
    appUI.addRightLocaleToItem("save", "ja", "未保存");
    appUI.setItemRightColor("save", TFT_RED);
  }
  appUI.addItem("lang", settings_chooseLang, "Change Language");
  appUI.addLocaleToItem("lang", "ja", "言語変更");
  appUI.addItem("power", settings_powerChoose, "Power Settings");
  appUI.addLocaleToItem("power", "ja", "電源設定");
  appUI.addItem("time", settings_dateTime, "Date and Time");
  appUI.addLocaleToItem("time", "ja", "日付と時刻");
  appUI.addItem("display", settings_display, "Display and Visuals");
  appUI.addLocaleToItem("display", "ja", "ディスプレイと外観");
  appUI.addItem("notice", settings_notice, "Notices");
  appUI.addLocaleToItem("notice", "ja", "通知");
  appUI.addItem("verinfo", settings_info, "Infomation");
  appUI.addLocaleToItem("verinfo", "ja", "情報");
  appUI.linkFunctionToBack(appEnd);
  appUI.makeUI(lang);
}

void settings_chooseLang() {
  appUI.reset();
  appUI.setTitle("Change Language");
  appUI.addItem("en", settings_setLang, "English");
  appUI.addItem("ja", settings_setLang, "Japanese");
  appUI.linkFunctionToBack(settings_init);
  appUI.makeUI();
}

void settings_setLang(String langTarget) {
  lang[0] = langTarget.charAt(0);
  lang[1] = langTarget.charAt(1);
  settings_saved = false;
  settings_init();
}

void settings_dateTime() {
  appUI.reset();
  appUI.setTitle("Date and Time");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "日付と時刻");
  appUI.linkFunctionToBack(settings_init);
  appUI.addItem("synctime", settings_syncTime, "Sync Time from NTP");
  appUI.addLocaleToItem("synctime", "ja", "NTPから時刻を同期");
  appUI.addItem("year", settings_chooseYear, "Change Year");
  appUI.addLocaleToItem("year", "ja", "年を変更");
  appUI.addItem("date", settings_chooseDate, "Change Date");
  appUI.addLocaleToItem("date", "ja", "日付を変更");
  appUI.addItem("time", settings_chooseTime, "Change Time");
  appUI.addLocaleToItem("time", "ja", "時間を変更");
  appUI.makeUI(lang);
}

void settings_chooseYear() {
  appUI.reset();
  appUI.setTitle("Year Settings");
  appUI.addLocaleToTitle("ja", "年設定");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.linkFunctionToBack(settings_dateTime);
  for (uint16_t i = 2020; i < 2050; i++) {
    appUI.addItem(String(i), settings_setYear, (String) i);
  }
  appUI.makeUI(lang);
}

void settings_setYear(String year) {
  UIAddtional = UI_NOTHING;
  cv_uiadditional.deleteSprite();
  m5::rtc_date_t date;
  date.year = year.toInt();
  date.month = dateTime.date.month;
  date.weekDay = dateToWeekday(date.year, dateTime.date.month, dateTime.date.weekDay);
  date.date = dateTime.date.date;
  M5.Rtc.setDate(date);
  setRTCAlarmIRQ();
  settings_init();
}

void settings_chooseDate() {
  appUI.reset();
  appUI.setTitle("Date Settings");
  appUI.addLocaleToTitle("ja", "日付設定");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTransparentMode(true);
  appUI.linkFunctionToBack(settings_setDate);
  UIAddtional = UI_TIME;
  UIAddtionalSettings = TIMEUI_MODE_DATE;
  cv_uiadditional.createSprite(300, 165);
  UItimeLeft = dateTime.date.month;
  UItimeRight = dateTime.date.date;
  M5.Display.fillRect(0, 64, sizeX, sizeY, TFT_BLACK);
  drawTimeUI();
  appUI.makeUI(lang);
}

void settings_setDate() {
  UIAddtional = UI_NOTHING;
  cv_uiadditional.deleteSprite();
  m5::rtc_date_t date;
  date.year = dateTime.date.year;
  date.month = UItimeLeft;
  date.weekDay = dateToWeekday(dateTime.date.year, UItimeLeft, UItimeRight);
  date.date = UItimeRight;
  M5.Rtc.setDate(date);
  setRTCAlarmIRQ();
  settings_init();
}

void settings_chooseTime() {
  appUI.reset();
  appUI.setTitle("Time Settings");
  appUI.addLocaleToTitle("ja", "時間設定");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTransparentMode(true);
  appUI.linkFunctionToBack(settings_setTime);
  UIAddtional = UI_TIME;
  UIAddtionalSettings = TIMEUI_MODE_HOURMIN;
  cv_uiadditional.createSprite(300, 165);
  UItimeLeft = dateTime.time.hours;
  UItimeRight = dateTime.time.minutes;
  M5.Display.fillRect(0, 64, sizeX, sizeY, TFT_BLACK);
  drawTimeUI();
  appUI.makeUI(lang);
}

void settings_setTime() {
  UIAddtional = UI_NOTHING;
  cv_uiadditional.deleteSprite();
  m5::rtc_time_t time;
  time.hours = UItimeLeft;
  time.minutes = UItimeRight;
  time.seconds = dateTime.time.seconds;
  M5.Rtc.setTime(time);
  setRTCAlarmIRQ();
  settings_init();
}

void settings_display() {
  appUI.reset();
  appUI.setTitle("Display and Visuals");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "ディスプレイと外観");
  appUI.linkFunctionToBack(settings_init);
  appUI.addItem("brightness", settings_chooseBrightness, "Screen Brightness");
  appUI.addLocaleToItem("brightness", "ja", "画面の明るさ");
  appUI.addRightLocaleToItem("brightness", "en", String((screenBrightness+1)*10)+"%");
  appUI.addItem("style", settings_chooseStyle, "Dial Style");
  appUI.addLocaleToItem("style", "ja", "文字盤のスタイル");
  if (dialType == DIAL_CLASSIC) {
    appUI.addRightLocaleToItem("style", "en", "Classic");
    appUI.addRightLocaleToItem("style", "ja", "クラシック");
  } else {
    appUI.addRightLocaleToItem("style", "en", "ZeroDial");
  }
  appUI.makeUI(lang);
}

void settings_chooseBrightness() {
  appUI.reset();
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Screen Brightness");
  appUI.addLocaleToTitle("ja", "画面の明るさ");
  for (uint8_t i = 1; i <= 10; i++) {
    appUI.addItem(String(i-1), settings_setBrightness, String(i*10)+"%");
    appUI.addLocaleToItem(String(i-1), "ja", String(i*10)+"%");
  }
  appUI.linkFunctionToBack(settings_display);
  appUI.makeUI(lang);
}

void settings_setBrightness(String brightness) {
  settings_saved = false;
  screenBrightness = brightness.toInt();
  M5.Display.setBrightness(255*((screenBrightness+1)/10.0));
  settings_display();
}

void settings_chooseStyle() {
  appUI.reset();
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Dial Style");
  appUI.addLocaleToTitle("ja", "文字盤のスタイル");
  appUI.addItem("classic", settings_setStyle, "Classic");
  appUI.addLocaleToItem("classic", "ja", "クラシック");
  appUI.addItem("zerodial", settings_setStyle, "ZeroDial");
  appUI.linkFunctionToBack(settings_display);
  appUI.makeUI(lang);
}

void settings_setStyle(String style) {
  settings_saved = false;
  if (style == "classic") {
    dialType = DIAL_CLASSIC;
  } else {
    dialType = DIAL_ZERODIAL;
  }
  makeClockBase();
  settings_init();
}

void settings_notice() {
  appUI.reset();
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Notices");
  appUI.addLocaleToTitle("ja", "通知");
  appUI.addItem("volume", settings_chooseVolume, "Volume");
  appUI.addLocaleToItem("volume", "ja", "音量");
  appUI.addRightLocaleToItem("volume", "en", String((speakerVolume)*10)+"%");
  appUI.addItem("vibration", settings_switchVibration, "Vibration");
  appUI.addLocaleToItem("vibration", "ja", "振動");
  appUI.addRightLocaleToItem("vibration", "en", boolStr(modelType == 2, boolStr(vibration, "Enable", "Disable"), "N/A"));
  appUI.addRightLocaleToItem("vibration", "ja", boolStr(modelType == 2, boolStr(vibration, "有効", "無効"), "使用不可"));
  appUI.addItem("testNotice", settings_testNotice, "Test Notice");
  appUI.addLocaleToItem("testNotice", "ja", "通知のテスト");
  appUI.linkFunctionToBack(settings_init);
  appUI.makeUI(lang);
}

void settings_chooseVolume() {
  appUI.reset();
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Volume");
  appUI.addLocaleToTitle("ja", "音量");
  for (uint8_t i = 0; i <= 10; i++) {
    appUI.addItem(String(i), settings_setVolume, String(i*10)+"%");
    appUI.addLocaleToItem(String(i), "ja", String(i*10)+"%");
  }
  appUI.linkFunctionToBack(settings_notice);
  appUI.makeUI(lang);
}

void settings_setVolume(String volume) {
  settings_saved = false;
  speakerVolume = volume.toInt();
  settings_notice();
}

void settings_switchVibration() {
  if (modelType == 2) {
    vibration = !vibration;
    settings_saved = false;
    appUI.addRightLocaleToItem("vibration", "en", boolStr(vibration, "Enable", "Disable"));
    appUI.addRightLocaleToItem("vibration", "ja", boolStr(vibration, "有効", "無効"));
    appUI.makeUI(lang);
  }
}

void settings_testNotice() {
  notice("Test Notice", "");
}

void settings_info() {
  appUI.reset();
  String verStr = getVersionString(version);
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTitle("Infomation");
  appUI.addLocaleToTitle("ja", "情報");
  appUI.addItem("basever", nothing, "Version");
  appUI.addLocaleToItem("basever", "ja", "バージョン");
  appUI.addRightLocaleToItem("basever", "en", verStr);
  appUI.addItem("model", nothing, "Model");
  appUI.addLocaleToItem("model", "ja", "モデル");
  appUI.addRightLocaleToItem("model", "en", getModel(modelType));
  appUI.addItem("pmic", nothing, "Power Management IC");
  appUI.addLocaleToItem("pmic", "ja", "電源管理IC");
  if (M5.Power.getType() == m5::Power_Class::pmic_axp192) {
    appUI.addRightLocaleToItem("pmic", "en", "AXP192");
  } else if (M5.Power.getType() == m5::Power_Class::pmic_axp2101) {
    appUI.addRightLocaleToItem("pmic", "en", "AXP2101");
  } else {
    appUI.addRightLocaleToItem("pmic", "en", "Unknown");
    appUI.addRightLocaleToItem("pmic", "ja", "不明");
  }
  appUI.addItem("imu", nothing, "IMU");
  appUI.addLocaleToItem("imu", "ja", "IMU");
  if (M5.Imu.getType() == m5::imu_mpu6886) {
    appUI.addRightLocaleToItem("imu", "en", "MPU6886");
  } else if (M5.Imu.getType() == m5::imu_bmi270) {
    appUI.addRightLocaleToItem("imu", "en", "BMI270");
  } else if (M5.Imu.getType() == m5::imu_none) {
    appUI.addRightLocaleToItem("imu", "en", "None");
    appUI.addRightLocaleToItem("imu", "ja", "なし");
  } else {
    appUI.addRightLocaleToItem("imu", "en", "Unknown");
    appUI.addRightLocaleToItem("imu", "ja", "不明");
  }
  appUI.linkFunctionToBack(settings_init);
  appUI.makeUI(lang);
}

void settings_powerChoose() {
  uint32_t fixedChargeCurrent = chargeCurrent*100;
  float fixedChargeVoltage = (chargeVoltage*0.1)+4.0;
  appUI.reset();
  appUI.setTitle("Power Settings");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "電源設定");
  appUI.linkFunctionToBack(settings_init);
  appUI.addItem("charge", settings_power, "Settings when Charging");
  appUI.addLocaleToItem("charge", "ja", "充電時の設定");
  appUI.addItem("battery", settings_power, "Settings when Battery");
  appUI.addLocaleToItem("battery", "ja", "バッテリー時の設定");
  appUI.addItem("current", settings_chooseCurrent, "Max Charge Current");
  appUI.addLocaleToItem("current", "ja", "最大充電電流");
  appUI.addRightLocaleToItem("current", "en", String(fixedChargeCurrent)+"mA");
  appUI.addRightLocaleToItem("current", "ja", String(fixedChargeCurrent)+"mA");
  appUI.addItem("voltage", settings_chooseVoltage, "Max Charge Voltage");
  appUI.addLocaleToItem("voltage", "ja", "最大充電電圧");
  appUI.addRightLocaleToItem("voltage", "en", String(fixedChargeVoltage, 1)+"V");
  appUI.addRightLocaleToItem("voltage", "ja", String(fixedChargeVoltage, 1)+"V");
  appUI.addItem("showvoltage", settings_switchShowVoltage, "Show VBat");
  appUI.addLocaleToItem("showvoltage", "ja", "電池電圧の表示");
  appUI.addRightLocaleToItem("showvoltage", "en", boolStr(showVoltage, "Enabled", "Disabled"));
  appUI.addRightLocaleToItem("showvoltage", "ja", boolStr(showVoltage, "有効", "無効"));
  appUI.makeUI(lang);
}

void settings_switchShowVoltage() {
  if (modelType == 2) {
    showVoltage = !showVoltage;
    settings_saved = false;
    appUI.addRightLocaleToItem("showvoltage", "en", boolStr(showVoltage, "Enabled", "Disabled"));
    appUI.addRightLocaleToItem("showvoltage", "ja", boolStr(showVoltage, "有効", "無効"));
    appUI.makeUI(lang);
  }
}

void settings_chooseCurrent() {
  appUI.reset();
  appUI.setTitle("Max Charge Current");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "最大充電電流");
  appUI.linkFunctionToBack(settings_powerChoose);
  appUI.addItem("1", settings_setCurrent, "100mA");
  appUI.addItem("2", settings_setCurrent, "200mA");
  appUI.addItem("3", settings_setCurrent, "300mA");
  appUI.makeUI(lang);
}

void settings_setCurrent(String current) {
  settings_saved = false;
  if (current == "1") {
    chargeCurrent = 1;
  } else if (current == "2") {
    chargeCurrent = 2;
  } else if (current == "3") {
    chargeCurrent = 3;
  }
  M5.Power.setChargeCurrent(min(chargeCurrent*100, 300));
  settings_powerChoose();
}

void settings_chooseVoltage() {
  appUI.reset();
  appUI.setTitle("Max Charge Voltage");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "最大充電電圧");
  appUI.linkFunctionToBack(settings_powerChoose);
  appUI.addItem("0", settings_setVoltage, "4.0V");
  appUI.addItem("1", settings_setVoltage, "4.1V");
  appUI.addItem("2", settings_setVoltage, "4.2V");
  appUI.makeUI(lang);
}

void settings_setVoltage(String voltage) {
  settings_saved = false;
  if (voltage == "0") {
    chargeVoltage = 0;
  } else if (voltage == "1") {
    chargeVoltage = 1;
  } else if (voltage == "2") {
    chargeVoltage = 2;
  }
  M5.Power.setChargeVoltage(min((chargeVoltage*100)+4000, 4200));
  settings_powerChoose();
}

void settings_power(String chargeOrBattery) {
  if (chargeOrBattery == "charge") {
    settings_choseBat = false;
  } else if (chargeOrBattery == "battery") {
    settings_choseBat = true;
  } else {
    settings_init();
    return;
  }
  settings_powerBack();
}

void settings_powerBack() {
  appUI.reset();
  if (settings_choseBat) appUI.setTitle("Power (Bat)");
  else appUI.setTitle("Power (Chg)");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  if (settings_choseBat) appUI.addLocaleToTitle("ja", "電源設定（電池）");
  else appUI.addLocaleToTitle("ja", "電源設定（充電）");
  appUI.linkFunctionToBack(settings_powerChoose);
  appUI.addItem("sleep", settings_switchSleep, "Sleeping");
  appUI.addLocaleToItem("sleep", "ja", "スリープ");
  appUI.addRightLocaleToItem("sleep", "en", boolStr(doSleep[settings_choseBat], "Enable", "Disable"));
  appUI.addRightLocaleToItem("sleep", "ja", boolStr(doSleep[settings_choseBat], "有効", "無効"));
  if (doSleep[settings_choseBat]) appUI.addItem("touchsleep", settings_chooseSleep, "Touch sleep time");
  else appUI.addItem("touchsleep", nothing, "Touch sleep time");
  appUI.addLocaleToItem("touchsleep", "ja", "タッチスリープ時間");
  appUI.addRightLocaleToItem("touchsleep", "en", boolStr(doSleep[settings_choseBat], String(sleepTimings[settings_choseBat])+"s", "N/A"));
  appUI.addRightLocaleToItem("touchsleep", "ja", boolStr(doSleep[settings_choseBat], String(sleepTimings[settings_choseBat])+"秒", "使用不可"));
  if (doSleep[1] and settings_choseBat) appUI.addItem("gyrosleep", settings_chooseSleep, "Gyro sleep time");
  else appUI.addItem("gyrosleep", nothing, "Gyro sleep time");
  appUI.addLocaleToItem("gyrosleep", "ja", "ジャイロスリープ時間");
  appUI.addRightLocaleToItem("gyrosleep", "en", boolStr((doSleep[1] and settings_choseBat), String(sleepTimings[2+settings_choseBat])+"s", "N/A"));
  appUI.addRightLocaleToItem("gyrosleep", "ja", boolStr((doSleep[1] and settings_choseBat), String(sleepTimings[2+settings_choseBat])+"秒", "使用不可"));
  appUI.addItem("ext", settings_chooseExtPower, "Ext. Power supply");
  appUI.addLocaleToItem("ext", "ja", "外部への給電");
  if (extSettings[settings_choseBat] == EXT_POWER_ALWAYS) {
    appUI.addRightLocaleToItem("ext", "en", "Always");
    appUI.addRightLocaleToItem("ext", "ja", "常時");
  } else if (extSettings[settings_choseBat] == EXT_POWER_ACTIVE) {
    appUI.addRightLocaleToItem("ext", "en", "Only-A");
    appUI.addRightLocaleToItem("ext", "ja", "画面点灯時");
  } else if (extSettings[settings_choseBat] == EXT_POWER_NEVER) {
    appUI.addRightLocaleToItem("ext", "en", "Never");
    appUI.addRightLocaleToItem("ext", "ja", "しない");
  }
  appUI.makeUI(lang);
}

void settings_switchSleep() {
  doSleep[settings_choseBat] = !doSleep[settings_choseBat];
  appUI.addRightLocaleToItem("sleep", "en", boolStr(doSleep[settings_choseBat], "Enable", "Disable"));
  appUI.addRightLocaleToItem("sleep", "ja", boolStr(doSleep[settings_choseBat], "有効", "無効"));
  appUI.addRightLocaleToItem("touchsleep", "en", boolStr(doSleep[settings_choseBat], String(sleepTimings[settings_choseBat])+"s", "N/A"));
  appUI.addRightLocaleToItem("touchsleep", "ja", boolStr(doSleep[settings_choseBat], String(sleepTimings[settings_choseBat])+"秒", "使用不可"));
  appUI.addRightLocaleToItem("gyrosleep", "en", boolStr((doSleep[1] and settings_choseBat), String(sleepTimings[2+settings_choseBat])+"s", "N/A"));
  appUI.addRightLocaleToItem("gyrosleep", "ja", boolStr((doSleep[1] and settings_choseBat), String(sleepTimings[2+settings_choseBat])+"秒", "使用不可"));
  appUI.makeUI(lang);
}

void settings_chooseExtPower() {
  appUI.reset();
  if (settings_choseBat) appUI.setTitle("Ext.Power (Bat)");
  else appUI.setTitle("Ext.Power (Chg)");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  if (settings_choseBat) appUI.addLocaleToTitle("ja", "外部への給電（電池）");
  else appUI.addLocaleToTitle("ja", "外部への給電（充電）");
  appUI.linkFunctionToBack(settings_powerBack);
  appUI.addItem("A", settings_setExtPower, "Always");
  appUI.addLocaleToItem("A", "ja", "常時");
  appUI.addItem("O", settings_setExtPower, "Only Active");
  appUI.addLocaleToItem("O", "ja", "画面点灯時");
  appUI.addItem("N", settings_setExtPower, "Never");
  appUI.addLocaleToItem("N", "ja", "しない");
  appUI.makeUI(lang);
}

void settings_setExtPower(String extMode) {
  if (extMode == "A") extSettings[settings_choseBat] = EXT_POWER_ALWAYS;
  else if (extMode == "O") extSettings[settings_choseBat] = EXT_POWER_ACTIVE;
  else if (extMode == "N") extSettings[settings_choseBat] = EXT_POWER_NEVER;
  settings_saved = false;
  settings_powerBack();
}

void settings_chooseSleep(String touchOrGyro) {
  appUI.reset();
  char initial;
  if (touchOrGyro == "touchsleep") {
    settings_choseGyro = false;
    initial = 'T';
  } else if (touchOrGyro == "gyrosleep") {
    settings_choseGyro = true;
    initial = 'G';
  } else {
    settings_powerBack();
    return;
  }
  if (settings_choseBat) appUI.setTitle((String) initial+".Sleep Time (Bat)");
  else appUI.setTitle((String) initial+".Sleep Time (Chg)");
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  if (settings_choseBat) appUI.addLocaleToTitle("ja", (String) initial+"スリープ時間(電池)");
  else appUI.addLocaleToTitle("ja", (String) initial+"スリープ時間(充電)");
  appUI.linkFunctionToBack(settings_powerBack);
  appUI.addItem("5", settings_setSleep, "5s");
  appUI.addLocaleToItem("5", "ja", "5秒");
  appUI.addItem("10", settings_setSleep, "10s");
  appUI.addLocaleToItem("10", "ja", "10秒");
  appUI.addItem("15", settings_setSleep, "15s");
  appUI.addLocaleToItem("15", "ja", "15秒");
  appUI.addItem("30", settings_setSleep, "30s");
  appUI.addLocaleToItem("30", "ja", "30秒");
  appUI.addItem("60", settings_setSleep, "60s");
  appUI.addLocaleToItem("60", "ja", "60秒");
  appUI.makeUI(lang);
}

// 設定メニュー内で時刻同期を実行 / Sync time from settings menu (with visual feedback)
void settings_syncTime() {
  M5.Display.clear();
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.println("Time Sync...");
  long wifiTimer = millis();
  String SSID = connectWiFi(wifiJson);
  if (SSID != "") {
    M5.Display.println("WiFi: "+SSID);
    while (!(WiFi.status() == WL_CONNECTED or (millis()-wifiTimer) > 10000)) {
      delay(1000);
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.println("Wi-Fi Ready");
    syncTime();
    lastSync = dateTime.date.date+(dateTime.date.month<<5);
  } else {
    M5.Display.println("Wi-Fi Failed");
  }
  dateTime = M5.Rtc.getDateTime();
  WiFi.disconnect(true);
}

void settings_setSleep(String slpTime) {
  uint8_t slpTimeInt = slpTime.toInt();
  sleepTimings[(settings_choseGyro*2)+settings_choseBat] = slpTimeInt;
  sleepTimings_sys[(settings_choseGyro*2)+settings_choseBat] = 60000-(slpTimeInt*1000);
  settings_saved = false;
  settings_powerBack();
}

void settings_save() {
  if (!settings_saved) {
    settings_saved = true;
    Preferences pref;
    pref.begin("mk75_settings");
    pref.putString("lang", lang);
    uint32_t slpTime = ((uint32_t) sleepTimings[0] << 24) | ((uint32_t) sleepTimings[1] << 16) | ((uint32_t) sleepTimings[2] << 8) | (uint32_t) sleepTimings[3];
    pref.putUInt("slpTime", slpTime);
    uint8_t slpFlags = ((uint8_t) extSettings[0] << 4) | ((uint8_t) extSettings[1] << 2) | ((uint8_t) doSleep[0] << 1) | ((uint8_t) doSleep[1]);
    pref.putUChar("slpFlags", slpFlags);
    pref.putUChar("dialType", dialType);
    pref.putUChar("charge", combineHex(chargeCurrent, chargeVoltage));
    pref.putUChar("hmi", (vibration << 7)+(speakerVolume*10)+screenBrightness);
    pref.putBool("showVoltage", showVoltage);
    pref.end();
    appUI.addRightLocaleToItem("save", "en", "Saved");
    appUI.addRightLocaleToItem("save", "ja", "保存済み");
    appUI.setItemRightColor("save", TFT_GREEN);
    appUI.makeUI(lang);
  }
}

void settings_loop() {
  appUI.update(dateTime, battery, getBatteryVoltage());
}