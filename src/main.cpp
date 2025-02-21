#include <M5Unified.h>
#include <M5GFX.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <assets/images.hpp>
#include "libs/NanaUI.hpp"
#include "new"

static M5Canvas cv_display(&M5.Display);
static M5Canvas cv_clock(&cv_display);
static M5Canvas cv_menu(&cv_display);
static M5Canvas cv_ckbase(&cv_display);
static M5Canvas cv_dtime_bat(&cv_clock);
static M5Canvas cv_day(&cv_clock);
static M5Canvas cv_ckhhand(&cv_clock);
static M5Canvas cv_ckmhand(&cv_clock);

UI *appUI;
JsonDocument wifi;
JsonDocument train;

const String lang = "ja";
String nowApp = "";

const int sizeX = 320;
const int centerX = sizeX/2;
const int sizeY = 240;
const int centerY = sizeY/2;
const int ckCenterX = 110;
const int ckCenterY = 110;
const int JST = 32400;
const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
const String apps[6] = {"timer", "settings", "alarm", "music", "stopwatch", "dbmeter"};
const String appsEn[6] = {"Timer", "Settings", "Alarm", "Music", "Stopwatch", "dB Meter"};
const String appsJa[6] = {"タイマー", "設定", "アラーム", "音楽", "ストップ\nウォッチ", "dBメーター"};
String M5Model;

int slpTimer = 0;
int vibTimer = 0;
int screenSwipe = 0;
int prevTX = 0;
int prevTY = 0;
int firstTX = 0;
int firstTY = 0;
int cycle = 0;
int prevLoopTime = 0;
int prevSwipeAcc[6] = {0, 0, 0, 0, 0, 0};
int appStart = 0;

float mpu[6] = {0, 0, 0, 0, 0, 0};

bool wasVBUS = false;
bool wasTouched = false;
bool haveToDeleteAppUI = false;
bool afterSlp = false;

uint8_t battery = M5.Power.getBatteryLevel();
m5::rtc_datetime_t dateTime = M5.Rtc.getDateTime();

void nothing() {}

void thickLine(M5Canvas target, int x0, int y0, int x1, int y1, int color) {
  target.drawLine(x0, y0, x1, y1, color);
  target.drawLine(x0+1, y0, x1+1, y1, color);
  target.drawLine(x0-1, y0, x1-1, y1, color);
  target.drawLine(x0, y0+1, x1, y1+1, color);
  target.drawLine(x0, y0-1, x1, y1-1, color);
}

int limit(int num, int min, int max) {
  if (num < min) return min;
  else if (num > max) return max;
  return num;
}

uint32_t batcolor(int bat) {
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

// Powered by ChatGPT
bool isHoliday(int month, int day, int weekday) {
  // 元日 (1月1日)
  if (month == 1 && day == 1) return true;

  // 成人の日 (1月の第2月曜日)
  if (month == 1 && weekday == 1 && day >= 8 && day <= 14) return true;

  // 建国記念の日 (2月11日)
  if (month == 2 && day == 11) return true;

  // 春分の日 (仮: 3月20日)
  if (month == 3 && day == 20) return true;

  // 昭和の日 (4月29日)
  if (month == 4 && day == 29) return true;

  // 憲法記念日 (5月3日)
  if (month == 5 && day == 3) return true;

  // みどりの日 (5月4日)
  if (month == 5 && day == 4) return true;

  // こどもの日 (5月5日)
  if (month == 5 && day == 5) return true;

  // 海の日 (7月の第3月曜日)
  if (month == 7 && weekday == 1 && day >= 15 && day <= 21) return true;

  // 山の日 (8月11日)
  if (month == 8 && day == 11) return true;

  // 敬老の日 (9月の第3月曜日)
  if (month == 9 && weekday == 1 && day >= 15 && day <= 21) return true;

  // 秋分の日 (仮: 9月23日)
  if (month == 9 && day == 23) return true;

  // 体育の日 (10月の第2月曜日、現在「スポーツの日」)
  if (month == 10 && weekday == 1 && day >= 8 && day <= 14) return true;

  // 文化の日 (11月3日)
  if (month == 11 && day == 3) return true;

  // 勤労感謝の日 (11月23日)
  if (month == 11 && day == 23) return true;

  // 天皇誕生日 (2月23日)
  if (month == 2 && day == 23) return true;

  // 祝日が日曜日の場合、翌月曜日を振替休日とする
  if (weekday == 0) { // 日曜日
    return (isHoliday(month, day - 1, 6)); // 前日が祝日か確認
  }

  return false; // 祝日ではない
}

String readSPIJson(String filename, JsonDocument *target, int timeout) {
  JsonDocument temp;
  Serial.println("readSPIJson Started");
  File file = SPIFFS.open(filename, FILE_READ);
  if (file) {
    /*
    //file.setTimeout(timeout);
    String fileStr = "";
    int size = file.size();
    int progress = 0;
    while (file.available()) {
      //if (progress % 1000 == 0) Serial.println(filename + " " + (((float) progress/(float) size)*100) + "% " + progress + "/" + size);
      uint8_t chr1 = file.peek();
      uint8_t chr2 = file.read();
      if (chr1 == chr2) {
        progress++;
        fileStr += (char) chr1;
        Serial.print((char) chr1);
      } else {
        file.seek(file.position()-1);
        Serial.print("[ERROR]");
      }
    }
    */
    DeserializationError error = deserializeJson(temp, file);
    file.close();
    if (error) {
      Serial.print(error.c_str());
      return "JSON Error";
    }
    *target = temp;
    Serial.println("readSPIJson Ended Success");
    return "";
  }
  file.close();
  return "Config file isn't found";
}

// Some Apps
void appEnd() {
  if (millis() > appStart+1000) {
    nowApp = "";
    delete appUI;
    haveToDeleteAppUI = false;
    M5.Speaker.end();
  }
}

void beep() {
  M5.Speaker.tone(440, 500);
}

// Settings
void settings_init() {
  appStart = millis();
  Serial.println("settings_init");
  M5.Speaker.begin();
  M5.Speaker.setVolume(127);
  appUI = new UI();
  appUI->setTitle("Settings");
  appUI->addItem((String) "beep", beep, (String) "Beep5");
  appUI->addItem((String) "beeper", beep, (String) "Beep1");
  appUI->addItem((String) "beepest", beep, (String) "Beep2");
  appUI->addItem((String) "beeeeep", beep, (String) "Beep3");
  appUI->addItem((String) "bepepepepe", beep, (String) "Beep4");
  appUI->linkFunctionToBack(appEnd);
  appUI->makeUI();
}

void settings_loop() {
  appUI->update(dateTime, battery);
}

// Train
String timetableName = "";
bool train_isMainUI = false;
int train_refleshTimer;
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
  appUI = new UI();
  haveToDeleteAppUI = true;
  appUI->setLocaleFont("en", 1);
  appUI->setTitle("Train");
  appUI->addItem("config", train_config, "Config");
  appUI->addItem("switch_week", train_switch_week);
  appUI->addItem("switch_mode", train_switch_mode);
  appUI->addLocaleToItem("switch_mode", "en", "Time");
  if (dateTime.date.weekDay == 0 || dateTime.date.weekDay == 6 || isHoliday(dateTime.date.month, dateTime.date.date, dateTime.date.weekDay)) {
    isWeekend = true;
    appUI->addLocaleToItem("switch_week", "en", "Weekend");
  } else {
    isWeekend = false;
    appUI->addLocaleToItem("switch_week", "en", "Weekday");
  }
  appUI->addItem("train1", nothing);
  appUI->addItem("train2", nothing);
  appUI->addItem("train3", nothing);
  appUI->linkFunctionToBack(appEnd);
  appUI->makeUI();
}

void train_switch_week() {
  if (isWeekend) {
    isWeekend = false;
    appUI->addLocaleToItem("switch_week", "en", "Weekday");
  } else {
    isWeekend = true;
    appUI->addLocaleToItem("switch_week", "en", "Weekend");
  }
  appUI->makeUI();
}

void train_switch_mode() {
  if (isRemainingMode) {
    isRemainingMode = false;
    appUI->addLocaleToItem("switch_mode", "en", "Time");
  } else {
    isRemainingMode = true;
    appUI->addLocaleToItem("switch_mode", "en", "Rem. Minutes");
  }
  appUI->makeUI();
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
  appUI = new UI();
  appUI->setLocaleFont("en", 1);
  haveToDeleteAppUI = true;
  appUI->setTitle("Set Timetable");
  for ( JsonPair loopTimetableName : train["timetable"].as<JsonObject>() ) {
    const char* nameBuffer = loopTimetableName.key().c_str();
    appUI->addItem(nameBuffer, train_setTimetable);
    Serial.println(nameBuffer);
  }
  appUI->linkFunctionToBack(train_init);
  appUI->makeUI();
}

void train_loop() {
  if (train_refleshTimer == 0 && train_isMainUI) {
    if (timetableName == "") {
      appUI->addLocaleToItem("train1", "en", "Please select");
      appUI->addLocaleToItem("train2", "en", "the Timetable");
      appUI->addLocaleToItem("train3", "en", "from Config");
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
        timetable = train["timetable"][timetableName]["weekends"];
      } else {
        timetable = train["timetable"][timetableName]["weekdays"];
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
      int loopHourAdd = 0;
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
        if (!train["color"][type[i]].isNull()) {
          for (uint8_t j = 0; j < 3; j++) {
            colors[i][j] = train["color"][type[i]][j];
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
        appUI->addRightLocaleToItem("train1", "en", String(dateTime.time.hours+hourAdd[0])+":"+force2digits(min[0]));
        appUI->addRightLocaleToItem("train2", "en", String(dateTime.time.hours+hourAdd[1])+":"+force2digits(min[1]));
        appUI->addRightLocaleToItem("train3", "en", String(dateTime.time.hours+hourAdd[2])+":"+force2digits(min[2]));
      }
      appUI->makeUI();
    }
  }
  train_refleshTimer++;
  if (train_refleshTimer >= 10) {
    train_refleshTimer = 0;
  }
  appUI->update(dateTime, battery);
}

// Debug Settings
void settings_init_another() {
  appStart = millis();
  Serial.println("settings_init");
  M5.Speaker.begin();
  M5.Speaker.setVolume(127);
  appUI = new UI();
  appUI->setTitle("Settings");
  appUI->addItem((String) "beep", beep, (String) "Only Beep");
  appUI->linkFunctionToBack(appEnd);
  appUI->makeUI();
}

void makeClockBase() {
  cv_ckbase.createSprite(220, 220);
  cv_ckbase.fillCircle(ckCenterX, ckCenterY, 110, WHITE);
  cv_ckbase.fillCircle(ckCenterX, ckCenterY, 105, BLACK);
  float deg;
  for (int i = 0; i < 12; i++) {
    deg = i*0.523;
    if (i == 6) {
      cv_ckbase.setCursor((sin(deg)*93)+ckCenterX-14, (cos(deg)*93)+ckCenterY-13);
      cv_ckbase.setFont(&fonts::Font4);
      cv_ckbase.setTextColor(WHITE, BLACK);
      cv_ckbase.setTextSize(1);
      cv_ckbase.print("12");
    } else if (i % 3 == 0) {
      cv_ckbase.setCursor((sin(deg)*93)+ckCenterX-7, (cos(deg)*93)+ckCenterY-13);
      cv_ckbase.setFont(&fonts::Font4);
      cv_ckbase.setTextColor(WHITE, BLACK);
      cv_ckbase.setTextSize(1);
      cv_ckbase.print(12-((i+18)%12));
    } else {
      thickLine(cv_ckbase, (sin(deg)*97)+ckCenterX, (cos(deg)*97)+ckCenterY, (sin(deg)*106)+ckCenterX, (cos(deg)*106)+ckCenterY, WHITE);
    }
  }
  cv_ckhhand.createSprite(7, 70);
  cv_ckmhand.createSprite(5, 105);
  cv_ckhhand.fillScreen(SKYBLUE);
  cv_ckmhand.fillScreen(SKYBLUE);
}

void updateDateTimeBat() {
  dateTime = M5.Rtc.getDateTime();
  battery = M5.Power.getBatteryLevel();
}

void updateClock() {
  cv_ckbase.pushSprite(&cv_clock, 0, 0);
  if (M5.Power.Axp2101.isVBUS()) {
    cv_clock.fillArc(ckCenterX, ckCenterY, 110, 108, 270, (battery*3.6)-90.5, CYAN);
  } else {
    cv_clock.fillArc(ckCenterX, ckCenterY, 110, 108, 270, (battery*3.6)-90.5, batcolor(battery));
  }
  cv_ckhhand.setPivot(2, 2);
  cv_ckhhand.pushRotated(&cv_clock, (180+((dateTime.time.hours*30)+(dateTime.time.minutes/2)))%360);
  cv_ckmhand.setPivot(1, 1);
  cv_ckmhand.pushRotated(&cv_clock, (180+((dateTime.time.minutes*6)+(dateTime.time.seconds/10)))%360);
  cv_clock.fillCircle(ckCenterX, ckCenterY, 8, SKYBLUE);
  cv_clock.fillCircle(ckCenterX, ckCenterY, 5, BLACK);
}

void updateDigitals() {
  cv_dtime_bat.clear();
  cv_dtime_bat.setTextColor(WHITE, BLACK);
  cv_dtime_bat.drawString(force2digits(dateTime.time.hours)+":"+force2digits(dateTime.time.minutes)+" "+force2digits(dateTime.time.seconds), 0, 0, &fonts::Font2);
  if (M5.Power.Axp2101.isVBUS()) { cv_dtime_bat.setTextColor(CYAN, BLACK); }
  cv_dtime_bat.drawRightString(String(battery)+"%", sizeX, 0, &fonts::Font2);
  cv_day.clear();
  cv_day.setTextColor(WHITE, BLACK);
  cv_day.drawString(String(dateTime.date.month)+"/"+String(dateTime.date.date)+" "+week[dateTime.date.weekDay]+" "+dateTime.date.year, 0, 0, &fonts::Font2);
  int ramFree = (int) (heap_caps_get_free_size(MALLOC_CAP_8BIT));
  int ramTotal = (int) (heap_caps_get_total_size(MALLOC_CAP_8BIT));
  cv_day.drawRightString((String) ((ramTotal-ramFree)/1000)+"KB/"+(String) (ramTotal/1000)+"KB", sizeX, 0, &fonts::Font2);
}

/*
String readSDJson(String filename) {
  if (SD.begin(4, SPI)){
    File file = SD.open(filename, FILE_READ);
    if (file) {
      String jsoncfg;
      while (file.available()) {
        jsoncfg = file.readString();
      }
      file.close();
      int len = jsoncfg.length() + 1;
      char jsonca[len];
      jsoncfg.toCharArray(jsonca, len);
      DeserializationError error = deserializeJson(json, jsonca);
      SD.end();
      if (error) {
        return "JSON Error";
      }
      return "";
    }
    SD.end();
    return "Config file isn't found";
  }
  return "SD isn't found or broken";
}
*/

bool conectWiFi(JsonDocument conf) {
  if (!conf.isNull()) {
    for (JsonPair kv : conf.as<JsonObject>()) {
      String pass = kv.value();
      int len = pass.length() + 1;
      char passca[len];
      pass.toCharArray(passca, len);
      WiFi.begin(kv.key().c_str(), passca);
      int timer = 0;
      while (WiFi.status() != WL_CONNECTED and timer < 50){
          delay(100);
          timer++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        return true;
      }
    }
  }
  return false;
}

bool connectWiFi() {
  WiFi.begin();
  int timer = 0;
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
    http.addHeader("Content-Type", "application/json");
    int responseCode = http.GET();
    body = http.getString();
    http.end();
  } else {
    body = "";
  }
  return body;
}

void setRTCFromUNIX(time_t unixTime) {
  struct tm *timeinfo;
  timeinfo = localtime(&unixTime);
  m5::rtc_time_t TimeStruct;
  m5::rtc_date_t DateStruct;
  TimeStruct.hours = timeinfo->tm_hour;
  TimeStruct.minutes = timeinfo->tm_min;
  TimeStruct.seconds = timeinfo->tm_sec;
  M5.Rtc.setTime(TimeStruct);
  DateStruct.weekDay = timeinfo->tm_wday;
  DateStruct.month = timeinfo->tm_mon + 1;
  DateStruct.date = timeinfo->tm_mday;
  DateStruct.year = timeinfo->tm_year + 1900;
  M5.Rtc.setDate(DateStruct);
}

bool copySDtoSPI() {
  if (SD.begin(4, SPI)) {
    File folder = SD.open("/spiffs");
    if (!folder) {
      SD.end();
      //Serial.println("Error1");
      return false;
    } else if (!folder.isDirectory()) {
      SD.end();
      //Serial.println("Error2");
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
      //String CopyFile = SDFile.readString();
      //Serial.println(CopyFile);
      //SPIFile.print(CopyFile);
      int size = SDCardFile.size();
      int progress = 0;
      while (SDCardFile.available()) {
        if (progress % 1000 == 0) Serial.println(SPIPath + " " + (((float) progress/(float) size)*100) + "% " + progress + "/" + size);
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
    //Serial.println("Error3");
    return false;
  }
}

void setupConfigs() {
  SPIFFS.begin(true);
  M5Model = getModel();
  M5.Display.println("MK75-Watch");
  M5.Display.print("Model: ");
  if (M5Model == "M5StackCore2") {
    M5.Display.setTextColor(GREEN, BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Supported Model");
  } else {
    M5.Display.setTextColor(RED, BLACK);
    M5.Display.println(M5Model);
    M5.Display.println("Unsupported Model");
  }
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.print("copy sd to spiffs...");
  if (copySDtoSPI()) {
    M5.Display.println("success");
  } else {
    M5.Display.println("failed");
  }
  M5.Display.print("settings read...");
  String SPIResult = readSPIJson("/wifi.json", &wifi, 1000);
  if (SPIResult == "") {
    M5.Display.println("success");
    if (WiFi.status() != WL_CONNECTED) {
      M5.Display.print("wifi from json connect...");
      if (conectWiFi(wifi)) {
        M5.Display.println("success");
      } else {
        M5.Display.println("failed");
      }
    }
  } else {
    M5.Display.println(SPIResult);
  }
  if (WiFi.status() != WL_CONNECTED) {
    M5.Display.print("wifi from saved connect...");
    if (connectWiFi()) {
      M5.Display.println("success");
    } else {
      M5.Display.println("failed");
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.print("change time...");
    int tmrStart = millis();
    String timeResp = connectHTTP("http://worldtimeapi.org/api/timezone/Japan");
    if (timeResp != "") {
      JsonDocument timeRespJSON;
      int len = timeResp.length() + 1;
      char timeRespCA[len];
      timeResp.toCharArray(timeRespCA, len);
      DeserializationError error = deserializeJson(timeRespJSON, timeRespCA);
      if (error) {
        M5.Display.println("failed");
      } else {
        int time = timeRespJSON["unixtime"];
        setRTCFromUNIX(time+JST+ceil((millis()-tmrStart)/2000));
        M5.Display.println("success");
      }
    } else {
      M5.Display.println("failed");
    }
  } else {
    M5.Display.println("can't set time");
  }
  WiFi.disconnect(true);
  M5.Display.print("Loading train json...");
  readSPIJson("/train.json", &train, 100000);
  M5.Display.println("finish");
  delay(1000);
  M5.Display.clear();
}

void setupSprites() {
  cv_display.createSprite(sizeX, sizeY);
  cv_clock.createSprite(220, 220);
  cv_menu.createSprite(320, 220);
  makeClockBase();
  cv_dtime_bat.createSprite(sizeX, 17);
  cv_day.createSprite(sizeX, 17);
}

void setupMenu() {
  cv_menu.setFont(&lgfxJapanMincho_40);
  for (int ix = -1; ix < 2; ix++) {
    for (int iy = 0; iy < 2; iy++) {
      cv_menu.fillRect(((ix*105)-50)+(sizeX/2), ((iy*85)-88)+(sizeY/2), 100, 80, WHITE);
      cv_menu.fillRect(((ix*105)-47)+(sizeX/2), ((iy*85)-85)+(sizeY/2), 94, 74, BLACK);
      cv_menu.pushImage(((ix*105)-16)+(sizeX/2), ((iy*85)-80)+(sizeY/2), 32, 32, icons[(ix+1)+(iy*3)]);
      cv_menu.setTextColor(WHITE, BLACK);
      if (lang == "ja") {
        cv_menu.drawCenterString(appsJa[(ix+1)+(iy*3)], (ix*105)+(sizeX/2), ((iy*85)-45)+(sizeY/2), &fonts::efontJA_16);
      } else if (lang == "en") {
        cv_menu.drawCenterString(appsEn[(ix+1)+(iy*3)], (ix*105)+(sizeX/2), ((iy*85)-45)+(sizeY/2), &fonts::Font2);
      } else {
        cv_menu.drawCenterString("ERROR. LOL", (ix*105)+(sizeX/2), ((iy*85)-45)+(sizeY/2), &fonts::Font2);
      }
    }
  }
}

bool checkGyro() {
  /*
  M5.Imu.getAccel(&mpu[0], &mpu[1], &mpu[2]);
  return ((mpu[0] > 0.95) and (abs(mpu[1]) < 0.75) and (abs(mpu[2]) < 0.75));
  */
  M5.Imu.getGyro(&mpu[3], &mpu[4], &mpu[5]);
  return ((mpu[3] > 200));
}

void loopTouch() {
  int touch = M5.Touch.getCount();
  if (touch) {
    auto detail = M5.Touch.getDetail();
    if (prevTX != -1) {
      screenSwipe += prevTX-detail.x;
      for (int i = 0; i < 5; i++) {
      prevSwipeAcc[i] = prevSwipeAcc[i+1];
    }
      prevSwipeAcc[5] = prevTX-detail.x;
    }
    prevTX = detail.x;
    prevTY = detail.y;
    if (wasTouched) {
      firstTX = detail.x;
      firstTY = detail.y;
    }
  } else {
    if (wasTouched and screenSwipe <= 340 and screenSwipe >= 300) {
      for (int lax = -1; lax < 2; lax++) {
        for (int lay = 0; lay < 2; lay++) {
          int appx = ((lax*105)-50)+(sizeX/2);
          int appy = ((lay*85)-40)+(sizeY/2);
          if ((appx <= prevTX) and (appx+100 >= prevTX) and (appy <= prevTY) and (appy+80 >= prevTY) and (appx <= firstTX) and (appx+100 >= firstTX) and (appy <= firstTY) and (appy+80 >= firstTY)) {
            Serial.print(lax); Serial.print(" "); Serial.println(lay);
            vibTimer = 100;
            if (lax == 0 and lay == 0) {
              nowApp = "settings";
              settings_init();
            } else if (lax == 1 and lay == 1) {
              nowApp = "train";
              train_init();
            } else {
              nowApp = "settings";
              settings_init_another();
            }
          }
        }
      }
    }
    if (screenSwipe < 0) {
      screenSwipe += (0-screenSwipe)/2;
      screenSwipe = round(screenSwipe);
      if (abs(screenSwipe) <= 1) screenSwipe = 0;
    } else if (screenSwipe > 320) {
      screenSwipe += (320-screenSwipe)/2;
      screenSwipe = round(screenSwipe);
      if (abs(screenSwipe-320) <= 1) screenSwipe = 320;
    } else if (screenSwipe % 320 != 0) {
      int calib = limit(prevSwipeAcc[0]*10, -160, 160);
      int target = limit(round(((float) (screenSwipe+calib)/320))*320, 0, 320);
      screenSwipe += (target-screenSwipe)/2;
      if (abs(screenSwipe-target) <= 1) screenSwipe = target;
    }
    prevTX = -1;
    prevTY = -1;
  }
  wasTouched = (touch > 0);
}

void loopSleep() {
  bool touch = M5.Touch.getCount() > 0;
  if (!((checkGyro() and (!M5.Power.Axp2101.isVBUS())) or (touch) or (vibTimer > 0))) {
    if (nowApp == "") {
      slpTimer += prevLoopTime*3;
    } else {
      slpTimer += prevLoopTime;
    }
    if (slpTimer >= 15000) {
      int touch = 0;
      int light = false;
      bool charged = M5.Power.Axp2101.isVBUS();
      M5.Display.clear();
      M5.Display.setBrightness(0);
      M5.Display.sleep();
      while (!((checkGyro() and (!M5.Power.Axp2101.isVBUS())) or (touch > 0) or (M5.BtnPWR.isPressed()) or (charged != M5.Power.Axp2101.isVBUS()))) {
        M5.Power.lightSleep(100000);
        delayMicroseconds(1);
        M5.update();
        touch = M5.Touch.getCount();
      }
      if (M5.Power.Axp2101.isVBUS() and !charged) {
        vibTimer = 500;
      }
      afterSlp = true;
      M5.Display.wakeup();
      M5.Display.setBrightness(63);
      slpTimer = 0;
    }
  } else {
    slpTimer = 0;
  }
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);
  M5.Power.begin();
  M5.Imu.init();
  Serial.begin(115200);
  M5.Display.init();
  M5.Display.setBrightness(63);
  setupConfigs();
  setupSprites();
  setupMenu();
  wasVBUS = M5.Power.Axp2101.isVBUS();
  
}

void loop() {
  int tmrStart = millis();
  M5.update();
  cv_display.clear();
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
    nowApp = "";
  }
  if (M5.BtnC.wasPressed()) {
    if (M5.Display.getBrightness() )
    M5.Display.setBrightness((M5.Display.getBrightness()+32)%256);
  }
  if (nowApp == "settings") {
    settings_loop();
  } else if (nowApp == "train") {
    train_loop();
  } else {
    loopTouch();
    if ((screenSwipe % 320 != 0) or (screenSwipe == 320)) cv_menu.pushSprite(&cv_display, 320-screenSwipe, centerY-ckCenterY, BLACK);
    if ((screenSwipe % 320 != 0) or (screenSwipe == 0)) {
      updateClock();
      cv_clock.pushSprite(&cv_display, centerX-ckCenterX-screenSwipe, centerY-ckCenterY, BLACK);
    }
    if (M5.BtnA.wasPressed()) {
      screenSwipe = 0;
    }
    updateDigitals();
    M5.update();
    loopTouch();
    cv_day.pushSprite(&cv_display, 1, sizeY-17, BLACK);
    cv_dtime_bat.pushSprite(&cv_display, 0, 0, BLACK);
    cv_display.pushSprite(0, 0);
  }
  if (afterSlp) {
    afterSlp = false;
  } else {
    prevLoopTime = millis()-tmrStart;
  }
}
