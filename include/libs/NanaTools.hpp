/*
  [ NanaTools ] by K-Nana
  MIT License https://opensource.org/license/mit
*/

#pragma once
#include <M5Unified.h>
#include <list>

void nothing() {}

int limit(int num, int min, int max) {
  if (num < min) return min;
  else if (num > max) return max;
  return num;
}

bool inLimit(int num, int min, int max) {
  if (num < min) return false;
  else if (num > max) return false;
  return true;
}

String forceDigits(int num, int digits) {
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

std::list<String> split(String sentence, char denim) {
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

bool isStringDigit(String target) {
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

String successOrFail(bool target) {
  if (target) return "Success";
  return "Failed";
}

uint32_t calculateElapsedTime(uint32_t start, uint32_t end) {
  if (end >= start) return end - start;
  return (UINT32_MAX - start) + end + 1;
}

uint8_t combineHex(uint8_t lower, uint8_t upper) {
  return (upper << 4) | lower;
}

void decombineHex(uint8_t hexValue, uint8_t *lower, uint8_t *upper) {
  *lower = hexValue & 0x0F;
  *upper = (hexValue >> 4) & 0x0F;
}