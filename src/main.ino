#include <M5Unified.h>
#include <M5GFX.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

static M5Canvas cv_display(&M5.Display);
static M5Canvas cv_clock(&cv_display);
static M5Canvas cv_menu(&cv_display);
static M5Canvas cv_ckbase(&cv_display);
static M5Canvas cv_dtime_bat(&cv_clock);
static M5Canvas cv_day(&cv_clock);
static M5Canvas cv_ckhhand(&cv_clock);
static M5Canvas cv_ckmhand(&cv_clock);

JsonDocument config;

const int sizeX = 320;
const int centerX = 160;
const int sizeY = 240;
const int centerY = 120;
const int ckCenterX = 110;
const int ckCenterY = 110;
const int JST = 32401;
const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
String M5Model;

int slpTimer = 0;
int vibTimer = 0;
int screenSwipe = 0;
int prevTX = 0;
int prevTY = 0;
int cycle = 0;
int prevLoopTime = 0;
int prevSwipeAcc_1 = 0;
int prevSwipeAcc_2 = 0;
int prevSwipeAcc_3 = 0;

bool wasVBUS = false;

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

String force2digits(int num) {
  if (num < 10) {
    return "0"+String(num);
  }
  return String(num);
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

void updateClock() {
  uint8_t bat = M5.Power.getBatteryLevel();
  auto dt = M5.Rtc.getDateTime();
  cv_ckbase.pushSprite(&cv_clock, 0, 0);
  if (M5.Power.Axp2101.isVBUS()) {
    cv_clock.fillArc(ckCenterX, ckCenterY, 110, 108, 270, (bat*3.6)-90.5, CYAN);
  } else {
    cv_clock.fillArc(ckCenterX, ckCenterY, 110, 108, 270, (bat*3.6)-90.5, batcolor(bat));
  }
  cv_ckhhand.setPivot(2, 2);
  cv_ckhhand.pushRotated(&cv_clock, (180+((dt.time.hours*30)+(dt.time.minutes/2)))%360);
  cv_ckmhand.setPivot(1, 1);
  cv_ckmhand.pushRotated(&cv_clock, (180+((dt.time.minutes*6)+(dt.time.seconds/10)))%360);
}

void updateDigitals() {
  uint8_t bat = M5.Power.getBatteryLevel();
  auto dt = M5.Rtc.getDateTime();
  cv_dtime_bat.clear();
  cv_dtime_bat.setTextColor(WHITE, BLACK);
  cv_dtime_bat.drawString(force2digits(dt.time.hours)+":"+force2digits(dt.time.minutes)+" "+force2digits(dt.time.seconds), 1, 0, &fonts::Font2);
  if (M5.Power.Axp2101.isVBUS()) { cv_dtime_bat.setTextColor(CYAN, BLACK); }
  cv_dtime_bat.drawRightString(String(bat)+"%", sizeX, 0, &fonts::Font2);
  cv_day.clear();
  cv_day.setTextColor(WHITE, BLACK);
  cv_day.drawString(String(dt.date.month)+"/"+String(dt.date.date)+" "+week[dt.date.weekDay]+" "+dt.date.year, 1, 1, &fonts::Font2);
  int ramFree = (int) (heap_caps_get_free_size(MALLOC_CAP_8BIT));
  int ramTotal = (int) (heap_caps_get_total_size(MALLOC_CAP_8BIT));
  cv_day.drawRightString((String) ((ramTotal-ramFree)/1000)+"KB/"+(String) (ramTotal/1000)+"KB", sizeX, 1, &fonts::Font2);
}

String readSDJson(String filename) {
  if (SD.begin(4, SPI)){
    File file = SD.open(filename, FILE_READ);
    if (file) {
      String jsoncfg;
      while (file.available()) {
        jsoncfg = file.readString();
      }
      file.close();
      Serial.println(jsoncfg);
      int len = jsoncfg.length() + 1;
      char jsonca[len];
      jsoncfg.toCharArray(jsonca, len);
      DeserializationError error = deserializeJson(config, jsonca);
      if (error) {
        return "JSON Error";
      }
      return "";
    }
    return "Config file isn't found";
  }
  return "SD isn't found or broken";
}

bool conectWiFi(JsonDocument conf) {
  if (conf["wifi"]["ssid"]) {
    for (JsonPair kv : conf["wifi"]["ssid"].as<JsonObject>()) {
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

void setupConfigs() {
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
  M5.Display.print("wifi from saved connect...");
  if (connectWiFi()) {
    M5.Display.println("success");
  } else {
    M5.Display.println("failed");
  }
  M5.Display.println(SD.cardType());
  M5.Display.print("settings read...");
  String SDResult = readSDJson("/settings.json");
  if (SDResult == "") {
    M5.Display.println("success");
    if (WiFi.status() != WL_CONNECTED) {
      M5.Display.print("wifi from json connect...");
      if (conectWiFi(config)) {
        M5.Display.println("success");
      } else {
        M5.Display.println("failed");
      }
    }
  } else {
    M5.Display.println(SDResult);
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
      cv_menu.fillRect(((ix*105)-50)+(sizeX/2), ((iy*85)-85)+(sizeY/2), 100, 80, WHITE);
      cv_menu.fillRect(((ix*105)-47)+(sizeX/2), ((iy*85)-82)+(sizeY/2), 94, 74, BLACK);
    }
  }
}

bool checkGyro() {
  float ax = 0;
  float ay = 0;
  float az = 0;
  M5.Imu.getAccel(&ax, &ay, &az);
  return ((az > 0.95) and (abs(ay) < 0.75) and (abs(ax) < 0.75));
}

void loopTouch(int touch) {
  if (touch) {
    Serial.print("Hold: "); Serial.print(prevSwipeAcc_1); Serial.print(" "); Serial.print(prevSwipeAcc_2); Serial.print(" "); Serial.println(prevSwipeAcc_3);
    auto detail = M5.Touch.getDetail();
    if (prevTX != -1) {
      screenSwipe += prevTX-detail.x;
      prevSwipeAcc_3 = prevSwipeAcc_2;
      prevSwipeAcc_2 = prevSwipeAcc_1;
      prevSwipeAcc_1 = prevTX-detail.x;
    }
    prevTX = detail.x;
    prevTY = detail.y;
  } else {
    if (screenSwipe < 0) {
      screenSwipe += (0-screenSwipe)/2;
      screenSwipe = round(screenSwipe);
      if (abs(screenSwipe) <= 1) screenSwipe = 0;
    } else if (screenSwipe > 320) {
      screenSwipe += (320-screenSwipe)/2;
      screenSwipe = round(screenSwipe);
      if (abs(screenSwipe-320) <= 1) screenSwipe = 320;
    } else if (screenSwipe % 320 != 0) {
      int calib = limit(prevSwipeAcc_3*3, -160, 160);
      int target = limit(round(((float) (screenSwipe+calib)/320))*320, 0, 320);
      screenSwipe += (target-screenSwipe)/2;
      if (abs(screenSwipe-target) <= 1) screenSwipe = target;
    }
    prevTX = -1;
    prevTY = -1;
  }
}

void loopSleep(bool touch) {
  if (!((checkGyro() and (!M5.Power.Axp2101.isVBUS())) or (touch) or (vibTimer > 0))) {
    slpTimer += prevLoopTime;
    if (slpTimer >= 5000) {
      int touch = 0;
      int light = false;
      bool charged = M5.Power.Axp2101.isVBUS();
      M5.Display.clear();
      M5.Display.setBrightness(0);
      M5.Display.sleep();
      while (!((checkGyro() and (!M5.Power.Axp2101.isVBUS())) or (touch > 0) or (charged != M5.Power.Axp2101.isVBUS()))) {
        M5.Power.lightSleep(2500000);
        delayMicroseconds(1);
        M5.update();
        touch = M5.Touch.getCount();
        light = abs(255-light);
        M5.Power.setLed(light);
      }
      if (M5.Power.Axp2101.isVBUS() and !charged) {
        vibTimer = 500;
      }
      M5.Display.wakeup();
      M5.Display.setBrightness(64);
      M5.Power.setLed(255);
      slpTimer = 0;
    }
  } else {
    slpTimer = 0;
  }
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

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);
  M5.Power.begin();
  M5.Imu.init();
  Serial.begin(115200);
  M5.Display.init();
  M5.Display.setBrightness(64);
  setupConfigs();
  setupSprites();
  setupMenu();
  M5.Power.setLed(255);
  wasVBUS = M5.Power.Axp2101.isVBUS();
}

void loop() {
  int tmrStart = millis();
  M5.update();
  cv_display.clear();
  bool touch = (M5.Touch.getCount() > 0);
  loopTouch(touch);
  loopSleep(touch);
  if (vibTimer > 0) {
    M5.Power.setVibration(127);
    vibTimer -= prevLoopTime;
  } else {
    M5.Power.setVibration(0);
  }
  if ((!wasVBUS) and M5.Power.Axp2101.isVBUS()) {
    vibTimer = 500;
  }
  wasVBUS = M5.Power.Axp2101.isVBUS();
  updateDigitals();
  updateClock();
  cv_clock.pushSprite(&cv_display, centerX-ckCenterX-screenSwipe, centerY-ckCenterY, BLACK);
  cv_menu.pushSprite(&cv_display, 320-screenSwipe, centerY-ckCenterY, BLACK);
  cv_day.pushSprite(&cv_display, 1, sizeY-17, BLACK);
  cv_dtime_bat.pushSprite(&cv_display, 1, 0, BLACK);
  cv_display.pushSprite(0, 0);
  prevLoopTime = millis()-tmrStart;
}
