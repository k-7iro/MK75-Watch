#include <M5Unified.h>
#include <M5GFX.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

static M5GFX lcd;
static M5Canvas cv_display(&lcd);
static M5Canvas cv_clock(&cv_display);
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

int slpTimer = 0;
int vibTimer = 0;

bool wasVBUS = false;

void thickLine(M5Canvas target, int x0, int y0, int x1, int y1, int color) {
  target.drawLine(x0, y0, x1, y1, color);
  target.drawLine(x0+1, y0, x1+1, y1, color);
  target.drawLine(x0-1, y0, x1-1, y1, color);
  target.drawLine(x0, y0+1, x1, y1+1, color);
  target.drawLine(x0, y0-1, x1, y1-1, color);
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
  return lcd.color888((u_int8_t) r, (u_int8_t) g, 0);
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
  cv_clock.pushSprite(&cv_display, centerX-ckCenterX, centerY-ckCenterY, BLACK);
}

void updateDigitals() {
  uint8_t bat = M5.Power.getBatteryLevel();
  auto dt = M5.Rtc.getDateTime();
  cv_dtime_bat.clear();
  cv_dtime_bat.setTextColor(WHITE, BLACK);
  cv_dtime_bat.drawString(force2digits(dt.time.hours)+":"+force2digits(dt.time.minutes), 1, 1, &fonts::Font4);
  cv_dtime_bat.drawString(force2digits(dt.time.seconds), 1, 27, &fonts::Font4);
  if (M5.Power.Axp2101.isVBUS()) { cv_dtime_bat.setTextColor(CYAN, BLACK); }
  cv_dtime_bat.drawRightString(String(bat)+"%", sizeX, 1, &fonts::Font4);
  cv_dtime_bat.pushSprite(&cv_display, 1, 1, BLACK);
  cv_day.clear();
  cv_day.setTextColor(WHITE, BLACK);
  cv_day.drawString(String(dt.date.month)+"/"+String(dt.date.date), 1, 1, &fonts::Font4);
  cv_day.drawString(week[dt.date.weekDay]+" '"+force2digits(dt.date.year%100), 1, 27, &fonts::Font4);
  cv_day.pushSprite(&cv_display, 1, sizeY-55, BLACK);
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
  lcd.print("wifi from saved connect...");
  if (connectWiFi()) {
    lcd.println("success");
  } else {
    lcd.println("failed");
  }
  lcd.println(SD.cardType());
  lcd.print("settings read...");
  String SDResult = readSDJson("/settings.json");
  if (SDResult == "") {
    lcd.println("success");
    if (WiFi.status() != WL_CONNECTED) {
      lcd.print("wifi from json connect...");
      if (conectWiFi(config)) {
        lcd.println("success");
      } else {
        lcd.println("failed");
      }
    }
  } else {
    lcd.println(SDResult);
  }
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("change time...");
    int timerStart = millis();
    int timerEnd;
    String timeResp = connectHTTP("http://worldtimeapi.org/api/timezone/Japan");
    if (timeResp != "") {
      JsonDocument timeRespJSON;
      int len = timeResp.length() + 1;
      char timeRespCA[len];
      timeResp.toCharArray(timeRespCA, len);
      DeserializationError error = deserializeJson(timeRespJSON, timeRespCA);
      if (error) {
        lcd.println("failed");
      } else {
        int time = timeRespJSON["unixtime"];
        timerEnd = millis();
        setRTCFromUNIX(time+JST+ceil((timerEnd-timerStart)/2000));
        lcd.println("success");
      }
    } else {
      lcd.println("failed");
    }
  } else {
    lcd.println("can't set time");
  }
  WiFi.disconnect(true);
  delay(1000);
  lcd.clear();
}

void setupSprites() {
  cv_display.createSprite(sizeX, sizeY);
  cv_clock.createSprite(220, 220);
  makeClockBase();
  cv_dtime_bat.createSprite(sizeX, 55);
  cv_day.createSprite(sizeX, 55);
}

bool checkGyro() {
  float ax = 0;
  float ay = 0;
  float az = 0;
  M5.Imu.getAccel(&ax, &ay, &az);
  return ((az > -0.75) and (ay > -0.75) and (ay < 0.75) and (ax > -0.75) and (ax < 0.75));
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);
  M5.Power.begin();
  M5.Imu.init();
  Serial.begin(115200);
  lcd.init();
  lcd.setBrightness(127);
  setupConfigs();
  setupSprites();
  M5.Power.setLed(255);
  wasVBUS = M5.Power.Axp2101.isVBUS();
}

void loop() {
  M5.update();
  cv_display.clear();
  if (!((checkGyro() and (!M5.Power.Axp2101.isVBUS())) or (M5.Touch.getCount() > 0) or (vibTimer > 0))) {
    if (slpTimer++ >= 50) {
      int touch = 0;
      int light = false;
      bool charged = M5.Power.Axp2101.isVBUS();
      lcd.clear();
      lcd.setBrightness(0);
      lcd.sleep();
      while (!((checkGyro() and (!M5.Power.Axp2101.isVBUS())) or (touch > 0) or (charged != M5.Power.Axp2101.isVBUS()))) {
        M5.Power.lightSleep(2500000);
        delayMicroseconds(1);
        M5.update();
        touch = M5.Touch.getCount();
        light = abs(255-light);
        M5.Power.setLed(light);
      }
      lcd.wakeup();
      lcd.setBrightness(127);
      M5.Power.setLed(255);
      slpTimer = 0;
    }
  } else {
    slpTimer = 0;
  }
  if (vibTimer > 0) {
    M5.Power.setVibration(127);
    vibTimer--;
  } else {
    M5.Power.setVibration(0);
  }
  if ((!wasVBUS) and M5.Power.Axp2101.isVBUS()) {
    vibTimer = 2;
  }
  wasVBUS = M5.Power.Axp2101.isVBUS();
  updateDigitals();
  updateClock();
  cv_display.pushSprite(0, 0);
  delay(100);                                                                                                      
}
