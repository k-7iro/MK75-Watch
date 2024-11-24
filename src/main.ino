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

const int sizeX = 320;
const int centerX = 160;
const int sizeY = 240;
const int centerY = 120;
const int ckCenterX = 110;
const int ckCenterY = 110;
const int JST = 32401;
const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};

int count = 0;
int sd_stats = 0;

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
  if (M5.Power.isCharging()) {
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
  if (M5.Power.isCharging()) { cv_dtime_bat.setTextColor(GREEN, BLACK); }
  cv_dtime_bat.drawRightString(String(bat)+"%", sizeX, 1, &fonts::Font4);
  cv_dtime_bat.pushSprite(&cv_display, 1, 1, BLACK);
  cv_day.clear();
  cv_day.setTextColor(WHITE, BLACK);
  cv_day.drawString(String(dt.date.month)+"/"+String(dt.date.date), 1, 1, &fonts::Font4);
  cv_day.drawString(week[dt.date.weekDay]+" '"+force2digits(dt.date.year%100), 1, 27, &fonts::Font4);
  cv_day.pushSprite(&cv_display, 1, sizeY-55, BLACK);
}

JsonDocument readSDJson(String filename) {
  if (SD.begin(4)){
    File file = SD.open(filename);
    if (file) {
      String jsoncfg;
      while (file.available()) {
        jsoncfg = file.readString();
      }
      file.close();
      JsonDocument doc;
      int len = jsoncfg.length() + 1;
      char jsonca[len];
      jsoncfg.toCharArray(jsonca, len);
      DeserializationError error = deserializeJson(doc, jsonca);
      if (!error) {
        return doc;
      }
    }
  }
}

bool conectWiFi(JsonDocument config) {
  if (config["wifi"]["ssid"]) {
    for (JsonPair kv : config["wifi"]["ssid"].as<JsonObject>()) {
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
    Serial.println("HTTPClient begin!");
    http.addHeader("Content-Type", "application/json");
    int responseCode = http.GET();
    body = http.getString();
    Serial.println(responseCode);
    Serial.println(body);
    http.end();
  } else {
    Serial.println("Failed HTTPClient begin!");
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
  /*
  lcd.print("settings read...");
  JsonDocument json = readSDJson("/settings.json");
  if (!json.isNull()) {
    lcd.println("success");
    if (WiFi.status() != WL_CONNECTED) {
      lcd.print("wifi from json connect...");
      if (conectWiFi(json)) {
        lcd.println("success");
      } else {
        lcd.println("failed");
      }
    }
  } else {
    lcd.println("failed");
  }
  */
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
  return true;
}

void setup() {
  auto cfg = M5.config();
  cfg.internal_imu = true;
  M5.begin(cfg);
  M5.Power.begin();
  if (esp_sleep_get_wakeup_cause() == 0) {
    Serial.begin(115200);
    lcd.init();
    lcd.setBrightness(128);
    setupConfigs();
    setupSprites();
  } else {
    if (checkGyro()) {
      Serial.begin(115200);
      lcd.init();
      lcd.setBrightness(128);
      setupSprites();
    } else {
      M5.Power.deepSleep(1000);
    }
  }
}

void loop() {
  M5.update();
  cv_display.clear();
  updateDigitals();
  updateClock();
  cv_display.pushSprite(0, 0);
  if (count > 100) {count = 0;}
  delay(100);                                                                                                         
}