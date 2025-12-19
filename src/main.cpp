/*
  [ MK75-Watch Ver.2β ] by K-Nana
  Smartwatch Firmware for M5Stack Core2.
  MIT License https://opensource.org/license/mit

  < WARNING: Beta version >
  This is a development version not intended for general use. 
  Unexpected issues may occur. 
  It is available for those who want to test new features early.
*/

#include <M5Unified.h>
#include <M5GFX.h>
#include <SD.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_sntp.h>
#include <map>
#include "assets/images.hpp"
#include "assets/sounds.hpp"
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "new"

#define NTP_TIMEZONE "JST-9" //今後設定で変更可能にする
#define NTP_SERVER1 "0.pool.ntp.org"
#define NTP_SERVER2 "1.pool.ntp.org"
#define NTP_SERVER3 "2.pool.ntp.org"

static M5Canvas cv_display(&M5.Display);
static M5Canvas cv_clock(&cv_display);
static M5Canvas cv_menu(&cv_display);
static M5Canvas cv_ckbase(&cv_display);
static M5Canvas cv_dtime_bat(&cv_display);
static M5Canvas cv_day(&cv_clock);
static M5Canvas cv_ckhhand(&cv_clock);
static M5Canvas cv_ckmhand(&cv_clock);

static M5Canvas cv_stwt1(&cv_display);
static M5Canvas cv_stwt2(&cv_display);
static M5Canvas cv_stwt3(&cv_display);
static M5Canvas cv_stwt4(&cv_display);
static M5Canvas cv_stwt5(&cv_display);
static M5Canvas cv_stwt_top(&cv_display);

static M5Canvas cv_timesel(&M5.Display);

UI *appUI;
JsonDocument wifiJson;
JsonDocument trainJson;
JsonDocument alarmJson;
JsonDocument spDatesJson;
std::list<long> timers;

const String lang = "ja";
String nowApp = "";

String UIAddtional = "";
uint8_t UItimeLeft;
uint8_t UItimeRight;

const int32_t sizeX = 320;
const int32_t centerX = sizeX/2;
const int32_t sizeY = 240;
const int32_t centerY = sizeY/2;
const int32_t ckCenterX = 110;
const int32_t ckCenterY = 110;
const int32_t JST = 32400;
const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
const String apps[6] = {"timer", "settings", "alarm", "commander", "stopwatch", "train"};
const String appsEn[6] = {"Timer", "Settings", "Alarm", "?", "Stopwatch", "Train"};
const String appsJa[6] = {"タイマー", "設定", "アラーム", "?", "ストップWt", "時刻表"};
String M5Model;

int32_t slpTimer = 10000;
int32_t vibTimer = 0;
int32_t screenSwipe = 0;
int32_t screenSwipeVertical = 0;
int32_t screenSwipeVerticalFirst = 0;
int32_t prevTX = 0;
int32_t prevTY = 0;
int32_t firstTX = 0;
int32_t firstTY = 0;
int32_t cycle = 0;
int32_t prevLoopTime = 0;
int32_t prevSwipeAcc[4] = {0, 0, 0, 0};
int32_t appStart = 0;
int32_t checkAlarmTimer = 0;
uint8_t lastAlarmMin = 60;
uint16_t lastSync = 0;
int32_t birthChangeTimer = 0;
uint8_t birthChangeID = 0;

float mpu[3] = {0, 0, 0};
float prevGyro[5] = {0, 0, 0, 0, 0};

bool wasVBUS = false;
bool wasTouched = false;
bool afterSlp = false;
bool haveToDeleteAppUI = false;
bool appMenu = false;
bool touchedOnMenu = false;

uint8_t battery = M5.Power.getBatteryLevel();
m5::rtc_datetime_t dateTime = M5.Rtc.getDateTime();

void nothing() {}

void thickLine(M5Canvas target, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
  target.drawLine(x0, y0, x1, y1, color);
  target.drawLine(x0+1, y0, x1+1, y1, color);
  target.drawLine(x0-1, y0, x1-1, y1, color);
  target.drawLine(x0, y0+1, x1, y1+1, color);
  target.drawLine(x0, y0-1, x1, y1-1, color);
}

String boolStr(bool target, String ifTrue, String ifFalse) {
  if (target) {
    return ifTrue;
  } else {
    return ifFalse;
  }
}

uint32_t batcolor(int32_t bat) {
  float r, g;
  if (bat <= 50) {
    r = 255;
    g = (bat*255)/50;
  } else {
    r = ((100-bat)*255)/50;
    g = 255;
  }
  return M5.Display.color888((u_int8_t) r, (u_int8_t) g, 0);
}

String getModel() {
  switch (M5.getBoard()) {
    case m5::board_t::board_M5StackCoreS3: return "M5StackS3";
    case m5::board_t::board_M5AtomS3Lite: return "M5ATOMS3Lite";
    case m5::board_t::board_M5AtomS3: return "M5ATOMS3";
    case m5::board_t::board_M5StampC3: return "M5StampC3";
    case m5::board_t::board_M5StampC3U: return "M5StampC3U";
    case m5::board_t::board_M5Stack: return "M5Stack";
    case m5::board_t::board_M5StackCore2: return "M5StackCore2";
    case m5::board_t::board_M5StickC: return "M5StickC";
    case m5::board_t::board_M5StickCPlus: return "M5StickCPlus";
    case m5::board_t::board_M5StackCoreInk: return "M5CoreInk";
    case m5::board_t::board_M5Paper: return "M5Paper";
    case m5::board_t::board_M5Tough: return "M5Tough";
    case m5::board_t::board_M5Station: return "M5Station";
    case m5::board_t::board_M5AtomMatrix: return "M5ATOM Matrix";
    case m5::board_t::board_M5AtomLite: return "M5ATOM Lite";
    case m5::board_t::board_M5AtomPsram: return "M5ATOM PSRAM";
    case m5::board_t::board_M5AtomU: return "M5ATOM U";
    case m5::board_t::board_M5TimerCam: return "TimerCamera";
    case m5::board_t::board_M5StampPico: return "M5StampPico";
    default: return "Unknown";
  }
}

void appEnd() {
  if (millis() > appStart+1000) {
    if (nowApp == "stopwatch") {
      cv_stwt1.deleteSprite();
      cv_stwt2.deleteSprite();
      cv_stwt3.deleteSprite();
      cv_stwt4.deleteSprite();
      cv_stwt5.deleteSprite();
      cv_stwt_top.deleteSprite();
    } else if (nowApp == "commander") {
      Wire.end();
    }
    nowApp = "";
    UIAddtional = "";
    if (haveToDeleteAppUI) {
      haveToDeleteAppUI = false;
      delete appUI;
    }
    M5.Speaker.end();
  }
}

// Powered by ChatGPT
bool isHoliday(int32_t month, int32_t day, int32_t year, int32_t weekday) {
  if (month == 1 && day == 1) return true;
  if (month == 1 && weekday == 1 && day >= 8 && day <= 14) return true;
  if (month == 2 && day == 11) return true;
  if (month == 3 && day == 20+(year%4 == 3)) return true;
  if (month == 4 && day == 29) return true;
  if (month == 5 && day == 3) return true;
  if (month == 5 && day == 4) return true;
  if (month == 5 && day == 5) return true;
  if (month == 7 && weekday == 1 && day >= 15 && day <= 21) return true;
  if (month == 8 && day == 11) return true;
  if (month == 9 && weekday == 1 && day >= 15 && day <= 21) return true;
  if (month == 9 && day == 23-(year%4 == 0)) return true;
  if (month == 10 && weekday == 1 && day >= 8 && day <= 14) return true;
  if (month == 11 && day == 3) return true;
  if (month == 11 && day == 23) return true;
  if (month == 2 && day == 23) return true;
  if (weekday == 0) {
    return (isHoliday(month, day - 1, year, 6));
  }
  return false;
}

int monthLastDay(int month, int year) {
  if (month == 2) {
    if (year%4 == 0 and (year%100 != 0 or year%400 == 0)) return 29;
    else return 28;
  } else if (month == 4 or month == 6 or month == 9 or month == 11) return 30;
  return 31;
}

String readSPIJson(String filename, JsonDocument *target, int32_t timeout) {
  JsonDocument temp;
  File file = SPIFFS.open(filename, FILE_READ);
  if (file) {
    DeserializationError error = deserializeJson(temp, file);
    file.close();
    if (error) {
      return "JSON Error";
    }
    *target = temp;
    return "";
  }
  file.close();
  return "Config file isn't found";
}

String writeSPIJson(String filename, JsonDocument *target) {
  JsonDocument temp = *target;
  if (SPIFFS.exists(filename)) {SPIFFS.remove(filename);}
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (file) {
    serializeJson(temp, file);
    file.close();
    return "";
  }
  file.close();
  return "Can't open file";
}

bool checkGyro() {
  /*
  M5.Imu.getAccel(&mpu[0], &mpu[1], &mpu[2]);
  return ((mpu[0] > 0.95) and (abs(mpu[1]) < 0.75) and (abs(mpu[2]) < 0.75));
  */
  //M5.Imu.getAccel(&mpu[0], &mpu[1], &mpu[2]);
  M5.Imu.getGyro(&mpu[0], &mpu[1], &mpu[2]);
  float sum = 0;
  prevGyro[0] = mpu[0];
  for (int8_t i = 4; i >= 0; i--) {
    sum += prevGyro[i];
    if (i != 5) {
      prevGyro[i+1] = prevGyro[i];
    }
  }
  return ((sum > 700));
}

bool checkAccel() {
  M5.Imu.getAccel(&mpu[0], &mpu[1], &mpu[2]);
  float sum = abs(mpu[0]) + abs(mpu[1]) + abs(mpu[2]);
  return ((sum < 1.2) and (sum > 0.8));
}

String connectWiFi(JsonDocument conf) {
  String bestSSID = "";
  int8_t bestRSSI = -128;
  if (!conf.isNull()) {
    int32_t n = WiFi.scanNetworks();
    for (int32_t i = 0; i < n; ++i) {
      //Serial.println(WiFi.SSID(i));
      if (!conf[WiFi.SSID(i)].isNull()) {
        if (bestRSSI < WiFi.RSSI(i)) {
          bestSSID = WiFi.SSID(i);
          bestRSSI = WiFi.RSSI(i);
        }
      }
    }
  }
  if (bestSSID != "") {
    //Serial.println("Try to connect "+bestSSID+" ...");
    String pass = conf[bestSSID];
    WiFi.begin(bestSSID, pass);
    return bestSSID;
  }
  return "";
}

bool connectWiFi() {
  WiFi.begin();
  int32_t timer = 0;
  while (WiFi.status() != WL_CONNECTED and timer < 100){
      delay(100);
      timer++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  return false;
}

String connectHTTP(String url) {
  WiFiClient client;
  HTTPClient http;
  String body;
  if (http.begin(client, url)) {
    //http.addHeader("Content-Type", "application/json");
    int32_t responseCode = http.GET();
    body = http.getString();
    http.end();
  } else {
    body = "";
  }
  return body;
}

void syncTime() {
  long tmrStart = millis();
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    delay(1000);
  }
  time_t t = time(nullptr)+1; // Advance one second.
  while (t > time(nullptr));  /// Synchronization in seconds
  M5.Rtc.setDateTime(localtime(&t));
  Serial.println("OK!");
}

void syncTimeOnSettings() {
  M5.Display.clear();
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.println("Time Sync...");
  long wifiTimer = millis();
  String SSID = connectWiFi(wifiJson);
  if (SSID != "") {
    M5.Display.println("WiFi: "+SSID);
    while (!(WiFi.status() == WL_CONNECTED and (millis()-wifiTimer) < 10000)) {
      delay(1000);
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.println("Wi-Fi Ready");
    syncTime();
  } else {
    M5.Display.println("Wi-Fi Failed");
  }
  dateTime = M5.Rtc.getDateTime();
  lastSync = dateTime.date.date+(dateTime.date.month<<5);
  WiFi.disconnect(true);
}

void notice(String title, String time) {
  M5.Display.wakeup();
  M5.Display.setBrightness(63);
  M5.Display.clear();
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawCenterString("Touch to Stop", sizeX/2, (sizeY/2)+50, &fonts::Font4);
  M5.Display.drawCenterString(time, sizeX/2, sizeY/2, &fonts::Font6);
  M5.Display.drawCenterString(title, sizeX/2, (sizeY/2)-50, &fonts::Font4);
  M5.Speaker.begin();
  M5.Speaker.setVolume(200);
  uint16_t soundTimer = 2000;
  while (true) {
    int32_t loopTimer = millis();
    if (soundTimer == 2000) {
      soundTimer = 0;
      M5.Speaker.playWav(alarm_sound);
      M5.Power.setVibration(127);
    } else if (soundTimer == 1000) {
      M5.Power.setVibration(0);
    }
    M5.update();
    if (M5.Touch.getCount() > 0) {
      M5.Speaker.end();
      return;
    }
    soundTimer += millis()-loopTimer;
  }
}

bool checkAlarm() {
  for( JsonObject loopAlarm : alarmJson.as<JsonArray>() ) {
    bool isWeekend = (dateTime.date.weekDay == 0 || dateTime.date.weekDay == 6);
    if ((isWeekend && loopAlarm["weekend"]) || (!isWeekend && loopAlarm["weekday"])) {
      int32_t hour = loopAlarm["hour"];
      int32_t min = loopAlarm["min"];
      if (dateTime.time.minutes != lastAlarmMin) {
        lastAlarmMin = 60;
      }
      if (dateTime.time.hours == hour and dateTime.time.minutes == min and lastAlarmMin != min) {
        lastAlarmMin = min;
        notice("Alarm", forceDigits(hour, 2)+":"+forceDigits(min, 2));
        return true;
      }
    }
  }
  return false;
}

bool checkTimer() {
  for(auto i = timers.begin(); i != timers.end(); i++) {
    if (*i <= millis()) {
      timers.erase(i);
      notice("Timer", "");
      return true;
    }
  }
  return false;
}

void scrollsWhenTouch(m5::touch_detail_t detail, int32_t* target, bool vertical = false) {
  int32_t now;
  int32_t prev;
  if (vertical) {
    now = detail.y;
    prev = prevTY;
  } else {
    now = detail.x;
    prev = prevTX;
  }
  if (prevTX != -1) {
    *target += prev-now;
    for (uint8_t i = 0; i < 3; i++) {
      prevSwipeAcc[i] = prevSwipeAcc[i+1];
    }
    prevSwipeAcc[3] = prev-now;
  }
  prevTX = detail.x;
  prevTY = detail.y;
  if (wasTouched) {
    firstTX = detail.x;
    firstTY = detail.y;
  }
}

void scrollsWhenNotTouch(int32_t* target, int32_t indexes, int32_t distant, bool useAcc = true) {
  if (*target < 0) {
    *target += (0-*target)/2;
    *target = round(*target);
    if (abs(*target) <= 1) *target = 0;
  } else if (*target > (indexes-1)*distant) {
    *target += (((indexes-1)*distant)-*target)/2;
    *target = round(*target);
    if (abs(*target-((indexes-1)*distant)) <= 1) *target = ((indexes-1)*distant);
  } else if (*target % distant != 0) {
    int8_t maxAcc = 0;
    for (uint8_t i = 0; i < 4; i++) {
      if (prevSwipeAcc[i] < 0 and prevSwipeAcc[i] < maxAcc) {
        maxAcc = prevSwipeAcc[i];
      } else if (prevSwipeAcc[i] > 0 and prevSwipeAcc[i] > maxAcc) {
        maxAcc = prevSwipeAcc[i];
      }
    }
    int32_t calib = limit(maxAcc*10, -(distant/2), distant/2);
    int32_t swipeTarget = limit(round(((float) (*target+calib)/distant))*distant, 0, (indexes-1)*distant);
    *target += (swipeTarget-*target)/2;
    if (abs(*target-swipeTarget) <= 1) *target = swipeTarget;
  }
  prevTX = -1;
  prevTY = -1;
}

void lowPowSleep() {
  uint8_t touch = 0;
  bool charged = M5.Power.Axp2101.isVBUS();
  M5.Display.clear();
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  M5.Imu.sleep();
  M5.Power.setExtOutput(false);
  while (!((touch > 0) or (charged != M5.Power.Axp2101.isVBUS()))) {
    dateTime = M5.Rtc.getDateTime();
    if (checkAlarm() || checkTimer()) break;
    if (lastSync != dateTime.date.date+(dateTime.date.month*32)) {
      M5.Power.setLed(255);
      long wifiTimer = millis();
      String SSID = connectWiFi(wifiJson);
      if (SSID != "") {
        while (!(WiFi.status() == WL_CONNECTED and (millis()-wifiTimer) < 10000)) {
          delay(1000);
        }
      }
      if (WiFi.status() == WL_CONNECTED) {
        syncTime();
      }
      dateTime = M5.Rtc.getDateTime();
      lastSync = dateTime.date.date+(dateTime.date.month<<5);
      WiFi.disconnect(true);
      M5.Power.setLed(0);
    }
    M5.Power.lightSleep(10000000);
    M5.update();
    touch = M5.Touch.getCount();
  }
  M5.Power.setExtOutput(true);
  M5.Imu.begin();
}

void callLowPowSleep() {
  appEnd();
  screenSwipe = 0;
  lowPowSleep();
  afterSlp = true;
  M5.Display.wakeup();
  M5.Display.setBrightness(63);
  slpTimer = 10000;
}

void activeSleep() {
  uint8_t touch = 0;
  uint8_t alarmTimer = 0;
  uint16_t lowPowTimer = 0;
  bool charged = M5.Power.Axp2101.isVBUS();
  M5.Display.clear();
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  M5.Power.setExtOutput(false);
  while (!((touch > 0) or (charged != M5.Power.Axp2101.isVBUS()))) {
    M5.Power.lightSleep(100000);
    M5.update();
    if ((!charged) and (checkGyro())) break;
    if (alarmTimer == 100) {
      alarmTimer = 0;
      dateTime = M5.Rtc.getDateTime();
      if (checkAlarm() || checkTimer()) break;
      if (checkAccel()) {
        lowPowTimer++;
      } else {
        lowPowTimer = 0;
      }
      if (lowPowTimer == 600) {
        lowPowSleep();
        break;
      }
    }
    alarmTimer++;
    touch = M5.Touch.getCount();
  }
  M5.Power.setExtOutput(true);
}

void updateDateTimeBat() {
  dateTime = M5.Rtc.getDateTime();
  battery = M5.Power.getBatteryLevel();
}

void drawTimeUI() {
  cv_timesel.setFont(&fonts::Font8);
  cv_timesel.drawCenterString(forceDigits(UItimeLeft, 2)+":"+forceDigits(UItimeRight, 2), 150, 43);
  cv_timesel.fillRoundRect(30, 0, 100, 40, 6, TFT_LIGHTGRAY);
  cv_timesel.fillRoundRect(170, 0, 100, 40, 6, TFT_LIGHTGRAY);
  cv_timesel.fillRoundRect(30, 125, 100, 40, 6, TFT_LIGHTGRAY);
  cv_timesel.fillRoundRect(170, 125, 100, 40, 6, TFT_LIGHTGRAY);
  cv_timesel.fillRect(35, 5, 90, 30, TFT_BLACK);
  cv_timesel.fillRect(175, 5, 90, 30, TFT_BLACK);
  cv_timesel.fillRect(35, 130, 90, 30, TFT_BLACK);
  cv_timesel.fillRect(175, 130, 90, 30, TFT_BLACK);
  cv_timesel.fillTriangle(80, 10, 40, 30, 120, 30, TFT_WHITE);
  cv_timesel.fillTriangle(220, 10, 180, 30, 260, 30, TFT_WHITE);
  cv_timesel.fillTriangle(80, 155, 40, 135, 120, 135, TFT_WHITE);
  cv_timesel.fillTriangle(220, 155, 180, 135, 260, 135, TFT_WHITE);
}

// Some Apps

void beep() {
  M5.Speaker.tone(440, 500);
}

// Settings
void powerOff() { M5.Power.powerOff(); }

void settings_init() {
  appStart = millis();
  M5.Speaker.begin();
  M5.Speaker.setVolume(127);
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setTitle("Settings");
  appUI->addItem((String) "lpsleep", callLowPowSleep, (String) "Low Power Sleep");
  appUI->addItem((String) "shutdown", powerOff, (String) "Shutdown");
  appUI->addItem((String) "synctime", syncTimeOnSettings, (String) "Sync Time");
  appUI->linkFunctionToBack(appEnd);
  appUI->makeUI(lang);
}

void settings_loop() {
  appUI->update(dateTime, battery);
}

// Alarm
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
void alarm_save();

void alarm_init() {
  appStart = millis();
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setTitle("Alarm");
  appUI->addItem("save", alarm_save, "Save");
  if (alarm_saved) {
    appUI->addRightLocaleToItem("save", "en", "Saved");
    appUI->setItemRightColor("save", M5.Display.color888(0, 255, 0));
  } else {
    appUI->addRightLocaleToItem("save", "en", "Not Saved");
    appUI->setItemRightColor("save", M5.Display.color888(255, 0, 0));
  }
  appUI->addItem("add", alarm_add, "+ Add Alarm");
  uint8_t cnt = 0;
  for( JsonObject loopAlarm : alarmJson.as<JsonArray>() ) {
    int32_t hour = loopAlarm["hour"];
    int32_t min = loopAlarm["min"];
    appUI->addItem((String) cnt, alarm_config, forceDigits(hour, 2)+":"+forceDigits(min, 2));
    cnt++;
  }
  alarm_count = cnt;
  appUI->linkFunctionToBack(appEnd);
  appUI->makeUI(lang);
}

void alarm_add() {
  JsonDocument newAlarm;
  alarm_saved = false;
  newAlarm["hour"] = 6;
  newAlarm["min"] = 0;
  newAlarm["weekday"] = true;
  newAlarm["weekend"] = true;
  alarmJson.add(newAlarm);
  alarm_config((String) alarm_count);
}

void alarm_remove() {
  alarm_saved = false;
  //Serial.println(alarm_toConfig);
  alarmJson.remove((size_t) alarm_toConfig);
  alarm_init();
}

void alarm_config(String name) {
  alarm_toConfig = name.toInt();
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setTitle("Edit Alarm");
  appUI->addItem("time", alarm_chooseTime, "Time");
  appUI->addRightLocaleToItem("time", "en", forceDigits(alarmJson[alarm_toConfig]["hour"], 2)+":"+forceDigits(alarmJson[alarm_toConfig]["min"], 2));
  appUI->addItem("weekday", alarm_switchWeekday, "Weekday");
  appUI->addRightLocaleToItem("weekday", "en", boolStr(alarmJson[alarm_toConfig]["weekday"], "Enable", "Disable"));
  appUI->addItem("weekend", alarm_switchWeekend, "Weekend");
  appUI->addRightLocaleToItem("weekend", "en", boolStr(alarmJson[alarm_toConfig]["weekend"], "Enable", "Disable"));
  appUI->addItem("remove", alarm_remove, "Remove");
  appUI->linkFunctionToBack(alarm_init);
  appUI->makeUI(lang);
}

void alarm_configNow() {
  alarm_config((String) alarm_toConfig);
}

void alarm_save() {
  alarm_saved = true;
  appUI->addRightLocaleToItem("save", "en", "Saved");
  appUI->setItemRightColor("save", M5.Display.color888(0, 255, 0));
  writeSPIJson("/alarm.json", &alarmJson);
  appUI->makeUI();
}

void alarm_setTime() {
  UIAddtional = "";
  alarm_saved = false;
  cv_timesel.deleteSprite();
  alarmJson[alarm_toConfig]["hour"] = UItimeLeft;
  alarmJson[alarm_toConfig]["min"] = UItimeRight;
  alarm_configNow();
}

void alarm_chooseTime() {
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setTitle("Time");
  appUI->setTransparentMode(true);
  appUI->linkFunctionToBack(alarm_setTime);
  UIAddtional = "time";
  cv_timesel.createSprite(300, 165);
  UItimeLeft = alarmJson[alarm_toConfig]["hour"];
  UItimeRight = alarmJson[alarm_toConfig]["min"];
  M5.Display.fillRect(0, 48, sizeX, sizeY-48, TFT_BLACK);
  drawTimeUI();
  appUI->makeUI(lang);
}

void alarm_switchWeekday() {
  alarm_saved = false;
  alarmJson[alarm_toConfig]["weekday"] = !alarmJson[alarm_toConfig]["weekday"];
  appUI->addRightLocaleToItem("weekday", "en", boolStr(alarmJson[alarm_toConfig]["weekday"], "Enable", "Disable"));
  appUI->makeUI(lang);
}

void alarm_switchWeekend() {
  alarm_saved = false;
  alarmJson[alarm_toConfig]["weekend"] = !alarmJson[alarm_toConfig]["weekend"];
  appUI->addRightLocaleToItem("weekend", "en", boolStr(alarmJson[alarm_toConfig]["weekend"], "Enable", "Disable"));
  appUI->makeUI(lang);
}

void alarm_loop() {
  appUI->update(dateTime, battery);
}

// Stopwatch
long stwts[5] = {0, 0, 0, 0, 0};
long stwts_stop[5] = {0, 0, 0, 0, 0};
int32_t stwt_swipe = 0;
uint8_t stwt_touchedID = 0;
uint16_t stwt_touchedTime = 0;
bool stwt_afterReset = false;
bool stwt_wasTouched = false;

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
  cv_stwt_top.createSprite(sizeX, 31);
  cv_stwt_top.fillRect(0, 0, sizeX, 31, TFT_WHITE);
  cv_stwt_top.setTextColor(TFT_BLACK, TFT_WHITE);
  cv_stwt_top.drawCenterString("Stopwatch", sizeX/2, 1, &fonts::efontJA_24);
  cv_stwt_top.pushSprite(0, 17);
  stwt_swipe = 0;
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
  } else {
    scrollsWhenNotTouch(&stwt_swipe, 5, sizeX);
    stwt_wasTouched = false;
    stwt_afterReset = false;
    stwt_touchedTime = 0;
  }
  if (inLimit(stwt_swipe, (sizeX*-1)+1, (sizeX*1)-1)) {
    stopwatch_redraw(cv_stwt1, 0);
    cv_stwt1.pushSprite((sizeX/2)-85-stwt_swipe, (sizeY/2)-60);
  }
  if (inLimit(stwt_swipe, (sizeX*0)+1, (sizeX*2)-1)) {
    stopwatch_redraw(cv_stwt2, 1);
    cv_stwt2.pushSprite((sizeX/2)+235-stwt_swipe, (sizeY/2)-60);
  }
  if (inLimit(stwt_swipe, (sizeX*1)+1, (sizeX*3)-1)) {
    stopwatch_redraw(cv_stwt3, 2);
    cv_stwt3.pushSprite((sizeX/2)+555-stwt_swipe, (sizeY/2)-60);
  }
  if (inLimit(stwt_swipe, (sizeX*2)+1, (sizeX*4)-1)) {
    stopwatch_redraw(cv_stwt4, 3);
    cv_stwt4.pushSprite((sizeX/2)+875-stwt_swipe, (sizeY/2)-60);
  }
  if (inLimit(stwt_swipe, (sizeX*3)+1, (sizeX*5)-1)) {
    stopwatch_redraw(cv_stwt5, 4);
    cv_stwt5.pushSprite((sizeX/2)+1195-stwt_swipe, (sizeY/2)-60);
  }
  cv_display.pushSprite(0, 0);
}

// Train
String timetableName = "";
bool train_isMainUI = false;
int32_t train_refleshTimer;
bool isWeekend;
bool isRemainingMode = false;

void train_config();
void train_switch_week();
void train_switch_mode();

void train_init() {
  appStart = millis();
  train_isMainUI = true;
  train_refleshTimer = 0;
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setLocaleFont("en", 0);
  appUI->setLocaleFont("ja", 1);
  appUI->setTitle("Train");
  appUI->addLocaleToTitle("ja", "時刻表");
  appUI->addItem("config", train_config, "Set Timetable");
  appUI->addLocaleToItem("config", "ja", "時刻表を設定");
  appUI->addItem("switch_week", train_switch_week);
  appUI->addItem("switch_mode", train_switch_mode);
  if (isRemainingMode) {
    appUI->addLocaleToItem("switch_mode", "en", "Rem. Minutes");
    appUI->addLocaleToItem("switch_mode", "ja", "残り時間（分）");
  } else {
    appUI->addLocaleToItem("switch_mode", "en", "Time");
    appUI->addLocaleToItem("switch_mode", "ja", "時刻");
  }
  if (dateTime.date.weekDay == 0 || dateTime.date.weekDay == 6 || isHoliday(dateTime.date.month, dateTime.date.date, dateTime.date.year, dateTime.date.weekDay)) {
    isWeekend = true;
    appUI->addLocaleToItem("switch_week", "en", "Weekend");
    appUI->addLocaleToItem("switch_week", "ja", "休日");
  } else {
    isWeekend = false;
    appUI->addLocaleToItem("switch_week", "en", "Weekday");
    appUI->addLocaleToItem("switch_week", "ja", "平日");
  }
  appUI->addItem("train1", nothing);
  appUI->addItem("train2", nothing);
  appUI->addItem("train3", nothing);
  appUI->linkFunctionToBack(appEnd);
  appUI->makeUI(lang);
}

void train_switch_week() {
  if (isWeekend) {
    isWeekend = false;
    appUI->addLocaleToItem("switch_week", "en", "Weekday");
    appUI->addLocaleToItem("switch_week", "ja", "平日");
  } else {
    isWeekend = true;
    appUI->addLocaleToItem("switch_week", "en", "Weekend");
    appUI->addLocaleToItem("switch_week", "ja", "休日");
  }
  appUI->makeUI(lang);
}

void train_switch_mode() {
  if (isRemainingMode) {
    isRemainingMode = false;
    appUI->addLocaleToItem("switch_mode", "en", "Time");
    appUI->addLocaleToItem("switch_mode", "ja", "時刻");
  } else {
    isRemainingMode = true;
    appUI->addLocaleToItem("switch_mode", "en", "Rem. Minutes");
    appUI->addLocaleToItem("switch_mode", "ja", "残り時間（分）");
  }
  appUI->makeUI(lang);
}

void train_setTimetable(String name) {
  timetableName = name;
  train_init();
  train_isMainUI = true;
}

void train_config() {
  train_isMainUI = false;
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setLocaleFont("en", 1);
  appUI->setLocaleFont("ja", 1);
  appUI->setTitle("Set Timetable");
  appUI->addLocaleToTitle("ja", "時刻表を設定");
  for ( JsonPair loopTimetableName : trainJson["timetable"].as<JsonObject>() ) {
    const char* nameBuffer = loopTimetableName.key().c_str();
    appUI->addItem(nameBuffer, train_setTimetable);
  }
  appUI->linkFunctionToBack(train_init);
  appUI->makeUI(lang);
}

void train_loop() {
  if (train_refleshTimer == 0 && train_isMainUI) {
    if (timetableName == "") {
      appUI->addLocaleToItem("train1", "en", "If a timetable is set, ");
      appUI->addLocaleToItem("train2", "en", "the next train or");
      appUI->addLocaleToItem("train3", "en", "bus appears here.");
      appUI->addLocaleToItem("train1", "ja", "時刻表を設定すると");
      appUI->addLocaleToItem("train2", "ja", "ここに次の電車やバスを");
      appUI->addLocaleToItem("train3", "ja", "表示できます。");
      appUI->setItemColor("train1", TFT_LIGHTGRAY);
      appUI->setItemColor("train2", TFT_LIGHTGRAY);
      appUI->setItemColor("train3", TFT_LIGHTGRAY);
      appUI->addRightLocaleToItem("train1", "en", "");
      appUI->addRightLocaleToItem("train2", "en", "");
      appUI->addRightLocaleToItem("train3", "en", "");
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
      for ( const auto loopTimetable : timetable[String(dateTime.time.hours)].as<JsonArray>() ) {
        if (loopTimetable["m"] > dateTime.time.minutes) {
          String dest_buffer = loopTimetable["d"];
          String type_buffer = loopTimetable["t"];
          if (loopTimetable["m"] < min[0]) {
            min[0] = loopTimetable["m"];
            dest[0] = dest_buffer;
            type[0] = type_buffer;
            hourAdd[0] = 0;
          } else if (loopTimetable["m"] < min[1]) {
            min[1] = loopTimetable["m"];
            dest[1] = dest_buffer;
            type[1] = type_buffer;
            hourAdd[1] = 0;
          } else if (loopTimetable["m"] < min[2]) {
            min[2] = loopTimetable["m"];
            dest[2] = dest_buffer;
            type[2] = type_buffer;
            hourAdd[2] = 0;
          }
        }
      }
      int32_t loopHourAdd = 0;
      while (min[2] == 60) {
        loopHourAdd = (loopHourAdd+1)%24;
        for ( const auto loopTimetable : timetable[String(dateTime.time.hours+loopHourAdd)].as<JsonArray>() ) {
          String dest_buffer = loopTimetable["d"];
          String type_buffer = loopTimetable["t"];
          if (loopTimetable["m"] < min[0] && (hourAdd[0] == -1 || hourAdd[0] == loopHourAdd)) {
            min[0] = loopTimetable["m"];
            dest[0] = dest_buffer;
            type[0] = type_buffer;
            hourAdd[0] = loopHourAdd;
          } else if (loopTimetable["m"] < min[1] && (hourAdd[1] == -1 || hourAdd[1] == loopHourAdd)) {
            min[1] = loopTimetable["m"];
            dest[1] = dest_buffer;
            type[1] = type_buffer;
            hourAdd[1] = loopHourAdd;
          } else if (loopTimetable["m"] < min[2] && (hourAdd[2] == -1 || hourAdd[2] == loopHourAdd)) {
            min[2] = loopTimetable["m"];
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
      appUI->setItemColor("train1", (colors[0][0]*65536)+(colors[0][1]*256)+(colors[0][2]));
      appUI->addLocaleToItem("train1", "en", type[0]+" "+dest[0]);
      appUI->setItemColor("train2", (colors[1][0]*65536)+(colors[1][1]*256)+(colors[1][2]));
      appUI->addLocaleToItem("train2", "en", type[1]+" "+dest[1]);
      appUI->setItemColor("train3", (colors[2][0]*65536)+(colors[2][1]*256)+(colors[2][2]));
      appUI->addLocaleToItem("train3", "en", type[2]+" "+dest[2]);
      if (isRemainingMode) {
        appUI->addRightLocaleToItem("train1", "en", String(min[0]-dateTime.time.minutes+(hourAdd[0]*60)));
        appUI->addRightLocaleToItem("train2", "en", String(min[1]-dateTime.time.minutes+(hourAdd[1]*60)));
        appUI->addRightLocaleToItem("train3", "en", String(min[2]-dateTime.time.minutes+(hourAdd[2]*60)));
      } else {
        appUI->addRightLocaleToItem("train1", "en", String(dateTime.time.hours+hourAdd[0])+":"+forceDigits(min[0], 2));
        appUI->addRightLocaleToItem("train2", "en", String(dateTime.time.hours+hourAdd[1])+":"+forceDigits(min[1], 2));
        appUI->addRightLocaleToItem("train3", "en", String(dateTime.time.hours+hourAdd[2])+":"+forceDigits(min[2], 2));
      }
    }
    appUI->makeUI(lang);
  }
  train_refleshTimer++;
  if (train_refleshTimer >= 10) {
    train_refleshTimer = 0;
  }
  appUI->update(dateTime, battery);
}

// Timer
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
  appStart = millis();
  timer_editing = 0;
  train_isMainUI = true;
  train_refleshTimer = 0;
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setTitle("Timer");
  appUI->addItem("add", timer_make, "+ Add Timer");
  uint8_t cnt = 1;
  for(auto i = timers.begin(); i != timers.end(); i++ ) {
    uint32_t remains = (*i - millis())/1000;
    appUI->addItem(String(cnt), timer_edit, "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
    cnt++;
  }
  appUI->linkFunctionToBack(appEnd);
  appUI->makeUI();
}

void timer_make() {
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  appUI->setTitle("Set Timer");
  appUI->setTransparentMode(true);
  UIAddtional = "time";
  cv_timesel.createSprite(300, 165);
  UItimeLeft = alarmJson[alarm_toConfig]["hour"];
  UItimeRight = alarmJson[alarm_toConfig]["min"];
  M5.Display.fillRect(0, 48, sizeX, sizeY-48, TFT_BLACK);
  drawTimeUI();
  appUI->linkFunctionToBack(timer_start);
  appUI->makeUI();
}

void timer_start() {
  UIAddtional = "";
  cv_timesel.deleteSprite();
  uint16_t timerTime = ((UItimeLeft*60) + (UItimeRight));
  timers.push_back((timerTime*1000)+millis());
  timer_init();
}

void timer_edit(String id) {
  if (haveToDeleteAppUI) {
    delete appUI;
  }
  appUI = new UI;
  haveToDeleteAppUI = true;
  timer_editing = id.toInt();
  timer_editingMillis = *std::next(timers.begin(), timer_editing-1);
  uint32_t remains = (timer_editingMillis - millis())/1000;
  appUI->setTitle("Edit Timer");
  appUI->addItem("timer", nothing, String((uint8_t) floor(remains/60))+":"+String(remains%60));
  appUI->addItem("remove", timer_remove, "Remove Timer");
  appUI->linkFunctionToBack(timer_init);
  appUI->makeUI();
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
      appUI->addLocaleToItem(String(cnt), "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
      cnt++;
    }
    appUI->makeUI();
  } else if (timer_editing != 0) {
    uint32_t remains = (timer_editingMillis - millis())/1000;
    appUI->addLocaleToItem("timer", "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
    appUI->makeUI();
  }
  appUI->update(dateTime, battery);
}

void makeClockBase() {
  cv_ckbase.createSprite(220, 220);
  cv_ckbase.fillCircle(ckCenterX, ckCenterY, 110, TFT_WHITE);
  cv_ckbase.fillCircle(ckCenterX, ckCenterY, 105, TFT_BLACK);
  float deg;
  for (int8_t i = 0; i < 12; i++) {
    deg = i*0.523;
    if (i == 6) {
      cv_ckbase.setCursor((sin(deg)*93)+ckCenterX-14, (cos(deg)*93)+ckCenterY-13);
      cv_ckbase.setFont(&fonts::Font4);
      cv_ckbase.setTextColor(TFT_WHITE, TFT_BLACK);
      cv_ckbase.setTextSize(1);
      cv_ckbase.print("12");
    } else if (i % 3 == 0) {
      cv_ckbase.setCursor((sin(deg)*93)+ckCenterX-7, (cos(deg)*93)+ckCenterY-13);
      cv_ckbase.setFont(&fonts::Font4);
      cv_ckbase.setTextColor(TFT_WHITE, TFT_BLACK);
      cv_ckbase.setTextSize(1);
      cv_ckbase.print(12-((i+18)%12));
    } else {
      thickLine(cv_ckbase, (sin(deg)*97)+ckCenterX, (cos(deg)*97)+ckCenterY, (sin(deg)*106)+ckCenterX, (cos(deg)*106)+ckCenterY, TFT_WHITE);
    }
  }
  cv_ckhhand.createSprite(7, 70);
  cv_ckmhand.createSprite(5, 105);
  cv_ckhhand.fillScreen(TFT_DARKCYAN);
  cv_ckmhand.fillScreen(TFT_SKYBLUE);
}

void updateClock() {
  cv_ckbase.pushSprite(&cv_clock, 0, 0);
  if (M5.Power.Axp2101.isVBUS()) {
    cv_clock.fillArc(ckCenterX, ckCenterY, 110, 108, 270, (battery*3.6)-90.5, CYAN);
  } else {
    cv_clock.fillArc(ckCenterX, ckCenterY, 110, 108, 270, (battery*3.6)-90.5, batcolor(battery));
  }
  cv_ckhhand.setPivot(2, 2);
  //アンチエイジングを入れても入れなくてもあまり（1フレーム当たり、80~90ms中5ms程度しか）負荷が変わらないことを確認しました
  //cv_ckhhand.pushRotated(&cv_clock, (180+((dateTime.time.hours*30)+(dateTime.time.minutes/2)))%360);
  cv_ckhhand.pushRotatedWithAA(&cv_clock, (180+((dateTime.time.hours*30)+(dateTime.time.minutes/2)))%360);
  cv_ckmhand.setPivot(1, 1);
  //cv_ckmhand.pushRotated(&cv_clock, (180+((dateTime.time.minutes*6)+(dateTime.time.seconds/10)))%360);
  cv_ckmhand.pushRotatedWithAA(&cv_clock, (180+((dateTime.time.minutes*6)+(dateTime.time.seconds/10)))%360);
  cv_clock.fillCircle(ckCenterX, ckCenterY, 8, TFT_SKYBLUE);
  cv_clock.fillCircle(ckCenterX, ckCenterY, 5, TFT_BLACK);
}

void updateDigitals() {
  cv_dtime_bat.clear();
  cv_dtime_bat.setTextColor(TFT_WHITE, TFT_BLACK);
  cv_dtime_bat.drawString(forceDigits(dateTime.time.hours, 2)+":"+forceDigits(dateTime.time.minutes, 2)+" "+forceDigits(dateTime.time.seconds, 2), 0, 0, &fonts::Font2);
  if (M5.Power.Axp2101.isVBUS()) { cv_dtime_bat.setTextColor(CYAN, TFT_BLACK); }
  cv_dtime_bat.drawRightString(String(battery)+"%", sizeX, 0, &fonts::Font2);
  cv_day.clear();
  uint8_t dateY;
  if (!spDatesJson[String(dateTime.date.month)].isNull()) {
    if (!spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)].isNull()) {
      dateY = 0;
      if (birthChangeTimer+5000 < millis()) {
        birthChangeTimer = millis();
        birthChangeID++;
        if (birthChangeID >= spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)].size()) {
          birthChangeID = 0;
        }
      }
      int32_t dayColor = spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["color"];
      cv_day.setTextColor(M5.Display.color24to16(dayColor), TFT_BLACK);
      String dayName = spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["name"];
      cv_day.drawString(dayName, 0, 17, &fonts::Font2);
    } else {
      dateY = 17;
    }
  } else {
    dateY = 17;
  }
  cv_day.setTextColor(TFT_WHITE, TFT_BLACK);
  cv_day.drawString(String(dateTime.date.month)+"/"+String(dateTime.date.date)+" "+week[dateTime.date.weekDay]+" "+dateTime.date.year, 0, dateY, &fonts::Font2);
  int32_t ramFree = (int32_t) (heap_caps_get_free_size(MALLOC_CAP_8BIT));
  int32_t ramTotal = (int32_t) (heap_caps_get_total_size(MALLOC_CAP_8BIT));
  cv_day.drawRightString((String) ((ramTotal-ramFree)/1000)+"KB/"+(String) (ramTotal/1000)+"KB "+prevLoopTime+"ms/f", sizeX, 17, &fonts::Font2);
}

bool copySDtoSPI() {
  if (SD.begin(4, SPI)) {
    File folder = SD.open("/spiffs");
    if (!folder) {
      SD.end();
      return false;
    } else if (!folder.isDirectory()) {
      SD.end();
      return false;
    }
    File loopFile = folder.openNextFile();
    while (loopFile) {
      String fileName = loopFile.name();
      String SDPath = "/spiffs/"+fileName;
      String SPIPath = "/"+fileName;
      if (SPIFFS.exists(fileName)) {SPIFFS.remove(fileName);}
      File SDCardFile = SD.open(SDPath, FILE_READ);
      File SPIFile = SPIFFS.open(SPIPath, FILE_WRITE);
      int32_t size = SDCardFile.size();
      int32_t progress = 0;
      while (SDCardFile.available()) {
        //if (progress % 1000 == 0) Serial.println(SPIPath + " " + (((float) progress/(float) size)*100) + "% " + progress + "/" + size);
        uint8_t chr1 = SDCardFile.peek();
        uint8_t chr2 = SDCardFile.read();
        if (chr1 == chr2) {
          progress++;
          SPIFile.write(chr1);
        } else {
          SDCardFile.seek(SDCardFile.position()-1);
        }
      }
      SDCardFile.close();
      SPIFile.close();
      loopFile = folder.openNextFile();
    }
    SD.end();
    return true;
  } else {
    return false;
  }
}

void setupConfigs() {
  SPIFFS.begin(true);
  M5Model = getModel();
  M5.Display.println("MK75-Watch");
  M5.Display.print("Model: ");
  if (M5Model == "M5StackCore2") {
    M5.Display.setTextColor(GREEN, TFT_BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Supported Model");
  } else {
    M5.Display.setTextColor(RED, TFT_BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Unsupported Model");
  }
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.print("copy sd to spiffs...");
  if (copySDtoSPI()) {
    M5.Display.println("success");
  } else {
    M5.Display.println("failed");
  }
  M5.Display.print("Loading wifi json...");
  String SPIResult = readSPIJson("/wifi.json", &wifiJson, 1000);
  M5.Display.println("finish");
  /*
  long wifiTimer = 0;
  if (SPIResult == "") {
    if (WiFi.status() != WL_CONNECTED) {
      M5.Display.print("wifi from json connect...");
      String SSID = connectWiFi(wifiJson);
      if (SSID != "") {
        M5.Display.println(SSID+" success");
        wifiTimer = millis();
      } else {
        M5.Display.println("failed");
      }
    }
  } else {
    M5.Display.println("error ["+SPIResult+"]");
  }
  */
  M5.Display.print("Loading train json...");
  readSPIJson("/train.json", &trainJson, 100000);
  M5.Display.println("finish");
  M5.Display.print("Loading alarm json...");
  readSPIJson("/alarm.json", &alarmJson, 100000);
  M5.Display.println("finish");
  M5.Display.print("Loading special dates json...");
  readSPIJson("/special_dates.json", &spDatesJson, 100000);
  M5.Display.println("finish");
  /*
  if (wifiTimer != 0) {
    while (!(WiFi.status() == WL_CONNECTED and (millis()-wifiTimer) < 10000)) {
      delay(1000);
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.print("change time...");
    String error = syncTime();
    if (error == "") {
      M5.Display.println("success");
    } else {
      M5.Display.println("failed ["+error+"]");
    }
  } else {
    M5.Display.println("can't set time");
  }
  WiFi.disconnect(true);
  */
  dateTime = M5.Rtc.getDateTime();
  lastSync = dateTime.date.date+(dateTime.date.month<<5);
  delay(1000);
  M5.Display.clear();
}

void setupSprites() {
  cv_display.createSprite(sizeX, sizeY);
  cv_clock.createSprite(220, 220);
  cv_menu.createSprite(100, 240);
  makeClockBase();
  cv_dtime_bat.createSprite(sizeX, 17);
  cv_day.createSprite(sizeX, 34);
}

void drawMenu() {
  cv_menu.clear();
  cv_menu.setFont(&lgfxJapanMincho_40);
  for (int8_t i = 0; i < 6; i++) {
    if (inLimit(i*120, screenSwipeVertical-220, screenSwipeVertical+220)) {
      // パフォーマンス重視ならfillRoundRectではなくfillRectを使うべきかもしれない
      cv_menu.fillRoundRect(105-screenSwipe, (i*120)+75-screenSwipeVertical, 90, 90, 5, WHITE);
      cv_menu.fillRoundRect(110-screenSwipe, (i*120)+80-screenSwipeVertical, 80, 80, 4, BLACK);
      cv_menu.pushImage(134-screenSwipe, (i*120)+104-screenSwipeVertical, 32, 32, icons[i]);
      if (lang == "ja") {
        cv_menu.drawCenterString(appsJa[i], 150-screenSwipe, (i*120)+140-screenSwipeVertical, &fonts::efontJA_16);
      } else if (lang == "en") {
        cv_menu.drawCenterString(appsEn[i], 150-screenSwipe, (i*120)+140-screenSwipeVertical, &fonts::Font2);
      } else {
        //cv_menu.drawCenterString("ERROR. LOL", (ix*105)+(sizeX/2), ((iy*85)-45)+(sizeY/2), &fonts::Font2);
      }
    }
  }
}

void loopSleep() {
  bool touch = M5.Touch.getCount() > 0;
  if ((touch) or (vibTimer > 0)) {
    slpTimer = 0;
  } else if (checkGyro() and (!M5.Power.Axp2101.isVBUS()) and (slpTimer > 10000)) {
    slpTimer = 10000;
  } else {
    if (nowApp == "commander") {
      slpTimer += (prevLoopTime / 2);
    } else {
      slpTimer += prevLoopTime;
    }
    if (slpTimer >= 15000) {
      bool charged = M5.Power.Axp2101.isVBUS();
      if (charged) {
        lowPowSleep();
      } else {
        activeSleep();
      }
      if (M5.Power.Axp2101.isVBUS() and !charged) {
        vibTimer = 500;
      }
      afterSlp = true;
      M5.Display.wakeup();
      M5.Display.setBrightness(63);
      slpTimer = 10000;
    }
  }
}

void loopMenuTouch() {
  if (!afterSlp) {
    if (M5.Touch.getCount() > 0) {
      m5::Touch_Class::touch_detail_t tDetail = M5.Touch.getDetail();
      if (tDetail.wasPressed()) {
        touchedOnMenu = appMenu;
        for (int8_t i = 0; i < 4; i++) {
          prevSwipeAcc[i] = 0;
        }
        //screenSwipeVerticalFirst = screenSwipeVertical;
        if (tDetail.x < 220 or !appMenu) {
          appMenu = !appMenu;
        }
      }
      if (tDetail.base_x >= 220) {
        //screenSwipeVertical = screenSwipeVerticalFirst - tDetail.distanceY();
        scrollsWhenTouch(tDetail, &screenSwipeVertical, true);
        if (tDetail.wasClicked() and !tDetail.isDragging() and !tDetail.isFlicking() and touchedOnMenu) {
          for (int8_t i = 0; i < 6; i++) {
            if (inLimit(tDetail.x, 225, 315) and inLimit(tDetail.y, (i*120)+75-screenSwipeVertical, (i*120)+165-screenSwipeVertical)) {
              if (i == 0) {
                nowApp = "timer";
                timer_init();
              } else if (i == 1) {
                nowApp = "settings";
                settings_init();
              } else if (i == 2) {
                nowApp = "alarm";
                alarm_init();
              } else if (i == 3) {
                //nowApp = "timer";
                //timer_init();
              } else if (i == 4) {
                nowApp = "stopwatch";
                stopwatch_init();
              } else if (i == 5) {
                nowApp = "train";
                train_init();
              }
            }
          } 
        }
      }
    } else {
      scrollsWhenNotTouch(&screenSwipeVertical, 6, 120, false);
    }
  }
}

void loopTimeSel() {
  cv_timesel.pushSprite(centerX-150, centerY-60);
  if (M5.Touch.getCount() > 0) {
    m5::Touch_Class::touch_detail_t tDetail = M5.Touch.getDetail();
    if (tDetail.wasPressed() || tDetail.isHolding()) {
      if (inLimit(tDetail.x, 40, 140) && inLimit(tDetail.y, 50, 110)) {
        UItimeLeft++;
        if (UItimeLeft == 24) UItimeLeft = 0;
        drawTimeUI();
      } else if (inLimit(tDetail.x, 180, 280) && inLimit(tDetail.y, 50, 110)) {
        UItimeRight++;
        if (UItimeRight == 60) UItimeRight = 0;
        drawTimeUI();
      } else if (inLimit(tDetail.x, 40, 140) && inLimit(tDetail.y, 175, 235)) {
        UItimeLeft--;
        if (UItimeLeft == 255) UItimeLeft = 23;
        drawTimeUI();
      } else if (inLimit(tDetail.x, 180, 280) && inLimit(tDetail.y, 175, 235)) {
        UItimeRight--;
        if (UItimeRight == 255) UItimeRight = 59;
        drawTimeUI();
      }
    }
  }
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);
  M5.Power.begin();
  M5.Imu.begin();
  Serial.begin(115200);
  M5.Display.init();
  M5.Display.setBrightness(63);
  setupConfigs();
  setupSprites();
  //setupMenu();
  wasVBUS = M5.Power.Axp2101.isVBUS();
}

void loop() {
  int32_t tmrStart = millis();
  M5.update();
  if (nowApp != "commander") cv_display.clear();
  bool touch = (M5.Touch.getCount() > 0);
  loopSleep();
  if (vibTimer > 0) {
    M5.Power.setVibration(127);
    vibTimer -= prevLoopTime;
  } else {
    M5.Power.setVibration(0);
  }
  if ((!wasVBUS) and M5.Power.Axp2101.isVBUS()) {
    vibTimer = 200;
  }
  wasVBUS = M5.Power.Axp2101.isVBUS();
  updateDateTimeBat();
  if (M5.BtnB.wasPressed()) {
    screenSwipe = 0;
    if (nowApp != "") appEnd();
  }
  if (M5.BtnC.wasPressed()) {}
  // 時間入力
  if (UIAddtional == "time") {
    loopTimeSel();
  }
  if (nowApp == "settings") {
    settings_loop();
  } else if (nowApp == "train") {
    train_loop();
  } else if (nowApp == "alarm") {
    alarm_loop();
  } else if (nowApp == "stopwatch") {
    stopwatch_loop();
  } else if (nowApp == "timer") {
    timer_loop();
  } else {
    // アプリサイドバーの開閉
    loopMenuTouch();
    // アプリサイドバーの調整
    if (appMenu) {
        // 目標値100
        screenSwipe = ceil((screenSwipe-100)/2)+100;
    } else {
        // 目標値0
        screenSwipe = floor(screenSwipe/2);
    }
    updateClock();
    cv_clock.pushSprite(&cv_display, centerX-ckCenterX-round(screenSwipe/2), centerY-ckCenterY, TFT_BLACK);
    updateDigitals();
    cv_day.pushSprite(&cv_display, 1, sizeY-34, TFT_BLACK);
    cv_dtime_bat.pushSprite(&cv_display, 0, 0, TFT_BLACK);
    if (screenSwipe != 0) {
      drawMenu();
      cv_menu.pushSprite(&cv_display, 220, 0, TFT_BLACK);
    }
    cv_display.pushSprite(0, 0);
  }
  if (checkAlarmTimer > 10000) {
    checkAlarmTimer = 0;
    checkAlarm();
    checkTimer();
  } else {
    checkAlarmTimer += prevLoopTime;
  }
  if (afterSlp) {
    afterSlp = false;
  } else {
    prevLoopTime = millis()-tmrStart;
  }
}
