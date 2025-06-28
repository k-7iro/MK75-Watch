/*
  [ NanaTools ] by K-Nana
*/

#pragma once
#include <M5Unified.h>

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