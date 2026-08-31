#include <M5Unified.h>
#include <libs/NanaTools.hpp>
#include <WiFi.h>
#include <LittleFS.h>
#include "main.hpp"

String buffer;
std::list<String> parsed;

String getOption(std::list<String> command, String option) {
  bool isOptionReached = false;
  String result = "";
  for (auto itr = command.begin(); itr != command.end(); ++itr) {
    String current = *itr;
    if (current == "-"+option) {
      isOptionReached = true;
    } else if (current.indexOf("-") == 0) {
      isOptionReached = false;
    } else if (isOptionReached) {
      if (result != "") result += " ";
      result += current;
    }
  }
  return result;
}

bool checkOption(std::list<String> command, String option) {
  for (auto itr = command.begin(); itr != command.end(); ++itr) {
    String current = *itr;
    if (current == "-"+option) {
      return true;
    }
  }
  return false;
}

void wifiConsole_loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 0x0a) {
      parsed = split(buffer, ' ');
      auto itr = parsed.begin();
      if (itr != parsed.end()) {
        if (*itr == "wifi") {
          itr++;
          if (itr != parsed.end()) {
            String cmd = *itr;
            itr++;
            if (cmd == "add") {
              String ssid = getOption(parsed, "ssid");
              String pass = getOption(parsed, "pass");
              bool skipTestWifi = checkOption(parsed, "skiptest");
              if (ssid == "") {
                Serial.println("Error: SSID must be provided.");
              } else {
                if (!skipTestWifi) {
                  Serial.println("Testing connection to SSID \"" + ssid + "\"...");
                  if (WiFi.status() == WL_CONNECTED) {
                    WiFi.disconnect(false);
                  }
                  WiFi.begin(ssid, pass);
                  int32_t timer = 0;
                  while (WiFi.status() != WL_CONNECTED and timer < 100){
                    delay(100);
                    timer++;
                  }
                }
                if (WiFi.status() == WL_CONNECTED) {
                  WiFi.disconnect(true);
                  Serial.print("The test was successful, and ");
                } if (skipTestWifi || WiFi.status() == WL_CONNECTED) {
                  wifiJson[ssid] = pass;
                  writeSPIJson("/wifi.json", &wifiJson);
                  Serial.println("SSID \"" + ssid + "\" has been added.");
                } else {
                  Serial.println("It appears that the SSID \"" + ssid + "\" is not within range, or the password is incorrect.");
                }
              }
            } else if (cmd == "list") {
              Serial.println("Configured networks:");
              for (const auto& kv : wifiJson.as<JsonObject>()) {
                String ssid = kv.key().c_str();
                Serial.println(ssid);
              }
            } else if (cmd == "remove") {
              String ssid = getOption(parsed, "ssid");
              if (!wifiJson[ssid].isNull()) {
                wifiJson.remove(ssid);
                writeSPIJson("/wifi.json", &wifiJson);
                Serial.println("Removed network: " + ssid);
              } else {
                Serial.println("Network not found: " + ssid);
              }
            } else if (cmd == "clear") {
              wifiJson.clear();
              LittleFS.remove("/wifi.json");
              Serial.println("Cleared all configured networks.");
            } else {
              Serial.println("Unknown command: " + cmd);
            }
          }
        } else {
          Serial.println("Unknown command: " + buffer);
        }
      }
      buffer = "";
    } else {
      buffer += c;
    }
  }
}