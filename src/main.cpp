/*
  [ MK75-Watch ] by K-Nana
  Smartwatch Firmware for M5Stack Core2 / CoreS3.
  MIT License https://opensource.org/license/mit
*/

#include <M5Unified.h>
#include <M5GFX.h>
#include <SD.h>
#include <LittleFS.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_sntp.h>
#include <Preferences.h>
#include <map>
#include "assets/images.hpp"
#include "assets/sounds.hpp"
#include "assets/htmls.hpp"
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "libs/SerialFileEdit.hpp"
#include "new"

// ======================================
// == Application & UI Type Definitions ==
// ======================================

// アプリケーションタイプ / Application type enumeration
typedef enum {
  APP_NOTHING = 0,
  APP_ALARM = 1,
  APP_TIMER = 2,
  APP_STOPWATCH = 3,
  APP_TRAIN = 4,
  APP_EXT_DEVICE = 5,
  APP_RANDOM = 6,
  APP_SETTINGS = 7
} apptype_t;

// UI追加機能タイプ / Additional UI type
typedef enum {
  UI_NOTHING = 0,
  UI_TIME = 1,
  UI_RANDOM = 2
} uiaddtional_t;

// 時刻UI表示モード / Time display mode for UI
typedef enum {
  TIMEUI_MODE_HOURMIN = 0,
  TIMEUI_MODE_MINSEC = 1,
  TIMEUI_MODE_DATE = 2
} uiaddsettings_t;

// 外部デバイス電源制御設定 / External device power control settings
typedef enum {
  EXT_POWER_ALWAYS = 0,
  EXT_POWER_ACTIVE = 1,
  EXT_POWER_NEVER = 2
} extsettings_t;

// 時計盤デザインタイプ / Dial type for clock display
typedef enum {
  DIAL_ZERODIAL = 0,
  DIAL_CLASSIC = 1
} dialtype_t;

#define NTP_TIMEZONE "JST-9" // 日本標準時 / Japan Standard Time (UTC+9) in seconds
#define NTP_SERVER1 "ntp.nict.jp"
#define NTP_SERVER2 "ntp.jst.mfeed.ad.jp"
#define NTP_SERVER3 "time.google.com"

// =====================================
// ==  Canvas Buffers for Rendering  ==
// =====================================
// 描画用Canvas：レイヤー構造で各要素を個別に描画し、最後に合体する
// Rendering layers: Main display, clock, menu, and component buffers

static M5Canvas cv_display(&M5.Display);
static M5Canvas cv_clock(&cv_display);
static M5Canvas cv_menu(&cv_display);
static M5Canvas cv_ckbase(&cv_display);
static M5Canvas cv_ckbase_digit(&cv_ckbase);
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

static M5Canvas cv_uiadditional(&M5.Display); // 時刻選択UI用Canvas / Canvas for time selection UI

// =====================================
// ==     Global UI & Data Objects    ==
// =====================================
// UI管理、JSON設定ファイル、接続設定などのグローバル変数
// Global UI manager, JSON documents, WiFi server, and data structures

UI appUI;
JsonDocument wifiJson;
JsonDocument trainJson;
JsonDocument alarmJson;
JsonDocument spDatesJson;
std::list<long> timers;
WiFiServer server(80);
String header;
SemaphoreHandle_t xMutex; // カギの変数
TaskHandle_t drawTaskHandle = NULL;

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
SFE_USB SFE(&Serial, &LittleFS);
#else
SFE_HWS SFE(&Serial, &LittleFS);
#endif

apptype_t nowApp = APP_NOTHING; // 現在実行中のアプリケーション / Currently active application

// UI状態管理変数 / UI state management variables
uiaddtional_t UIAddtional = UI_NOTHING;
uiaddsettings_t UIAddtionalSettings = TIMEUI_MODE_HOURMIN;
int8_t UItimeLeft;
int8_t UItimeRight;
dialtype_t dialType = DIAL_ZERODIAL;

// =====================================
// ==     Screen Layout Constants    ==
// =====================================
// 画面サイズと中心座標、時計中心などのレイアウト定数
// Canvas size, center coordinates, and layout parameters

const int32_t sizeX = 320;
const int32_t centerX = sizeX/2;
const int32_t sizeY = 240;
const int32_t centerY = sizeY/2;
const int32_t ckCenterX = 110;
const int32_t ckCenterY = 110;
const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
const String apps[7] = {"timer", "alarm", "stopwatch", "train", "random", "external", "settings"};
const String appsEn[7] = {"Timer", "Alarm", "Stopwatch", "TrainTime", "Random", "Ext.Device", "Settings"};
const String appsJa[7] = {"タイマー", "アラーム", "ストップWt", "交通時刻表", "ランダム", "外部デバイス", "設定"};
const uint8_t howManyApps = 7;
const uint32_t version = 2607150; // [version]

const uint8_t timeSyncHour = 4;
const IPAddress ip(192, 168, 10, 75);
const IPAddress subnet(255, 255, 255, 0);
const char* github_root_ca = "-----BEGIN CERTIFICATE-----\n"
"MIIFBjCCAu6gAwIBAgIRAMISMktwqbSRcdxA9+KFJjwwDQYJKoZIhvcNAQELBQAw\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw\n"
"WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n"
"RW5jcnlwdDEMMAoGA1UEAxMDUjEyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
"CgKCAQEA2pgodK2+lP474B7i5Ut1qywSf+2nAzJ+Npfs6DGPpRONC5kuHs0BUT1M\n"
"5ShuCVUxqqUiXXL0LQfCTUA83wEjuXg39RplMjTmhnGdBO+ECFu9AhqZ66YBAJpz\n"
"kG2Pogeg0JfT2kVhgTU9FPnEwF9q3AuWGrCf4yrqvSrWmMebcas7dA8827JgvlpL\n"
"Thjp2ypzXIlhZZ7+7Tymy05v5J75AEaz/xlNKmOzjmbGGIVwx1Blbzt05UiDDwhY\n"
"XS0jnV6j/ujbAKHS9OMZTfLuevYnnuXNnC2i8n+cF63vEzc50bTILEHWhsDp7CH4\n"
"WRt/uTp8n1wBnWIEwii9Cq08yhDsGwIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB\n"
"hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB\n"
"/wIBADAdBgNVHQ4EFgQUALUp8i2ObzHom0yteD763OkM0dIwHwYDVR0jBBgwFoAU\n"
"ebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAC\n"
"hhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcG\n"
"A1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcN\n"
"AQELBQADggIBAI910AnPanZIZTKS3rVEyIV29BWEjAK/duuz8eL5boSoVpHhkkv3\n"
"4eoAeEiPdZLj5EZ7G2ArIK+gzhTlRQ1q4FKGpPPaFBSpqV/xbUb5UlAXQOnkHn3m\n"
"FVj+qYv87/WeY+Bm4sN3Ox8BhyaU7UAQ3LeZ7N1X01xxQe4wIAAE3JVLUCiHmZL+\n"
"qoCUtgYIFPgcg350QMUIWgxPXNGEncT921ne7nluI02V8pLUmClqXOsCwULw+PVO\n"
"ZCB7qOMxxMBoCUeL2Ll4oMpOSr5pJCpLN3tRA2s6P1KLs9TSrVhOk+7LX28NMUlI\n"
"usQ/nxLJID0RhAeFtPjyOCOscQBA53+NRjSCak7P4A5jX7ppmkcJECL+S0i3kXVU\n"
"y5Me5BbrU8973jZNv/ax6+ZK6TM8jWmimL6of6OrX7ZU6E2WqazzsFrLG3o2kySb\n"
"zlhSgJ81Cl4tv3SbYiYXnJExKQvzf83DYotox3f0fwv7xln1A2ZLplCb0O+l/AK0\n"
"YE0DS2FPxSAHi0iwMfW2nNHJrXcY3LLHD77gRgje4Eveubi2xxa+Nmk/hmhLdIET\n"
"iVDFanoCrMVIpQ59XWHkzdFmoHXHBV7oibVjGSO7ULSQ7MJ1Nz51phuDJSgAIU7A\n"
"0zrLnOrAj/dfrlEWRhCvAgbuwLZX1A2sjNjXoPOHbsPiy+lO1KF8/XY7\n"
"-----END CERTIFICATE-----\n";

// =====================================
// ==      Device Information         ==
// =====================================
String M5Model;

// =====================================
// ==  State & Timer Management Vars ==
// =====================================
// スリープタイマー、タッチ検出、画面スワイプなどの状態管理
// Sleep, vibration, and screen gesture tracking variables

int32_t slpTimer = 0;
uint32_t vibTimer = 0;
int32_t screenSwipe = 0;
int32_t screenSwipeVertical = 0;
int32_t screenSwipeVerticalFirst = 0;
int32_t prevTX = 0;
int32_t prevTY = 0;
int32_t cycle = 0;
int32_t prevLoopTime = 0;
int32_t prevSwipeAcc[4] = {0, 0, 0, 0};
int32_t appStart = 0;
int32_t checkAlarmTimer = 0;
uint8_t lastAlarmMin = 60;
uint16_t lastSync = 0;
uint32_t birthChangeTimer = 0;
uint8_t birthChangeID = 0;
uint8_t timeSyncMinute = 60;
uint8_t modelType = 0; // 2 for Core2, 10 for CoreS3
uint32_t latestVer = 0;
uint8_t backLight = 64;
uint16_t shutdownTimer = 0;
uint32_t mainLoopTimer = 0;
uint8_t chargeCurrent = 1;
uint8_t chargeVoltage = 0;
uint8_t screenBrightness = 1;
uint8_t speakerVolume = 1;
uint8_t chosenAtScroll = 0;
uint8_t nextChosenAtScroll = 0;

char lang[3];
uint8_t sleepTimings[4] = {};
uint32_t sleepTimings_sys[4] = {};
bool doSleep[2] = {};
extsettings_t extSettings[2] = {};

float gyro[3] = {0, 0, 0};
float accel[3] = {0, 0, 0};
float prevAccel[3] = {0, 0, 0};
float prevGyro[5] = {0, 0, 0, 0, 0};
float lastchange = 0;

bool wasVBUS = false;
bool afterSlp = false;
bool haveToDeleteAppUI = false;
bool appMenu = false;
bool touchedOnMenu = false;
bool autoShutdown = true; // 設定可能WIP
bool doDraw = true;
bool axp2101 = false;
bool vibration = false;
bool showVoltage = false;
bool lowpower = false;
bool appInit = false;
bool touchInterrupted = false;

uint8_t battery = M5.Power.getBatteryLevel();
m5::rtc_datetime_t dateTime; // RTC日時情報 / Real-time clock date/time

// ======================================
// ===== Utility Functions (汎用関数) =====
// ======================================

// ISR callback - signal to skip drawing in current loop cycle
IRAM_ATTR void touchInterrupt() {
  if (nowApp == APP_NOTHING) {
    doDraw = false;
    touchInterrupted = true;
    chosenAtScroll = nextChosenAtScroll;
  }
}

void compatibleAttachInterrupt() {
  if (modelType == 10) {
    attachInterrupt(GPIO_NUM_21, touchInterrupt, FALLING);
  } else {
    attachInterrupt(GPIO_NUM_39, touchInterrupt, FALLING);
  }
}

void compatibleDetachInterrupt() {
  if (modelType == 10) {
    detachInterrupt(GPIO_NUM_21);
  } else {
    detachInterrupt(GPIO_NUM_39);
  }
}


// 太い線を描画 / Draw a thicker line by overlaying multiple shifted lines
void thickLine(M5Canvas target, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
  target.drawLine(x0, y0, x1, y1, color);
  target.drawLine(x0+1, y0, x1+1, y1, color);
  target.drawLine(x0-1, y0, x1-1, y1, color);
  target.drawLine(x0, y0+1, x1, y1+1, color);
  target.drawLine(x0, y0-1, x1, y1-1, color);
}

// 論理値を指定文字列に変換 / Convert boolean to specified string
String boolStr(bool target, String ifTrue, String ifFalse) {
  if (target) {
    return ifTrue;
  } else {
    return ifFalse;
  }
}

// バッテリー残量に応じた色を計算 / Calculate color based on battery level (green->yellow->red)
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

// M5Stack デバイスモデルを英字名で取得 / Detect and return M5Stack device model name
String getModel() {
  modelType = M5.getBoard();
  switch (modelType) {
    case m5::board_t::board_M5StackCoreS3: return "CoreS3";
    case m5::board_t::board_M5StackCoreS3SE: return "CoreS3-SE";
    case m5::board_t::board_M5AtomS3Lite: return "ATOMS3 Lite";
    case m5::board_t::board_M5AtomS3: return "ATOMS3";
    case m5::board_t::board_M5StampC3: return "StampC3";
    case m5::board_t::board_M5StampC3U: return "StampC3U";
    case m5::board_t::board_M5Stack: return "Basic";
    case m5::board_t::board_M5StackCore2: return "Core2";
    case m5::board_t::board_M5StickC: return "StickC";
    case m5::board_t::board_M5StickCPlus: return "StickCPlus";
    case m5::board_t::board_M5StackCoreInk: return "CoreInk";
    case m5::board_t::board_M5Paper: return "Paper";
    case m5::board_t::board_M5Tough: return "Tough";
    case m5::board_t::board_M5Station: return "Station";
    case m5::board_t::board_M5AtomMatrix: return "ATOM Matrix";
    case m5::board_t::board_M5AtomLite: return "ATOM Lite";
    case m5::board_t::board_M5AtomPsram: return "ATOM PSRAM";
    case m5::board_t::board_M5AtomU: return "ATOM U";
    case m5::board_t::board_M5TimerCam: return "TimerCamera";
    case m5::board_t::board_M5StampPico: return "StampPico";
    default: return "Unknown";
  }
}

// アプリケーション実行終了処理 / Cleanup and teardown running application
void appEnd() {
  if (nowApp == APP_STOPWATCH) {
    cv_stwt1.deleteSprite();
    cv_stwt2.deleteSprite();
    cv_stwt3.deleteSprite();
    cv_stwt4.deleteSprite();
    cv_stwt5.deleteSprite();
    cv_stwt_top.deleteSprite();
  }
  nowApp = APP_NOTHING;
  UIAddtional = UI_NOTHING;
  appUI.reset();
  M5.Speaker.end();
  touchInterrupted = false;
}

// 日本の祝日判定 / Check if a given date is a Japanese holiday (powered by ChatGPT)
// 3連休ルール対応：繰り替え休日と昭和の日のフローティング等を実装
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

// うるう年判定 / Determine if a year is a leap year (Gregorian calendar rules)
bool isLeapYear(uint8_t year) {
  if (year % 400 == 0) return true;
  else if (year % 100 == 0) return false;
  else if (year % 4 == 0) return true;
  return false;
}

// 指定月の最大日数を取得（うるう年対応） / Get maximum day in month (leap year aware)
uint8_t getMonthMaxDay(uint8_t month, bool leapYear) {
  switch (month) {
    case 1: return 31;
    case 2: {
      if (leapYear) return 29;
      return 28;
    }
    case 3: return 31;
    case 4: return 30;
    case 5: return 31;
    case 6: return 30;
    case 7: return 31;
    case 8: return 31;
    case 9: return 30;
    case 10: return 31;
    case 11: return 30;
    case 12: return 31;
    default: return 0;
  }
}

// 日付から曜日を計算（ツェラーの公式） / Calculate weekday from date (Zeller's formula from Wikipedia)
uint8_t dateToWeekday(uint16_t y, uint8_t m, uint8_t d) {
  if (m < 3) {
    y--;
    m += 12;
  }
  return (y + y/4 - y/100 + y/400 + (13*m + 8)/5 + d) % 7;
}

// LittleFS から JSON ファイルを読み込み / Read JSON from LittleFS file
// Return: true=成功, false=失敗 / true=success, false=failure
bool readSPIJson(String filename, JsonDocument *target, int32_t timeout) {
  JsonDocument temp;
  File file = LittleFS.open(filename, FILE_READ);
  if (file) {
    DeserializationError error = deserializeJson(temp, file);
    file.close();
    if (error) {
      return false;
    }
    *target = temp;
    return true;
  }
  file.close();
  return false;
}

// JSON ファイルを LittleFS に書き込み / Write JSON to LittleFS file
bool writeSPIJson(String filename, JsonDocument *target) {
  JsonDocument temp = *target;
  if (LittleFS.exists(filename)) {LittleFS.remove(filename);}
  File file = LittleFS.open(filename, FILE_WRITE);
  if (file) {
    serializeJson(temp, file);
    file.close();
    return true;
  }
  file.close();
  return false;
}

// ジャイロスコープの動きを検出（デバイス揺動判定）/ Detect gyro activity level
bool checkGyro() {
  // 加速度ベースの判定（現在は未使用）/ Acceleration-based check (currently unused)
  /*
  M5.Imu.getAccel(&accel[0], &accel[1], &accel[2]);
  return ((accel[0] > 0.95) and (abs(accel[1]) < 0.75) and (abs(accel[2]) < 0.75));
  */
  // ジャイロスコープの履歴から動きの合計を計算 / Calculate gyro sum over history
  M5.Imu.getGyro(&gyro[0], &gyro[1], &gyro[2]);
  float sum = 0;
  prevGyro[0] = gyro[0];
  for (int8_t i = 4; i >= 0; i--) {
    sum += prevGyro[i];
    if (i != 4) {
      prevGyro[i+1] = prevGyro[i];
    }
  }
  return ((sum > 700));
}

// 加速度センサーの急激な変化を検出 / Detect significant acceleration changes
bool checkAccel() {
  M5.Imu.getAccel(&accel[0], &accel[1], &accel[2]);
  float change = 0;
  for (uint8_t i = 0; i < 3; i++) {
    change += abs(accel[i]-prevAccel[i]);
    prevAccel[i] = accel[i];
  }
  lastchange = change;
  return (change < 0.05);
}

bool isVbus() {
  return M5.Power.Axp2101.isVBUS();
  // return M5.Power.getVBUSVoltage() > 4900; // 4.9V以上をUSB接続とみなす（充電中も含む）
}

void compatibleLightSleep(uint32_t duration, bool noInterrupt) {
  if (noInterrupt) {
    //noInterrupts();
    compatibleDetachInterrupt();
  }
  if (modelType == 10) {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, false);
    esp_sleep_enable_timer_wakeup(duration);
    esp_light_sleep_start();
  } else M5.Power.lightSleep(duration);
  if (noInterrupt) {
    //interrupts();
    compatibleAttachInterrupt();
  }
}

// 指定日付から翌日を計算（月末日や年末変更を処理） / Calculate next calendar day
m5::rtc_datetime_t getNextDay(m5::rtc_datetime_t date) {
  m5::rtc_datetime_t result;
  result = date;
  if (date.date.date == getMonthMaxDay(date.date.month, isLeapYear(date.date.year))) {
    result.date.date = 1;
    if (date.date.month == 12) {
      result.date.month = 1;
      result.date.year += 1;
    } else {
      result.date.month += 1;
    }
  } else {
    result.date.date += 1;
  }
  result.date.weekDay = (result.date.weekDay+1)%7;
  return result;
}

// RTCアラームを設定（次のアラーム発動時刻を計算し、日本時を念慮して設定） / Configure RTC alarm for next scheduled alarm
// アラーム知慮JSONから次の発動時間を探索、翌日を計算して設定 / Complex logic: searches through all alarms, finds next matching weekday/time
void setRTCAlarmIRQ() {
  uint32_t nowTime = (dateTime.time.hours*3600)+(dateTime.time.minutes*60)+dateTime.time.seconds;
  uint32_t nextAlarmTime = UINT_MAX;
  uint8_t nextAlarmHour = 0;
  uint8_t nextAlarmMinutes = 0;
  uint8_t nextAlarmDays = 0;
  for( JsonObject loopAlarm : alarmJson.as<JsonArray>() ) {
    if (loopAlarm["weekday"] || loopAlarm["weekend"]) {
      uint32_t hour = loopAlarm["hour"];
      uint32_t min = loopAlarm["min"];
      uint32_t loopAlarmTime = (hour*3600)+(min*60);
      bool match = false;
      uint8_t daysAdd = 0;
      for (uint8_t i = 0; !match; i++) {
        uint8_t iWeekday = (dateTime.date.weekDay+i)%7;
        bool isWeekend = (iWeekday == 0 || iWeekday == 6);
        if (((isWeekend && loopAlarm["weekend"]) || (!isWeekend && loopAlarm["weekday"])) && ((i > 0) || (loopAlarmTime > nowTime))) {
          match = true;
          daysAdd = i;
        }
      }
      uint32_t time = (86400*daysAdd)+loopAlarmTime-nowTime;
      if (nextAlarmTime > time) {
        nextAlarmTime = time;
        nextAlarmHour = loopAlarm["hour"];
        nextAlarmMinutes = loopAlarm["min"];
        nextAlarmDays = daysAdd;
      }
    }
  }
  if (nextAlarmTime != UINT_MAX) {
    m5::rtc_datetime_t nextAlarm = dateTime;
    nextAlarm.time.hours = nextAlarmHour;
    nextAlarm.time.minutes = nextAlarmMinutes;
    nextAlarm.time.seconds = 0;
    for (uint8_t i; i < nextAlarmDays; i++) nextAlarm = getNextDay(nextAlarm);
    // Serial.print(nextAlarm.date.year);
    // Serial.print("/");
    // Serial.print(nextAlarm.date.month);
    // Serial.print("/");
    // Serial.print(nextAlarm.date.date);
    // Serial.print("/");
    // Serial.print(week[nextAlarm.date.weekDay]);
    // Serial.print(" ");
    // Serial.print(nextAlarm.time.hours);
    // Serial.print(":");
    // Serial.println(nextAlarm.time.seconds);
    M5.Rtc.setAlarmIRQ(nextAlarm.date, nextAlarm.time);
  } else {
    M5.Rtc.disableIRQ();
  }
}

// WiFi 接続（設定済みSSID一覧から最強電波を選択）/ Connect WiFi to strongest configured SSID
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

// WiFi 接続（デフォルト資格情報）/ Connect WiFi with default credentials
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

// HTTP GET リクエストを実行してレスポンスボディを返す / Perform HTTP GET request and return response
String connectHTTP(String url, String contentType = "text/plain") {
  WiFiClientSecure client;
  client.setCACert(github_root_ca); 
  HTTPClient https;
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  String body;
  if (https.begin(client, url)) {
    https.addHeader("Content-Type", contentType);
    int32_t responseCode = https.GET();
    body = https.getString();
    https.end();
  } else {
    body = "";
  }
  return body;
}

// 外部出力ポートの制御（常時ON/アクティブ時ON/常時OFF）/ Control external output port based on settings
void updateExtOutput(bool active, bool charge) {
  M5.Power.setExtOutput(extSettings[charge] == EXT_POWER_ALWAYS || (extSettings[charge] == EXT_POWER_ACTIVE && active));
}

// NTP サーバーと同期して RTC 時刻を更新 / Sync RTC with NTP server
void syncTime() {
  String latestVerStr = connectHTTP("https://k-7iro.github.io/mk75watch/version.txt");
  latestVer = latestVerStr.toInt();
  if (latestVer > version) {
    Preferences pref;
    pref.begin("mk75_settings");
    pref.putUInt("latestVer", latestVer);
    pref.end();
  }
  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  // Serial.println("Synctime 1");
  while (!(sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)) {
    delay(1000);
  }
  // Serial.println("Synctime 2");
  time_t t = time(nullptr)+1; // Advance one second.
  while (t > time(nullptr));  // Synchronization in seconds
  // Serial.println("Synctime 3");
  M5.Rtc.setDateTime(localtime(&t));
  setRTCAlarmIRQ();
  // Serial.println("OK!");
}

// 設定メニュー内で時刻同期を実行 / Sync time from settings menu (with visual feedback)
void syncTimeOnSettings() {
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

// アラーム/スピード速報を試播と操作で試播を実行 / Display notice with sound and vibration
void notice(String title, String time) {
  M5.Display.wakeup();
  M5.Display.setBrightness(255*((screenBrightness+1)/10.0));
  M5.Display.clear();
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.drawCenterString("Touch to Stop", sizeX/2, (sizeY/2)+50, &fonts::Font4);
  M5.Display.drawCenterString(time, sizeX/2, sizeY/2, &fonts::Font6);
  M5.Display.drawCenterString(title, sizeX/2, (sizeY/2)-50, &fonts::Font4);
  M5.Speaker.begin();
  M5.Speaker.setVolume(255*((speakerVolume)/10.0));
  uint16_t soundTimer = 2000;
  while (true) {
    int32_t loopTimer = millis();
    if (soundTimer == 2000) {
      soundTimer = 0;
      if (speakerVolume != 0) M5.Speaker.playWav(alarm_sound);
      if (vibration) M5.Power.setVibration(127);
    } else if (soundTimer == 1000) {
      M5.Power.setVibration(0);
    }
    M5.update();
    if (M5.Touch.getDetail().wasClicked()) {
      M5.update();
      M5.Speaker.end();
      return;
    }
    soundTimer += millis()-loopTimer;
  }
}

// 現在時刻で発動対象のアラームを確認して発動を実行 / Check if any alarm matches current time and trigger if needed
bool checkAlarm() {
  if (dateTime.time.minutes != lastAlarmMin) {
    lastAlarmMin = 60;
  }
  bool isWeekend = (dateTime.date.weekDay == 0 || dateTime.date.weekDay == 6);
  for( JsonObject loopAlarm : alarmJson.as<JsonArray>() ) {
    if ((isWeekend && loopAlarm["weekend"]) || (!isWeekend && loopAlarm["weekday"])) {
      int32_t hour = loopAlarm["hour"];
      int32_t min = loopAlarm["min"];
      if (dateTime.time.hours == hour and dateTime.time.minutes == min and lastAlarmMin != min) {
        lastAlarmMin = min;
        notice("Alarm", forceDigits(hour, 2)+":"+forceDigits(min, 2));
        M5.Rtc.clearIRQ();
        setRTCAlarmIRQ();
        return true;
      }
    }
  }
  return false;
}

// タイマー一覧から消却時を査訪し、発動を確認して発動を実行 / Check and trigger expired timers
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

// Touch scroll processing - calculates position delta from touch movement
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
}

// Inertial scroll animation with boundary snapping - decelerates scrolling
void scrollsWhenNotTouch(int32_t* target, int32_t indexes, int32_t distant, uint8_t* nowChosen, uint8_t* nextChosen, bool useAcc = true, float speed = 2, uint8_t maxAccMulti = 10) {
  if (*target < 0) {
    *target += (0-*target)/speed;
    *target = round(*target);
    if (abs(*target) <= 1) *target = 0;
  } else if (*target > (indexes-1)*distant) {
    *target += (((indexes-1)*distant)-*target)/speed;
    *target = round(*target);
    if (abs(*target-((indexes-1)*distant)) <= 1) *target = ((indexes-1)*distant);
  } else if (*target % distant != 0) {
    int8_t maxAcc = 0;
    for (uint8_t i = 0; i < 3; i++) {
      if (prevSwipeAcc[i] < 0 and prevSwipeAcc[i] < maxAcc) {
        maxAcc = prevSwipeAcc[i];
      } else if (prevSwipeAcc[i] > 0 and prevSwipeAcc[i] > maxAcc) {
        maxAcc = prevSwipeAcc[i];
      }
    }
    int32_t calib = limit(maxAcc*maxAccMulti, -(distant/2), distant/2);
    *nextChosen = limit(round(((float) (*target+calib)/distant)), 0, indexes-1);
    int32_t swipeTarget = limit(round(((float) (*target+calib)/distant))*distant, max(0, ((*nowChosen-1)*distant)), min((indexes-1)*distant, ((*nowChosen+1)*distant)));
    *target += (swipeTarget-*target)/speed;
    if (abs(*target-swipeTarget) <= 1) {
      *target = swipeTarget;
      *nowChosen = *nextChosen;
    }
  }
  prevTX = -1;
  prevTY = -1;
}

// Connect WiFi and sync time from NTP server
void connectWiFiAndTimeSync() {
  M5.Power.setLed(255);
  long wifiTimer = millis();
  String SSID = connectWiFi(wifiJson);
  if (SSID != "") {
    while (!(WiFi.status() == WL_CONNECTED or (millis()-wifiTimer) > 10000)) {
      delay(100);
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
  }
  dateTime = M5.Rtc.getDateTime();
  WiFi.disconnect(true);
  M5.Power.setLed(0);
}

// Deep sleep mode - maintains minimal power while monitoring for wake events
void lowPowSleep() {
  uint8_t touch = 0;
  bool charged = isVbus();
  M5.Display.clear();
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  M5.Imu.sleep();
  setCpuFrequencyMhz(80);
  lowpower = true;
  updateExtOutput(false, charged);
  while (!((touch > 0) or (charged != isVbus()))) {
    dateTime = M5.Rtc.getDateTime();
    if (checkAlarm() || checkTimer()) break;
    if (dateTime.time.hours == timeSyncHour and lastSync != dateTime.date.date+(dateTime.date.month*32)) {
      if (timeSyncMinute == 60) timeSyncMinute = random(60);
      if (dateTime.time.minutes == timeSyncMinute) {
        timeSyncMinute = 60;
        connectWiFiAndTimeSync();
        lastSync = dateTime.date.date+(dateTime.date.month<<5);
      }
    }
    compatibleLightSleep(10000000, true);
    M5.update();
    touch = M5.Touch.getCount();
  }
  updateExtOutput(true, isVbus());
  M5.Imu.begin();
}

void activeSleep() {
  uint8_t touch = 0;
  uint8_t alarmTimer = 0;
  uint16_t lowPowTimer = 0;
  bool charged = isVbus();
  M5.Display.clear();
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  setCpuFrequencyMhz(80);
  lowpower = true;
  updateExtOutput(false, charged);
  while (!((touch > 0) or (charged != isVbus()))) {
    compatibleLightSleep(100000, true);
    M5.update();
    if (M5.BtnPWR.getState() == m5::Button_Class::state_clicked) break;
    if ((!charged) and (checkGyro())) break;
    if (alarmTimer % 10 == 0) {
      if (checkAccel()) {
        lowPowTimer += 1;
      } else {
        lowPowTimer = 0;
      }
      if (lowPowTimer == 600) {
        if (autoShutdown && timers.size() == 0) {
          connectWiFiAndTimeSync();
          M5.Power.powerOff();
        } else lowPowSleep();
        break;
      }
    }
    if (alarmTimer == 100) {
      alarmTimer = 0;
      dateTime = M5.Rtc.getDateTime();
      if (checkAlarm() || checkTimer()) break;
    }
    alarmTimer++;
    touch = M5.Touch.getCount();
  }
  updateExtOutput(true, isVbus());
}

// Update current date, time, and battery level from RTC and power manager
void updateDateTimeBat() {
  dateTime = M5.Rtc.getDateTime();
  battery = M5.Power.getBatteryLevel();
}

// Draw time/date selection UI with up/down arrow buttons
void drawTimeUI() {
  cv_uiadditional.setFont(&fonts::Font8);
  if (UIAddtionalSettings == TIMEUI_MODE_DATE) {
    cv_uiadditional.drawCenterString(forceDigits(UItimeLeft, 2)+"/"+forceDigits(UItimeRight, 2), 150, 43);
  } else {
    cv_uiadditional.drawCenterString(forceDigits(UItimeLeft, 2)+":"+forceDigits(UItimeRight, 2), 150, 43);
  }
  cv_uiadditional.fillRoundRect(30, 0, 100, 40, 6, TFT_LIGHTGRAY);
  cv_uiadditional.fillRoundRect(170, 0, 100, 40, 6, TFT_LIGHTGRAY);
  cv_uiadditional.fillRoundRect(30, 125, 100, 40, 6, TFT_LIGHTGRAY);
  cv_uiadditional.fillRoundRect(170, 125, 100, 40, 6, TFT_LIGHTGRAY);
  cv_uiadditional.fillRect(35, 5, 90, 30, TFT_BLACK);
  cv_uiadditional.fillRect(175, 5, 90, 30, TFT_BLACK);
  cv_uiadditional.fillRect(35, 130, 90, 30, TFT_BLACK);
  cv_uiadditional.fillRect(175, 130, 90, 30, TFT_BLACK);
  cv_uiadditional.fillTriangle(80, 10, 40, 30, 120, 30, TFT_WHITE);
  cv_uiadditional.fillTriangle(220, 10, 180, 30, 260, 30, TFT_WHITE);
  cv_uiadditional.fillTriangle(80, 155, 40, 135, 120, 135, TFT_WHITE);
  cv_uiadditional.fillTriangle(220, 155, 180, 135, 260, 135, TFT_WHITE);
}

// Prepare clock face graphics (dial, hour hand, minute hand) with selected style
void makeClockBase() {
  uint16_t col0; //Dial color
  uint16_t col1; //分針の色
  uint16_t col2; //時針の色
  uint16_t col3; //目盛の色
  uint16_t col4; //目盛の色
  cv_ckbase_digit.createSprite(221, 221);
  cv_ckbase.clear(TFT_BLACK);
  cv_ckhhand.clear(TFT_BLACK);
  cv_ckmhand.clear(TFT_BLACK);
  if (dialType == DIAL_CLASSIC) {
    col0 = TFT_WHITE;
    col1 = TFT_SKYBLUE;
    col2 = TFT_DARKCYAN;
    drawCircleWithAA(&cv_ckbase, ckCenterX, ckCenterY, 110, TFT_WHITE, TFT_BLACK);
    drawCircleWithAA(&cv_ckbase, ckCenterX, ckCenterY, 105, TFT_BLACK, TFT_WHITE);
  } else {
    col0 = M5.Display.color565(170, 170, 170);
    col1 = M5.Display.color565(170, 170, 170);
    col2 = M5.Display.color565(170, 170, 170);
    col3 = M5.Display.color565(100, 100, 100);
    col4 = M5.Display.color565(20, 20, 20);
    for (int8_t i = 0; i < 60; i++) {
      cv_ckbase.drawGradientLine((sin(i*0.105)*98)+ckCenterX, (cos(i*0.105)*98)+ckCenterY, (sin(i*0.105)*104)+ckCenterX, (cos(i*0.105)*104)+ckCenterY, col4, col3);
    }
  }
  float deg;
  for (int8_t i = 0; i < 12; i++) {
    deg = i*0.523;
    cv_ckbase_digit.setFont(&fonts::Font4);
    cv_ckbase_digit.setTextColor(col0, TFT_BLACK);
    cv_ckbase_digit.setTextSize(1);
    if (i == 6) {
      cv_ckbase_digit.drawCenterString("12", (sin(deg)*90)+ckCenterX, (cos(deg)*90)+ckCenterY-10);
    } else if (i % 3 == 0) {
      cv_ckbase_digit.drawCenterString(String(12-((i+6)%12)), (sin(deg)*90)+ckCenterX, (cos(deg)*90)+ckCenterY-10);
    } else if (dialType == DIAL_ZERODIAL) {
      drawCircleWithAA(&cv_ckbase, (sin(deg)*90)+ckCenterX, (cos(deg)*90)+ckCenterY, 3, col0, TFT_BLACK);
    } else {
      thickLine(cv_ckbase, (sin(deg)*97)+ckCenterX, (cos(deg)*97)+ckCenterY, (sin(deg)*106)+ckCenterX, (cos(deg)*106)+ckCenterY, col0);
    }
  }
  cv_ckbase_digit.pushSprite(0, 0, TFT_BLACK);
  cv_ckbase_digit.deleteSprite();
  cv_ckhhand.fillRect(0, 0, 7, 63, col2);
  cv_ckhhand.fillTriangle(3, 69, 0, 63, 6, 63, col2);
  cv_ckmhand.fillRect(0, 0, 5, 100, col1);
  cv_ckmhand.fillTriangle(2, 104, 0, 100, 4, 100, col1);
  if (dialType == DIAL_ZERODIAL) {
    cv_ckhhand.fillRect(1, 0, 5, 65, M5.Display.color565(238, 238, 238));
    cv_ckhhand.fillTriangle(3, 68, 1, 64, 5, 64, M5.Display.color565(238, 238, 238));
    cv_ckmhand.fillRect(1, 0, 3, 102, M5.Display.color565(238, 238, 238));
    cv_ckmhand.fillTriangle(2, 103, 1, 101, 3, 101, M5.Display.color565(238, 238, 238));
  }
}

uint16_t getBatteryVoltage() {
  if (showVoltage) return M5.Power.getBatteryVoltage();
  return 65535;
}

// ======================================
// ===== Application Functions (アプリ関数) =====
// ======================================

// Play a simple beep tone (440 Hz, 500ms)
void beep() {
  M5.Speaker.tone(440, 500);
}

// Settings

/* 電源設定

ジャイロ起動からのスリープの時間 5s 10s 15s 30s 1m
タッチからのスリープの時間 5s 10s 15s 30s 1m
自動シャットダウンの時間 1m 2m 3m 5m 10m 20m 30m 60m なし

*/

void powerOff() { M5.Power.powerOff(); }

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
void settings_loop();

bool settings_saved = true;
bool settings_choseBat = false;
bool settings_choseGyro = false;

String getVersionString(uint32_t ver) {
  uint8_t vYear = ver/100000;
  uint8_t vMonth = (ver/1000)%100;
  uint8_t vSSDate = (ver/10)%100;
  uint8_t vMinorUpdate = ver%10;
  if (vSSDate == 0) {
    if (vMinorUpdate == 0) return "V"+String(vYear)+"."+String(vMonth);
    else return "V"+String(vYear)+"."+String(vMonth)+"."+String(vMinorUpdate);
  } else {
    if (vMinorUpdate == 0) return "V"+String(vYear)+"."+String(vMonth)+"-SS"+String(vSSDate);
    else return "V"+String(vYear)+"."+String(vMonth)+"."+"-SS"+String(vSSDate)+"."+String(vMinorUpdate);
  }
}

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
    appUI.setItemRightColor("save", M5.Display.color888(0, 255, 0));
  } else {
    appUI.addRightLocaleToItem("save", "en", "Not Saved");
    appUI.addRightLocaleToItem("save", "ja", "未保存");
    appUI.setItemRightColor("save", M5.Display.color888(255, 0, 0));
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
  appUI.addItem("synctime", syncTimeOnSettings, "Sync Time from NTP");
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
  M5.Display.fillRect(0, 65, sizeX, sizeY, TFT_BLACK);
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
  M5.Display.fillRect(0, 65, sizeX, sizeY, TFT_BLACK);
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
  appUI.addRightLocaleToItem("model", "en", getModel());
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
    appUI.setItemRightColor("save", M5.Display.color888(0, 255, 0));
    appUI.makeUI(lang);
  }
}

void settings_loop() {
  appUI.update(dateTime, battery, getBatteryVoltage());
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
    appUI.setItemRightColor("save", M5.Display.color888(0, 255, 0));
  } else {
    appUI.addRightLocaleToItem("save", "en", "Not Saved");
    appUI.addRightLocaleToItem("save", "ja", "未保存");
    appUI.setItemRightColor("save", M5.Display.color888(255, 0, 0));
  }
  appUI.addItem("add", alarm_add, "+ Add Alarm");
  appUI.addLocaleToItem("add", "ja", "+ アラームを追加");
  uint8_t cnt = 0;
  for( JsonObject loopAlarm : alarmJson.as<JsonArray>() ) {
    int32_t hour = loopAlarm["hour"];
    int32_t min = loopAlarm["min"];
    appUI.addItem(String(cnt), alarm_config, forceDigits(hour, 2)+":"+forceDigits(min, 2));
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
    appUI.setItemRightColor("save", M5.Display.color888(0, 255, 0));
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
  M5.Display.fillRect(0, 65, sizeX, sizeY, TFT_BLACK);
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

void alarm_loop() {
  appUI.update(dateTime, battery, getBatteryVoltage());
}

// Stopwatch
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
  if (isVbus()) { cv_dtime_bat.setTextColor(CYAN, TFT_BLACK); }
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
    scrollsWhenNotTouch(&stwt_swipe, 5, sizeX, &stwt_chosenAtScroll, &stwt_nextChosenAtScroll);
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
      appUI.setItemColor("train1", TFT_LIGHTGRAY);
      appUI.setItemColor("train2", TFT_LIGHTGRAY);
      appUI.setItemColor("train3", TFT_LIGHTGRAY);
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
      appUI.setItemColor("train1", (colors[0][0]*65536)+(colors[0][1]*256)+(colors[0][2]));
      appUI.addLocaleToItem("train1", "en", type[0]+" "+dest[0]);
      appUI.setItemColor("train2", (colors[1][0]*65536)+(colors[1][1]*256)+(colors[1][2]));
      appUI.addLocaleToItem("train2", "en", type[1]+" "+dest[1]);
      appUI.setItemColor("train3", (colors[2][0]*65536)+(colors[2][1]*256)+(colors[2][2]));
      appUI.addLocaleToItem("train3", "en", type[2]+" "+dest[2]);
      if (isRemainingMode) {
        appUI.addRightLocaleToItem("train1", "en", String(min[0]-dateTime.time.minutes+(hourAdd[0]*60)));
        appUI.addRightLocaleToItem("train2", "en", String(min[1]-dateTime.time.minutes+(hourAdd[1]*60)));
        appUI.addRightLocaleToItem("train3", "en", String(min[2]-dateTime.time.minutes+(hourAdd[2]*60)));
      } else {
        appUI.addRightLocaleToItem("train1", "en", String(dateTime.time.hours+hourAdd[0])+":"+forceDigits(min[0], 2));
        appUI.addRightLocaleToItem("train2", "en", String(dateTime.time.hours+hourAdd[1])+":"+forceDigits(min[1], 2));
        appUI.addRightLocaleToItem("train3", "en", String(dateTime.time.hours+hourAdd[2])+":"+forceDigits(min[2], 2));
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
  timer_editing = 0;
  train_isMainUI = true;
  train_refleshTimer = 0;
  appUI.reset();
  appUI.setTitle("Timer");
  appUI.addLocaleToTitle("ja", "タイマー");
  appUI.setLocaleFont("en", 1);
  appUI.setLocaleFont("ja", 1);
  appUI.addItem("add", timer_make, "+ Add Timer");
  appUI.addLocaleToItem("add", "ja", "+ タイマーを追加");
  uint8_t cnt = 1;
  for(auto i = timers.begin(); i != timers.end(); i++ ) {
    uint32_t remains = (*i - millis())/1000;
    appUI.addItem(String(cnt), timer_edit, "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
    cnt++;
  }
  appUI.linkFunctionToBack(appEnd);
  appUI.makeUI(lang);
}

void timer_make() {
  appUI.reset();
  appUI.setTitle("Set Timer");
  appUI.setLocaleFont("en", 1);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "タイマーをセット");
  appUI.setTransparentMode(true);
  UIAddtional = UI_TIME;
  UIAddtionalSettings = TIMEUI_MODE_MINSEC;
  cv_uiadditional.createSprite(300, 165);
  UItimeLeft = alarmJson[alarm_toConfig]["hour"];
  UItimeRight = alarmJson[alarm_toConfig]["min"];
  M5.Display.fillRect(0, 65, sizeX, sizeY, TFT_BLACK);
  drawTimeUI();
  appUI.linkFunctionToBack(timer_start);
  appUI.makeUI(lang);
}

void timer_start() {
  UIAddtional = UI_NOTHING;
  cv_uiadditional.deleteSprite();
  uint16_t timerTime = ((UItimeLeft*60) + (UItimeRight));
  timers.push_back((timerTime*1000)+millis());
  timer_init();
}

void timer_edit(String id) {
  appUI.reset();
  timer_editing = id.toInt();
  timer_editingMillis = *std::next(timers.begin(), timer_editing-1);
  uint32_t remains = (timer_editingMillis - millis())/1000;
  appUI.setTitle("Edit Timer");
  appUI.setLocaleFont("en", 1);
  appUI.setLocaleFont("ja", 1);
  appUI.addLocaleToTitle("ja", "タイマーを編集");
  appUI.addItem("timer", nothing, String((uint8_t) floor(remains/60))+":"+String(remains%60));
  appUI.addItem("remove", timer_remove, "Remove Timer");
  appUI.addLocaleToItem("remove", "ja", "タイマーを削除");
  appUI.linkFunctionToBack(timer_init);
  appUI.makeUI(lang);
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
      appUI.addLocaleToItem(String(cnt), "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
      cnt++;
    }
    appUI.makeUI(lang);
  } else if (timer_editing != 0) {
    uint32_t remains = (timer_editingMillis - millis())/1000;
    appUI.addLocaleToItem("timer", "en", String((uint8_t) floor((remains)/60))+":"+String((remains)%60));
    appUI.makeUI(lang);
  }
  appUI.update(dateTime, battery, getBatteryVoltage());
}

// random
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
  appUI.setTitle("Wheel "+number);
  appUI.addLocaleToTitle("ja", "ルーレット "+number);
  appUI.setLocaleFont("en", 0);
  appUI.setLocaleFont("ja", 1);
  appUI.setTransparentMode(true);
  UIAddtional = UI_RANDOM;
  random_maxNumber = number.toInt();
  random_number = random(1, random_maxNumber+1);
  random_prevNumber = random(1, random_maxNumber+1);
  random_countdown = 10;
  M5.Display.fillRect(0, 65, sizeX, sizeY, TFT_BLACK);
  cv_uiadditional.createSprite(300, 165);
  appUI.linkFunctionToBack(random_init);
  appUI.makeUI(lang);
}

// Ext.Devices
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

// Draw analog clock face with battery ring and rotating hour/minute hands
void updateClock() {
  cv_ckbase.pushSprite(&cv_clock, 0, 0);
  uint8_t arcSize;
  if (dialType == DIAL_ZERODIAL) arcSize = 20;
  else arcSize = 108;
  if (isVbus()) {
    cv_clock.fillArc(ckCenterX, ckCenterY, arcSize+1, arcSize, 270, (battery*3.6)-90.5, CYAN);
  } else {
    cv_clock.fillArc(ckCenterX, ckCenterY, arcSize+1, arcSize, 270, (battery*3.6)-90.5, batcolor(battery));
  }
  cv_ckhhand.setPivot(2, 2);
  //アンチエイジングを入れても入れなくてもあまり（1フレーム当たり、80~90ms中5ms程度しか）負荷が変わらないことを確認しました
  //cv_ckhhand.pushRotated(&cv_clock, (180+((dateTime.time.hours*30)+(dateTime.time.minutes/2)))%360);
  cv_ckhhand.pushRotatedWithAA(&cv_clock, (180+((dateTime.time.hours*30)+(dateTime.time.minutes/2)))%360);
  cv_ckmhand.setPivot(1, 1);
  //cv_ckmhand.pushRotated(&cv_clock, (180+((dateTime.time.minutes*6)+(dateTime.time.seconds/10)))%360);
  cv_ckmhand.pushRotatedWithAA(&cv_clock, (180+((dateTime.time.minutes*6)+(dateTime.time.seconds/10)))%360);
  cv_clock.fillCircle(ckCenterX, ckCenterY, 8, M5.Display.color565(170, 170, 170));
  cv_clock.fillCircle(ckCenterX, ckCenterY, 5, TFT_BLACK);
}

// Update digital time display (HH:MM:SS), battery indicator, and special date info
void updateDigitals() {
  cv_dtime_bat.clear();
  cv_dtime_bat.setTextColor(TFT_WHITE, TFT_BLACK);
  cv_dtime_bat.drawString(forceDigits(dateTime.time.hours, 2)+":"+forceDigits(dateTime.time.minutes, 2)+" "+forceDigits(dateTime.time.seconds, 2), 0, 0, &fonts::Font2);
  if (isVbus()) { cv_dtime_bat.setTextColor(CYAN, TFT_BLACK); }
  if (showVoltage) {
    cv_dtime_bat.drawRightString(String(battery)+"% - "+String((float) M5.Power.getBatteryVoltage()/1000, 2)+"V", sizeX, 0, &fonts::Font2);
  } else {
    cv_dtime_bat.drawRightString(String(battery)+"%", sizeX, 0, &fonts::Font2);
  }
  cv_day.clear();
  uint8_t dateY;
  bool newVerAvailable = (latestVer > version);
  if (!spDatesJson[String(dateTime.date.month)].isNull()) {
    if (!spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)].isNull()) {
      dateY = 0;
      uint8_t birthdayCount = spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)].size();
      if (birthChangeTimer+5000 < millis()) {
        birthChangeTimer = millis();
        birthChangeID++;
        if (birthChangeID >= birthdayCount+newVerAvailable) {
          birthChangeID = 0;
        }
      }
      if (birthChangeID == birthdayCount) {
        cv_day.setTextColor(TFT_RED, TFT_BLACK);
        String dayName = "Update Available: "+getVersionString(latestVer);
        cv_day.drawString(dayName, 0, 17, &fonts::Font2);
      } else {
        int32_t dayColor = spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["color"];
        cv_day.setTextColor(M5.Display.color24to16(dayColor), TFT_BLACK);
        String dayName = spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["name"];
        cv_day.drawString(dayName, 0, 17, &fonts::Font2);
      }
    } else if (newVerAvailable) {
      dateY = 0;
      cv_day.setTextColor(TFT_RED, TFT_BLACK);
      String dayName = "Update Available: "+getVersionString(latestVer);
      cv_day.drawString(dayName, 0, 17, &fonts::Font2);
    } else {
      dateY = 17;
    }
  } else if (newVerAvailable) {
    dateY = 0;
    cv_day.setTextColor(TFT_RED, TFT_BLACK);
    String dayName = "Update Available: "+getVersionString(latestVer);
    cv_day.drawString(dayName, 0, 17, &fonts::Font2);
  } else {
    dateY = 17;
  }
  cv_day.setTextColor(TFT_WHITE, TFT_BLACK);
  cv_day.drawString(String(dateTime.date.month)+"/"+String(dateTime.date.date)+" "+week[dateTime.date.weekDay]+" "+dateTime.date.year, 0, dateY, &fonts::Font2);
  //int32_t ramFree = (int32_t) (heap_caps_get_free_size(MALLOC_CAP_8BIT));
  //int32_t ramTotal = (int32_t) (heap_caps_get_total_size(MALLOC_CAP_8BIT));
  //cv_day.drawRightString((String) ((ramTotal-ramFree)/1000)+"KB/"+(String) (ramTotal/1000)+"KB "+prevLoopTime+"ms/f", sizeX, 17, &fonts::Font2);
}

bool copySDtoSPI() {
  if (SD.begin(4, SPI)) {
    File folder = SD.open("/littlefs");
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
      String SDPath = "/LittleFS/"+fileName;
      String SPIPath = "/"+fileName;
      if (LittleFS.exists(fileName)) {LittleFS.remove(fileName);}
      File SDCardFile = SD.open(SDPath, FILE_READ);
      File SPIFile = LittleFS.open(SPIPath, FILE_WRITE);
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

void wifiInitialSetup() {
  bool notConnected = true;
  String trySSID = "";
  String tryPass = "";
  while (notConnected) {
    M5.Display.clear();
    M5.Display.setCursor(0, 0);
    M5.Display.setFont(&fonts::Font4);
    M5.Display.setTextColor(SKYBLUE, TFT_BLACK);
    M5.Display.println("Wi-Fi Setup");
    M5.Display.setFont(&fonts::Font2);
    if (!trySSID.isEmpty()) {
      M5.Display.setTextColor(RED, TFT_BLACK);
      M5.Display.println("Failed to Connect "+trySSID+".");
    }
    M5.Display.setTextColor(WHITE, TFT_BLACK);
    M5.Display.println("1. Connect to Wi-Fi \"MK75-Setup\" using your smartphone or computer.");
    M5.Display.println("2. Scan the QR code or access the URL.");
    M5.Display.println("3. Follow the on-screen instructions.");
    String SSIDs = "";
    int32_t n = WiFi.scanNetworks();
    for (int32_t i = 0; i < n; i++) {
      SSIDs += "<option>";
      SSIDs += WiFi.SSID(i);
      SSIDs += "</option>";
    }
    WiFi.softAP("MK75-Setup");
    delay(100);
    WiFi.softAPConfig(ip, ip, subnet);
    IPAddress myIP = WiFi.softAPIP();
    server.begin();
    M5.Display.setTextColor(YELLOW, TFT_BLACK);
    M5.Display.println("http://192.168.10.75/wifi-setup/");
    M5.Display.fillRect(0, sizeY-120, 120, 120, WHITE);
    M5.Display.qrcode("http://192.168.10.75/wifi-setup/", 10, sizeY-110, 100, 2);
    bool notSSIDReady = true;
    while (notSSIDReady) {
      WiFiClient client = server.available();
      if (client) {
        client.setTimeout(1000);
        String request = client.readStringUntil('\n');
        // Serial.println(request);
        while (client.available()) client.read();
        if (request.endsWith("\r")) request = request.substring(0, request.length()-1);
        std::list<String> reqSplit = split(request, ' ');
        std::list<String>::iterator reqSplitItr = reqSplit.begin();
        if (*reqSplitItr == "GET") {
          reqSplitItr++;
          std::list<String> dirSplit = split(*reqSplitItr, '/');
          std::list<String>::iterator dirSplitItr = dirSplit.begin();
          uint32_t dirSplitLength = dirSplit.size();
          // Serial.println(*dirSplitItr);
          // Serial.println(dirSplitLength);
          if (dirSplitLength == 0) {
            client.print(Error404);
          } else if (*dirSplitItr == "wifi-setup") {
            if (dirSplitLength == 1) {
              client.print(WiFiSetForm(false, SSIDs));
            } else {
              dirSplitItr++;
              client.print(WiFiSetForm(true, ""));
              notSSIDReady = false;
              trySSID = *dirSplitItr;
              if (dirSplitLength != 2) {
                dirSplitItr++;
                tryPass = *dirSplitItr;
              }
            }
          } else {
            client.print(Error404);
          }
        }
        client.stop();
        long disconWaitTimer = millis();
        while ((client.connected() and (millis()-disconWaitTimer) <= 10000)) {
          delay(100);
        }
        if (client.connected()) continue;
      }
    }
    server.end();
    WiFi.disconnect(false);
    delay(100);
    M5.Display.clear();
    M5.Display.setCursor(0, 0);
    M5.Display.setFont(&fonts::Font4);
    M5.Display.setTextColor(SKYBLUE, TFT_BLACK);
    M5.Display.println("Wi-Fi Setup");
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextColor(WHITE, TFT_BLACK);
    M5.Display.println("Connecting "+trySSID+"...");
    long wifiTimer = millis();
    WiFi.begin(trySSID, tryPass);
    while (!(WiFi.status() == WL_CONNECTED or (millis()-wifiTimer) > 10000)) {
      delay(1000);
      // Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) notConnected = false;
  }
  M5.Display.println("Successed. Saving Wi-Fi Settings...");
  wifiJson.clear();
  wifiJson[trySSID] = tryPass;
  writeSPIJson("/wifi.json", &wifiJson);
  M5.Display.println("Finished. Syncing time...");
  // Serial.println(WiFi.status());
  syncTime();
  M5.Display.println("Finished.");
  WiFi.disconnect(true);
  String langStr = "en";
  uint32_t slpTime = 252642565;
  uint8_t slpFlags = 7;
  dialType = (dialtype_t) 0;
  lang[0] = langStr.charAt(0);
  lang[1] = langStr.charAt(1);
  delay(1000);
}

void setupConfigs() {
  M5.Display.setTextColor(SKYBLUE, TFT_BLACK);
  M5.Display.println("MK75-Watch");
  M5.Display.setTextColor(WHITE, TFT_BLACK);
  M5Model = getModel();
  M5.Display.print("Model: ");
  if (modelType == 2 || modelType == 10) {
    M5.Display.setTextColor(GREEN, TFT_BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Supported Model");
  } else {
    M5.Display.setTextColor(RED, TFT_BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Unsupported Model");
  }
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.print("Loading Preferences...");
  Preferences pref;
  pref.begin("mk75_settings");
  String langStr = pref.getString("lang", "en");
  uint32_t slpTime = pref.getUInt("slpTime", 252642565);
  uint8_t slpFlags = pref.getUChar("slpFlags", 7);
  showVoltage = pref.getBool("showVoltage", false);
  latestVer = pref.getUInt("latestVer", version);
  dialType = (dialtype_t) pref.getUChar("dialType", 0);
  uint8_t chargeSettings = pref.getUChar("charge", 0x23);
  decombineHex(chargeSettings, &chargeCurrent, &chargeVoltage);
  M5.Power.setChargeCurrent(min(chargeCurrent*100, 300));
  M5.Power.setChargeVoltage(min((chargeVoltage*100)+4000, 4200));
  uint8_t screenVolumeAndVibration = pref.getUChar("hmi", (0x80*(modelType==2))+52); // Human machine interface
  vibration = (screenVolumeAndVibration >> 7) & 0x01;
  uint8_t screenAndVolume = screenVolumeAndVibration & 0x7F;
  screenBrightness = screenAndVolume%10;
  speakerVolume = screenAndVolume/10;
  pref.end();
  M5.Display.setBrightness(255*((screenBrightness+1)/10.0));
  lang[0] = langStr.charAt(0);
  lang[1] = langStr.charAt(1);
  for (uint8_t i = 0; i < 4; i++) {
    sleepTimings[i] = (slpTime >> (24-(i*8))) & 0xFF;
    sleepTimings_sys[i] = 60000-(sleepTimings[i]*1000);
  }
  extSettings[0] = (extsettings_t) ((slpFlags >> 4) & 0x03);
  extSettings[1] = (extsettings_t) ((slpFlags >> 2) & 0x03);
  doSleep[0] = (slpFlags >> 1) & 0x01;
  doSleep[1] = (slpFlags) & 0x01;
  M5.Display.println("Success");
  if (!LittleFS.begin()) {
    M5.Display.print("Formatting LittleFS (It takes long time) ...");
    LittleFS.format();
    M5.Display.println("Success");
  }
  //SFE.begin();
  M5.Display.print("Copying SD Card's contents to LittleFS...");
  M5.Display.println(successOrFail(copySDtoSPI()));
  M5.Display.print("Loading wifi json...");
  bool wifiJsonAvailable = readSPIJson("/wifi.json", &wifiJson, 10000);
  M5.Display.println(successOrFail(wifiJsonAvailable));
  if (wifiJsonAvailable) {
    M5.Display.print("Loading train json...");
    M5.Display.println(successOrFail(readSPIJson("/train.json", &trainJson, 10000)));
    M5.Display.print("Loading alarm json...");
    M5.Display.println(successOrFail(readSPIJson("/alarm.json", &alarmJson, 10000)));
    setRTCAlarmIRQ();
    M5.Display.print("Loading special dates json...");
    M5.Display.println(successOrFail(readSPIJson("/special_dates.json", &spDatesJson, 10000)));
    delay(1000);
    checkAlarm();
    M5.Display.clear();
  } else {
    delay(1000);
    wifiInitialSetup();
  }
}

void setupSprites() {
  cv_display.createSprite(sizeX, sizeY);
  cv_clock.createSprite(221, 221);
  cv_menu.createSprite(100, 240);
  cv_ckbase.createSprite(221, 221);
  cv_ckhhand.createSprite(7, 70);
  cv_ckmhand.createSprite(5, 105);
  makeClockBase();
  cv_dtime_bat.createSprite(sizeX, 17);
  cv_day.createSprite(sizeX, 34);
}

// Draw application menu with app icons and names (vertical scrollable list)
void drawMenu() {
  cv_menu.clear();
  for (int8_t i = 0; i < howManyApps; i++) {
    if (inLimit(i*120, screenSwipeVertical-220, screenSwipeVertical+220)) {
      // パフォーマンス重視ならfillRoundRectではなくfillRectを使うべきかもしれない
      cv_menu.fillRoundRect(105-screenSwipe, (i*120)+75-screenSwipeVertical, 90, 90, 5, WHITE);
      cv_menu.fillRoundRect(110-screenSwipe, (i*120)+80-screenSwipeVertical, 80, 80, 4, BLACK);
      cv_menu.pushImage(110-screenSwipe, (i*120)+80-screenSwipeVertical, 80, 80, icons[i]);
      if (strcmp(lang, "ja") == 0) {
        cv_menu.drawCenterString(appsJa[i], 150-screenSwipe, (i*120)+170-screenSwipeVertical, &fonts::efontJA_16);
      } else if (strcmp(lang, "en") == 0) {
        cv_menu.drawCenterString(appsEn[i], 150-screenSwipe, (i*120)+170-screenSwipeVertical, &fonts::Font2);
      } else {
        //cv_menu.drawCenterString("ERROR. LOL", (ix*105)+(sizeX/2), ((iy*85)-45)+(sizeY/2), &fonts::Font2);
      }
    }
  }
}

void loopSleep() {
  bool touch = M5.Touch.getCount() > 0;
  if ((touch) or (vibTimer > millis())) {
    if (isVbus() and slpTimer > sleepTimings_sys[0]) slpTimer = sleepTimings_sys[0];
    else if (slpTimer > sleepTimings_sys[1]) slpTimer = sleepTimings_sys[1];
  } else if (checkGyro() and (!isVbus()) and (slpTimer > sleepTimings_sys[3])) {
    slpTimer = sleepTimings_sys[3];
  } else {
    slpTimer += prevLoopTime;
    if (slpTimer >= 60000 || M5.BtnPWR.getState() == m5::Button_Class::state_clicked) {
      bool charged = isVbus();
      if (charged) {
        lowPowSleep();
      } else {
        activeSleep();
      }
      if (isVbus() and !charged) {
        vibTimer = 500;
        M5.Power.setVibration(127);
      }
      afterSlp = true;
      M5.Display.wakeup();
      M5.Display.setBrightness(255*((screenBrightness+1)/10.0));
      if (isVbus()) slpTimer = sleepTimings_sys[0];
      else slpTimer = sleepTimings_sys[1];
    }
  }
}

// ======================================
// ===== Main Loop Functions (メインループ処理) =====
// ======================================

// Handle menu swipe gestures and app selection
void loopMenuTouch() {
  if (!afterSlp) {
    if (M5.Touch.getCount() > 0 || touchInterrupted) {
      m5::Touch_Class::touch_detail_t tDetail = M5.Touch.getDetail();
      if (touchInterrupted) {
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
          for (int8_t i = 0; i < howManyApps; i++) {
            if (inLimit(tDetail.x, 225, 315) and inLimit(tDetail.y, (i*120)+75-screenSwipeVertical, (i*120)+165-screenSwipeVertical)) {
              vibTimer = 200;
              M5.Power.setVibration(127);
              appInit = true;
              // アプリ追加するときはここに手を加えよう
              if (i == 0) {
                nowApp = APP_TIMER;
              } else if (i == 1) {
                nowApp = APP_ALARM;
              } else if (i == 2) {
                nowApp = APP_STOPWATCH;
              } else if (i == 3) {
                nowApp = APP_TRAIN;
              } else if (i == 4) {
                nowApp = APP_RANDOM;
              } else if (i == 5) {
                nowApp = APP_EXT_DEVICE;
              } else if (i == 6) {
                nowApp = APP_SETTINGS;
              }
            }
          } 
        }
      }
      touchInterrupted = false;
    } else {
      scrollsWhenNotTouch(&screenSwipeVertical, howManyApps, 120, &chosenAtScroll, &nextChosenAtScroll, false, 5, 10);
    }
  }
}

// Handle time/date picker button input for HH:MM, MM:SS, or MM/DD modes
void loopTimeSel() {
  int8_t UItimeLeftMax;
  int8_t UItimeLeftMin;
  int8_t UItimeRightMax;
  int8_t UItimeRightMin;
  if (UIAddtionalSettings == TIMEUI_MODE_HOURMIN) {
    UItimeLeftMax = 23;
    UItimeLeftMin = 0;
    UItimeRightMax = 59;
    UItimeRightMin = 0;
  } else if (UIAddtionalSettings == TIMEUI_MODE_MINSEC) {
    UItimeLeftMax = 99;
    UItimeLeftMin = 0;
    UItimeRightMax = 59;
    UItimeRightMin = 0;
  } else if (UIAddtionalSettings == TIMEUI_MODE_DATE) {
    UItimeLeftMax = 12;
    UItimeLeftMin = 1;
    UItimeRightMax = getMonthMaxDay(UItimeLeft, true);
    UItimeRightMin = 1;
  } 
  if (M5.Touch.getCount() > 0) {
    m5::Touch_Class::touch_detail_t tDetail = M5.Touch.getDetail();
    if (tDetail.wasPressed() || tDetail.isHolding()) {
      if (inLimit(tDetail.x, 40, 140) && inLimit(tDetail.y, 50, 110)) {
        UItimeLeft++;
        if (UItimeLeft > UItimeLeftMax) UItimeLeft = UItimeLeftMin;
        if (UIAddtionalSettings == TIMEUI_MODE_DATE and UItimeRight > UItimeRightMax) UItimeRight = UItimeRightMax;
        drawTimeUI();
      } else if (inLimit(tDetail.x, 180, 280) && inLimit(tDetail.y, 50, 110)) {
        UItimeRight++;
        if (UItimeRight > UItimeRightMax) UItimeRight = UItimeRightMin;
        if (UIAddtionalSettings == TIMEUI_MODE_DATE and UItimeRight > UItimeRightMax) UItimeRight = UItimeRightMax;
        drawTimeUI();
      } else if (inLimit(tDetail.x, 40, 140) && inLimit(tDetail.y, 175, 235)) {
        UItimeLeft--;
        if (UItimeLeft < UItimeLeftMin) UItimeLeft = UItimeLeftMax;
        drawTimeUI();
      } else if (inLimit(tDetail.x, 180, 280) && inLimit(tDetail.y, 175, 235)) {
        UItimeRight--;
        if (UItimeRight < UItimeRightMin) UItimeRight = UItimeRightMax;
        drawTimeUI();
      }
    }
  }
  cv_uiadditional.pushSprite(centerX-150, centerY-50);
}

void touchTask(void *pvParameters) {
  uint32_t taskTimer;
  while(1) {
    taskTimer = millis();
    if (nowApp == APP_NOTHING) {
      M5.update();
      loopMenuTouch();
      // アプリサイドバーの調整
      if (appMenu) {
          // 目標値100
          screenSwipe = ceil((screenSwipe-100)/1.2)+100;
      } else {
          // 目標値0
          screenSwipe = floor(screenSwipe/1.2);
      }
    }
    uint32_t elapsedTime = calculateElapsedTime(taskTimer, millis());
    if (elapsedTime < 25) {
      vTaskDelay(25 - elapsedTime / portTICK_PERIOD_MS);
    }
  }
}

// ======================================
// ===== Arduino Setup & Main Loop ======
// ======================================

// Initialize hardware, load settings, prepare sprites and display
void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);
  M5.Power.begin();
  M5.Imu.begin();
  M5.Rtc.begin();
  M5.Display.init();
  dateTime = M5.Rtc.getDateTime();
  setupConfigs();
  setupSprites();
  //setupMenu();
  wasVBUS = isVbus();
  compatibleAttachInterrupt();
  bool powered = isVbus();
  if (powered) slpTimer = sleepTimings_sys[0];
  else slpTimer = sleepTimings_sys[1];
  updateExtOutput(true, powered);
  updateClock();
  xTaskCreatePinnedToCore(
    touchTask,       // 実行する関数
    "touchTask",     // タスクの名前
    4096,          // メモリサイズ（バイト）
    NULL,          // 渡す引数
    0,             // 優先度（数字が大きいほど高い）
    NULL,          // タスクハンドル
    0              // コア番号（0 または 1）
  );
  xMutex = xSemaphoreCreateMutex();
  mainLoopTimer = micros();
}

// Main event loop - handles display updates, touch input, app logic, and power management
void loop() {
  int32_t tmrStart = millis();
  //SFE.update();
  doDraw = true;
  bool touch = (M5.Touch.getCount() > 0);
  bool powered = isVbus();
  if (doSleep[powered]) {
    loopSleep();
  } else {
    if (autoShutdown && timers.size() == 0) {
      if (checkAccel()) {
        shutdownTimer += 1;
      } else {
        shutdownTimer = 0;
      }
      if (shutdownTimer == 600) {
        connectWiFiAndTimeSync();
        M5.Power.powerOff();
      }
    }
  }
  if (vibTimer > 0) {
    M5.Power.setVibration(127);
    if (vibTimer < prevLoopTime) vibTimer = 0;
    else vibTimer -= prevLoopTime;
  } else {
    M5.Power.setVibration(0);
  }
  if (wasVBUS != powered) {
    if (powered) {
      vibTimer = 500;
      M5.Power.setVibration(127);
    }
    updateExtOutput(true, powered);
  }
  cv_display.clear();
  wasVBUS = powered;
  updateDateTimeBat();
  if (M5.BtnB.wasPressed()) {
    screenSwipe = 0;
    if (nowApp != APP_NOTHING) appEnd();
  }
  if (M5.BtnC.wasPressed()) {}
  // 時間入力
  if (UIAddtional == UI_TIME) {
    loopTimeSel();
  }
  if (nowApp != APP_NOTHING) {
    M5.update();
  }
  if (nowApp == APP_SETTINGS) {
    if (appInit) {
      appInit = false;
      settings_init();
    }
    settings_loop();
  } else if (nowApp == APP_TRAIN) {
    if (appInit) {
      appInit = false;
      train_init();
    }
    train_loop();
  } else if (nowApp == APP_ALARM) {
    if (appInit) {
      appInit = false;
      alarm_init();
    }
    alarm_loop();
  } else if (nowApp == APP_STOPWATCH) {
    if (appInit) {
      appInit = false;
      stopwatch_init();
    }
    stopwatch_loop();
  } else if (nowApp == APP_TIMER) {
    if (appInit) {
      appInit = false;
      timer_init();
    }
    timer_loop();
  } else if (nowApp == APP_RANDOM) {
    if (appInit) {
      appInit = false;
      random_init();
    }
    random_loop();
  } else if (nowApp == APP_EXT_DEVICE) {
    if (appInit) {
      appInit = false;
      edev_init();
    }
    edev_loop();
  } else {
    // アプリサイドバーの開閉
    updateClock();
    updateDigitals();
    cv_dtime_bat.pushSprite(&cv_display, 0, 0, TFT_BLACK);
    if (screenSwipe != 0) {
      drawMenu();
    }
    cv_clock.pushSprite(&cv_display, centerX-ckCenterX-round(screenSwipe/2), centerY-ckCenterY, TFT_BLACK);
    cv_day.pushSprite(&cv_display, 0, sizeY-34, TFT_BLACK);
    cv_dtime_bat.pushSprite(&cv_display, 0, 0, TFT_BLACK);
    if (screenSwipe != 0) {
      cv_menu.pushSprite(&cv_display, 220, 0, TFT_BLACK);
    }
    cv_display.pushSprite(0, 0);
    if ((screenSwipe == 0 || (screenSwipe == 100 && screenSwipeVertical%120 == 0)) && doDraw && !touch) {
      if (!lowpower) {
        setCpuFrequencyMhz(80);
        lowpower = true;
      }
    } else {
      if (lowpower) {
        setCpuFrequencyMhz(240);
        lowpower = false;
      }
    }
    mainLoopTimer = micros();
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
    prevLoopTime = calculateElapsedTime(tmrStart, millis());
    //Serial.println("Loop Time: "+String(prevLoopTime)+"ms");
    //Serial.println("Vib Time: "+String(vibTimer)+"ms");
  }
}
