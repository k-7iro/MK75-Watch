#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <Preferences.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"
#include "assets/htmls.hpp"
#include "main.hpp"

WiFiServer server(80);
const IPAddress ip(192, 168, 10, 75);
const IPAddress subnet(255, 255, 255, 0);

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