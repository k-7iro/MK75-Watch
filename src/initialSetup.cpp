#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "main.hpp"

void wifiInitialSetup(bool onSettings) {
  WiFiManager wm;
  bool notConnected = true;
  cv_display.clear();
  cv_display.fillRect(0, 17, sizeX, 64, TFT_WHITE);
  cv_display.setTextColor(TFT_BLACK, TFT_WHITE);
  cv_display.drawCenterString("Wi-Fi Setup", sizeX/2, 26, &fonts::Font4);
  if (onSettings) {
    uint16_t lightRed = M5.Display.color565(0xff, 0xaa, 0xaa);
    cv_display.fillRect(0, 17, 54, 64, lightRed);
    cv_display.setTextColor(BLACK, lightRed);
    cv_display.drawString("<", 20, 27, &fonts::Font4);
  }
  cv_display.setCursor(0, 65);
  cv_display.setFont(&fonts::Font2);
  cv_display.setTextColor(WHITE, TFT_BLACK);
  cv_display.println("1. Connect to Wi-Fi \"MK75-Setup\" using your smartphone or computer.");
  cv_display.println("2. Follow the on-screen instructions.");
  cv_display.fillRect(0, sizeY-120, 120, 120, WHITE);
  cv_display.qrcode("WIFI:S:MK75-Setup;T:nopass;;", 10, sizeY-110, 100, 2);
  if (onSettings) {
    updateDateTimeBat();
    if (showVoltage) {
      updateDigitalTimeBat(&cv_dtime_bat, dateTime, battery, M5.Power.getBatteryVoltage(), TFT_WHITE, TFT_BLACK);
    } else {
      updateDigitalTimeBat(&cv_dtime_bat, dateTime, battery, 65535, TFT_WHITE, TFT_BLACK);
    }
    cv_dtime_bat.pushSprite(0, 0);
  }
  cv_display.pushSprite(0, 0);
  wm.setConfigPortalBlocking(false);
  wm.autoConnect("MK75-Setup");
  while (WiFi.status() != WL_CONNECTED) {
    M5.update();
    wm.process();
    if (onSettings) {
      updateDateTimeBat();
      if (showVoltage) {
        updateDigitalTimeBat(&cv_dtime_bat, dateTime, battery, M5.Power.getBatteryVoltage(), TFT_WHITE, TFT_BLACK);
      } else {
        updateDigitalTimeBat(&cv_dtime_bat, dateTime, battery, 65535, TFT_WHITE, TFT_BLACK);
      }
      cv_dtime_bat.pushSprite(0, 0);
      cv_display.pushSprite(0, 0);
      if (M5.Touch.getCount() > 0) {
        m5::touch_detail_t tDetail = M5.Touch.getDetail();
        if (tDetail.wasClicked() && inLimit(tDetail.x, 0, 54) && inLimit(tDetail.y, 17, 64)) {
          wm.stopConfigPortal();
          WiFi.disconnect(true);
          break;
        }
      }
      if (M5.BtnA.wasClicked()) {
        wm.stopConfigPortal();
        WiFi.disconnect(true);
        break;
      }
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    cv_display.fillRect(0, 65, sizeX, sizeY, BLACK);
    cv_display.setCursor(0, 65);
    cv_display.setFont(&fonts::Font2);
    cv_display.setTextColor(WHITE, TFT_BLACK);
    cv_display.println("Connection Succeed. Saving Wi-Fi Settings...");
    cv_display.pushSprite(0, 0);
    if (!onSettings) wifiJson.clear();
    wifiJson[wm.getWiFiSSID()] = wm.getWiFiPass();
    if (!onSettings) {
      writeSPIJson("/wifi.json", &wifiJson);
      cv_display.println("Finished. Syncing time...");
      cv_display.pushSprite(0, 0);
      syncTime();
    }
    cv_display.println("Finished.");
    cv_display.pushSprite(0, 0);
    WiFi.disconnect(true, true);
    delay(1000);
    if (!onSettings) {
      M5.Display.clear();
      M5.Display.setCursor(0, 0);
      M5.Display.setFont(&fonts::Font0);
      M5.Display.setTextColor(WHITE, TFT_BLACK);
    }
  }
}