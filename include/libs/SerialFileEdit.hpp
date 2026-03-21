/*
  [ SerialFileEdit ] by K-Nana
  Access LittleFS, SPIFFS or SD card contents via Serial.
  MIT License https://opensource.org/license/mit
*/

#pragma once
#include <M5Unified.h>
#include <LittleFS.h>
#include "libs/NanaTools.hpp"

typedef enum {
  SFE_IDLE = 0,
  SFE_CONNECTED = 1,
  SFE_EDITING = 2
} sfe_state_t;

class SerialFileEdit {
  public:
    SerialFileEdit(Stream *serial, FS *fs) : SFE_Serial(serial), SFE_System(fs) {}
    void begin(uint32_t baud = 0);
    void end(bool endSerial = true);
    void update();
  private:
    Stream *SFE_Serial;
    FS *SFE_System;
    File editFile;
    sfe_state_t state = SFE_IDLE;
    String buffer;
    std::list<String> parsed;
};

template <typename T>
void streamBegin(T &stream, uint32_t baud) {
  stream.begin(baud);
}

template <typename T>
void streamEnd(T &stream) {
  stream.end();
}

void SerialFileEdit::begin(uint32_t baud) {
  if (baud != 0) streamBegin(SFE_Serial, baud);
  state = SFE_CONNECTED;
}

void SerialFileEdit::end(bool endSerial) {
  if (endSerial) streamEnd(SFE_Serial);
  if (editFile) editFile.close();
  state = SFE_IDLE;
}

void SerialFileEdit::update() {
  if (state == SFE_CONNECTED) {
    while (SFE_Serial->available()) {
      char c = SFE_Serial->read();
      if (c == 0x18) {
        buffer = "";
      } else if (c == 0x0a) {
        parsed = split(buffer, ' ');
        SFE_Serial->println(buffer); // DEBUG
        decltype(parsed)::iterator itr = parsed.begin();
        if (parsed.size() >= 2) {
          String cmd = *itr;
          SFE_Serial->println(cmd); // DEBUG
          itr++;
          String file = *itr;
          SFE_Serial->println(file); // DEBUG
          if (cmd == "ex") {
            if (SFE_System->exists(file)) {
              SFE_Serial->print(1);
            } else {
              SFE_Serial->print(0);
            }
          } else if (cmd == "md") {
            if (SFE_System->mkdir(file)) {
              SFE_Serial->print(1);
            } else {
              SFE_Serial->print(0);
            }
          } else if (cmd == "rm") {
            if (SFE_System->remove(file)) {
              SFE_Serial->print(1);
            } else {
              SFE_Serial->print(0);
            }
          } else if (cmd == "rd") {
            if (SFE_System->rmdir(file)) {
              SFE_Serial->print(1);
            } else {
              SFE_Serial->print(0);
            }
          } else if (cmd == "ls") {
            editFile = SFE_System->open(file);
            if (!editFile) {
              SFE_Serial->print(0);
            } else if (!editFile.isDirectory()) {
              SFE_Serial->print(0);
              editFile.close();
            } else {
              SFE_Serial->println(1);
              File loopFile = editFile.openNextFile();
              while (loopFile) {
                Serial.print(loopFile.name());
                if (loopFile.isDirectory()) Serial.print('/');
                Serial.println();
                loopFile = editFile.openNextFile();
              }
              editFile.close();
            }
          } else if (cmd == "re") {
            editFile = SFE_System->open(file);
            if (!editFile) {
              SFE_Serial->print(0);
            } else if (editFile.isDirectory()) {
              SFE_Serial->print(0);
              editFile.close();
            } else {
              SFE_Serial->println(1);
              while (editFile.available()) {
                Serial.write(editFile.read());
              }
              editFile.close();
            }
          } else if (cmd == "wr") {
            editFile = SFE_System->open(file, FILE_WRITE);
            state = SFE_EDITING;
          } else {
            SFE_Serial->print(0);
          }
        } else {
          SFE_Serial->print(0);
        }
        buffer = "";
      } else if (c != 0x0d) {
        buffer += c;
      }
    }
  } else if (state == SFE_EDITING) {
    while (SFE_Serial->available()) {
      char c = SFE_Serial->read();
      if (c == 0x18) {
        buffer.clear();
        state = SFE_CONNECTED;
        editFile.close();
      } else {
        editFile.write(c);
      }
    }
  }
}