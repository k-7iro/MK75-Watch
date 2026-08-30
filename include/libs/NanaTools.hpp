/*
  [ NanaTools ] by K-Nana
  MIT License https://opensource.org/license/mit
*/

#pragma once
#include <M5Unified.h>
#include <list>

inline void nothing() {}

inline bool inLimit(int num, int min, int max) {
  if (num < min) return false;
  else if (num > max) return false;
  return true;
}

inline String forceDigits(int num, int digits) {
  String result = String(num);
  uint8_t numdigits = result.length();
  if (digits > numdigits) {
    uint8_t loops = digits - numdigits;
    for (uint8_t i = 0; i < loops; i++) {
      result = "0"+result;
    }
  }
  return result;
}

inline std::list<String> split(String sentence, char denim) {
  std::list<String> result;
  while (true) {
    int find = sentence.indexOf(denim);
    if (find == -1) {
      if (sentence != "") result.push_back(sentence);
      return result;
    } else {
      if (find != 0) result.push_back(sentence.substring(0, find));
      sentence = sentence.substring(find+1, sentence.length());
    }
  }
}

inline bool isStringDigit(String target) {
  if (target.charAt(0) == '-') target = target.substring(1, target.length());
  uint8_t dotCount = 0;
  uint16_t cnt = 0;
  for (auto i = target.begin(); i != target.end(); i++) {
    if (*i == '.') {
      if ((cnt == 0) or (cnt == target.length()-1)) return false;
      dotCount++;
      if (dotCount > 1) return false;
    } else if (!isdigit(*i)) return false;
    cnt++;
  }
  return (!target.isEmpty());
}

inline String successOrFail(bool target) {
  if (target) return "Success";
  return "Failed";
}

// endにはmillis()やmicros()を入れるといいでしょう。
inline uint32_t calculateElapsedTime(uint32_t start, uint32_t end) {
  if (end >= start) return end - start;
  return (UINT32_MAX - start) + end + 1;
}

inline uint8_t combineHex(uint8_t lower, uint8_t upper) {
  return (upper << 4) | lower;
}

inline void decombineHex(uint8_t hexValue, uint8_t *lower, uint8_t *upper) {
  *lower = hexValue & 0x0F;
  *upper = (hexValue >> 4) & 0x0F;
}

inline uint16_t simpleHueToRgb(int h) { // Powered by Google AI Mode
    // 360度の範囲に丸める
    h = h % 360;
    if (h < 0) h += 360;

    int zone = h / 60;      // 0〜5 の 6つのゾーンに分ける
    int delta = (h % 60) * 255 / 60; // 60度の中での「増減する量」(0〜255)

    switch (zone) {
        case 0: return M5.Display.color565(255, delta, 0); // 1. 緑が増える
        case 1: return M5.Display.color565(255 - delta, 255, 0); // 2. 赤が減る
        case 2: return M5.Display.color565(0, 255, delta); // 3. 青が増える
        case 3: return M5.Display.color565(0, 255 - delta, 255); // 4. 緑が減る
        case 4: return M5.Display.color565(delta, 0, 255); // 5. 赤が増える
        default: return M5.Display.color565(255, 0, 255 - delta); // 6. 青が減る
    }
}

// 指定月の最大日数を取得（うるう年対応） / Get maximum day in month (leap year aware)
inline uint8_t getMonthMaxDay(uint8_t month, bool leapYear) {
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
inline uint8_t dateToWeekday(uint16_t y, uint8_t m, uint8_t d) {
  if (m < 3) {
    y--;
    m += 12;
  }
  return (y + y/4 - y/100 + y/400 + (13*m + 8)/5 + d) % 7;
}

// 日本の祝日判定 / Check if a given date is a Japanese holiday (powered by ChatGPT)
// 3連休ルール対応：繰り替え休日と昭和の日のフローティング等を実装
inline bool isHoliday(int32_t month, int32_t day, int32_t year, int32_t weekday) {
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
inline bool isLeapYear(uint8_t year) {
  if (year % 400 == 0) return true;
  else if (year % 100 == 0) return false;
  else if (year % 4 == 0) return true;
  return false;
}




// 太い線を描画 / Draw a thicker line by overlaying multiple shifted lines
inline void thickLine(M5Canvas *target, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t color) {
  target->drawLine(x0, y0, x1, y1, color);
  target->drawLine(x0+1, y0, x1+1, y1, color);
  target->drawLine(x0-1, y0, x1-1, y1, color);
  target->drawLine(x0, y0+1, x1, y1+1, color);
  target->drawLine(x0, y0-1, x1, y1-1, color);
}

// 論理値を指定文字列に変換 / Convert boolean to specified string
inline String boolStr(bool target, String ifTrue, String ifFalse) {
  if (target) {
    return ifTrue;
  } else {
    return ifFalse;
  }
}

// バッテリー残量に応じた色を計算 / Calculate color based on battery level (green->yellow->red)
inline uint32_t batcolor(int32_t bat) {
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
inline String getModel(m5::board_t modelType) {
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