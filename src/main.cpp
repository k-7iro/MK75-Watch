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
#include "main.hpp"
#include "new"

#define NTP_TIMEZONE "JST-9" // 日本標準時 / Japan Standard Time (UTC+9) in seconds
#define NTP_SERVER1 "ntp.nict.jp"
#define NTP_SERVER2 "ntp.jst.mfeed.ad.jp"
#define NTP_SERVER3 "time.google.com"

// =====================================
// ==  Canvas Buffers for Rendering  ==
// =====================================
// 描画用Canvas：レイヤー構造で各要素を個別に描画し、最後に合体する
// Rendering layers: Main display, clock, menu, and component buffers

M5Canvas cv_display(&M5.Display);
M5Canvas cv_clock(&cv_display);
M5Canvas cv_menu(&cv_display);
M5Canvas cv_ckbase(&cv_display);
M5Canvas cv_ckbase_digit(&cv_ckbase);
M5Canvas cv_dtime_bat(&cv_display);
M5Canvas cv_day(&cv_clock);
M5Canvas cv_ckhhand(&cv_clock);
M5Canvas cv_ckmhand(&cv_clock);

M5Canvas cv_stwt1(&cv_display);
M5Canvas cv_stwt2(&cv_display);
M5Canvas cv_stwt3(&cv_display);
M5Canvas cv_stwt4(&cv_display);
M5Canvas cv_stwt5(&cv_display);
M5Canvas cv_stwt_top(&cv_display);

M5Canvas cv_uiadditional(&M5.Display); // 時刻選択UI用Canvas / Canvas for time selection UI

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

const int32_t ckCenterX = 110;
const int32_t ckCenterY = 110;
const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
const String apps[7] = {"timer", "alarm", "stopwatch", "train", "random", "external", "settings"};
const String appsEn[7] = {"Timer", "Alarm", "Stopwatch", "TrainTime", "Random", "Ext.Device", "Settings"};
const String appsJa[7] = {"タイマー", "アラーム", "ストップWt", "交通時刻表", "ランダム", "外部デバイス", "設定"};
const uint8_t howManyApps = 7;
const uint32_t version = 2608280; // [version]
const int32_t sizeX = 320;
const int32_t centerX = 160;
const int32_t sizeY = 240;
const int32_t centerY = 120;

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
m5::board_t modelType; // 2 for Core2, 10 for CoreS3

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

inline void compatibleAttachInterrupt() {
  if (modelType == m5::board_t::board_M5StackCoreS3 || modelType == m5::board_t::board_M5StackCoreS3SE) {
    attachInterrupt(GPIO_NUM_21, touchInterrupt, FALLING);
  } else {
    attachInterrupt(GPIO_NUM_39, touchInterrupt, FALLING);
  }
}

inline void compatibleDetachInterrupt() {
  if (modelType == m5::board_t::board_M5StackCoreS3 || modelType == m5::board_t::board_M5StackCoreS3SE) {
    detachInterrupt(GPIO_NUM_21);
  } else {
    detachInterrupt(GPIO_NUM_39);
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

void compatibleLightSleep(uint32_t duration, bool noInterrupt) {
  if (noInterrupt) {
    //noInterrupts();
    compatibleDetachInterrupt();
  }
  if (modelType == m5::board_t::board_M5StackCoreS3 || modelType == m5::board_t::board_M5StackCoreS3SE) {
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
        if (loopAlarm["onetime"]) {
          loopAlarm["weekday"] = false;
          loopAlarm["weekend"] = false;
          writeSPIJson("/alarm.json", &alarmJson);
        }
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
void scrollsWhenTouch(m5::touch_detail_t detail, int32_t* target, bool vertical) {
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
void scrollsWhenNotTouch(int32_t* target, int32_t indexes, int32_t distant, bool useAcc, float speed, uint8_t maxAccMulti) {
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
    int32_t calib = constrain(maxAcc*maxAccMulti, -(distant/2), distant/2);
    int32_t swipeTarget = constrain(round(((float) (*target+calib)/distant))*distant, 0, (indexes-1)*distant);
    *target += (swipeTarget-*target)/speed;
    if (abs(*target-swipeTarget) <= 1) {
      *target = swipeTarget;
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
  bool charged = M5.Power.Axp2101.isVBUS();
  M5.Display.clear();
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  M5.Imu.sleep();
  lowpower = true;
  updateExtOutput(false, charged);
  while (!((touch > 0) or (charged != M5.Power.Axp2101.isVBUS()))) {
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
    setCpuFrequencyMhz(80);
    M5.update();
    touch = M5.Touch.getCount();
  }
  updateExtOutput(true, M5.Power.Axp2101.isVBUS());
  M5.Imu.begin();
}

void activeSleep() {
  uint8_t touch = 0;
  uint8_t alarmTimer = 0;
  uint16_t lowPowTimer = 0;
  bool charged = M5.Power.Axp2101.isVBUS();
  M5.Display.clear();
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  lowpower = true;
  updateExtOutput(false, charged);
  while (!((touch > 0) or (charged != M5.Power.Axp2101.isVBUS()))) {
    compatibleLightSleep(100000, true);
    setCpuFrequencyMhz(80);
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
  updateExtOutput(true, M5.Power.Axp2101.isVBUS());
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
      thickLine(&cv_ckbase, (sin(deg)*97)+ckCenterX, (cos(deg)*97)+ckCenterY, (sin(deg)*106)+ckCenterX, (cos(deg)*106)+ckCenterY, col0);
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

// ======================================
// ===== Application Functions (アプリ関数) =====
// ======================================

// Settings
extern void settings_init();
extern void settings_loop();

// Alarm
extern void alarm_init();
extern void alarm_loop();

// Stopwatch
extern void stopwatch_init();
extern void stopwatch_loop();

// Train
extern void train_init();
extern void train_loop();

// Timer
extern void timer_init();
extern void timer_loop();

// Random
extern void random_init();
extern void random_loop();

// Ext.Devices
extern void edev_init();
extern void edev_loop();

// Draw analog clock face with battery ring and rotating hour/minute hands
void updateClock() {
  cv_ckbase.pushSprite(&cv_clock, 0, 0);
  uint8_t arcSize;
  if (dialType == DIAL_ZERODIAL) arcSize = 20;
  else arcSize = 108;
  if (M5.Power.Axp2101.isVBUS()) {
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

void drawUpdate() {
  cv_day.setTextColor(TFT_RED, TFT_BLACK);
  String dayName = "Update Available: "+getVersionString(latestVer);
  cv_day.drawString(dayName, 0, 17, &fonts::Font2); 
}

// Update digital time display (HH:MM:SS), battery indicator, and special date info
void updateDigitals() {
  cv_dtime_bat.clear();
  cv_dtime_bat.setTextColor(TFT_WHITE, TFT_BLACK);
  cv_dtime_bat.drawString(forceDigits(dateTime.time.hours, 2)+":"+forceDigits(dateTime.time.minutes, 2)+" "+forceDigits(dateTime.time.seconds, 2), 0, 0, &fonts::Font2);
  if (M5.Power.Axp2101.isVBUS()) { cv_dtime_bat.setTextColor(CYAN, TFT_BLACK); }
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
        drawUpdate();
      } else {
        //int32_t dayColor = spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["color"];
        //cv_day.setTextColor(M5.Display.color24to16(dayColor), TFT_BLACK);
        cv_day.setTextColor(TFT_WHITE, TFT_BLACK);
        String dayName = spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["name"];
        cv_day.drawString(dayName, 0, 17, &fonts::Font2);
        uint16_t dayColor1 = cv_day.color24to16(spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["color1"]);
        uint16_t dayColor2 = cv_day.color24to16(spDatesJson[String(dateTime.date.month)][String(dateTime.date.date)][birthChangeID]["color2"]);
        gradientMask(&cv_day, 0, 17, cv_day.textWidth(dayName, &fonts::Font2), 17, dayColor1, dayColor2, TFT_WHITE);
      }
    } else if (newVerAvailable) {
      dateY = 0;
      drawUpdate();
    } else {
      dateY = 17;
    }
  } else if (newVerAvailable) {
    dateY = 0;
    drawUpdate();
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
  modelType = M5.getBoard();
  M5Model = getModel(modelType);
  M5.Display.print("Model: ");
  if (modelType == m5::board_t::board_M5StackCore2 || modelType == m5::board_t::board_M5StackCoreS3) {
    M5.Display.setTextColor(GREEN, TFT_BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Supported Model");
  } else if (modelType == m5::board_t::board_M5StackCoreS3SE) {
    M5.Display.setTextColor(YELLOW, TFT_BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Semi-Supported Model");
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
    if (M5.Power.Axp2101.isVBUS() and slpTimer > sleepTimings_sys[0]) slpTimer = sleepTimings_sys[0];
    else if (slpTimer > sleepTimings_sys[1]) slpTimer = sleepTimings_sys[1];
  } else if (checkGyro() and (!M5.Power.Axp2101.isVBUS()) and (slpTimer > sleepTimings_sys[3])) {
    slpTimer = sleepTimings_sys[3];
  } else {
    slpTimer += prevLoopTime;
    if (slpTimer >= 60000 || M5.BtnPWR.getState() == m5::Button_Class::state_clicked) {
      bool charged = M5.Power.Axp2101.isVBUS();
      if (charged) {
        lowPowSleep();
      } else {
        activeSleep();
      }
      if (M5.Power.Axp2101.isVBUS() and !charged) {
        vibTimer = 500;
        M5.Power.setVibration(127);
      }
      afterSlp = true;
      M5.Display.wakeup();
      M5.Display.setBrightness(255*((screenBrightness+1)/10.0));
      if (M5.Power.Axp2101.isVBUS()) slpTimer = sleepTimings_sys[0];
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
    if (M5.Touch.getCount() > 0) {
      m5::Touch_Class::touch_detail_t tDetail = M5.Touch.getDetail();
      if (tDetail.wasClicked() and tDetail.y < 240) {
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
      scrollsWhenNotTouch(&screenSwipeVertical, howManyApps, 120, false, 5, 10);
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
  wasVBUS = M5.Power.Axp2101.isVBUS();
  compatibleAttachInterrupt();
  bool powered = M5.Power.Axp2101.isVBUS();
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
  bool powered = M5.Power.Axp2101.isVBUS();
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
