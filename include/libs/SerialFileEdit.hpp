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
    SerialFileEdit(Stream *stream, FS *fs) : SFE_Stream(stream), SFE_System(fs) {};
    void update();
  protected:
    Stream *SFE_Stream;
    FS *SFE_System;
    File editFile;
    sfe_state_t state = SFE_IDLE;
    String buffer;
    std::list<String> parsed;
};

void SerialFileEdit::update() {
  if (state == SFE_CONNECTED) {
    while (SFE_Stream->available()) {
      char c = SFE_Stream->read();
      if (c == 0x18) {
        buffer = "";
      } else if (c == 0x0a) {
        parsed = split(buffer, ' ');
        SFE_Stream->println(buffer); // DEBUG
        decltype(parsed)::iterator itr = parsed.begin();
        if (parsed.size() >= 2) {
          String cmd = *itr;
          // SFE_Stream->println(cmd); // DEBUG
          itr++;
          String file = *itr;
          // SFE_Stream->println(file); // DEBUG
          if (cmd == "ex") {
            if (SFE_System->exists(file)) {
              SFE_Stream->print(1);
            } else {
              SFE_Stream->print(0);
            }
          } else if (cmd == "md") {
            if (SFE_System->mkdir(file)) {
              SFE_Stream->print(1);
            } else {
              SFE_Stream->print(0);
            }
          } else if (cmd == "rm") {
            if (SFE_System->remove(file)) {
              SFE_Stream->print(1);
            } else {
              SFE_Stream->print(0);
            }
          } else if (cmd == "rd") {
            if (SFE_System->rmdir(file)) {
              SFE_Stream->print(1);
            } else {
              SFE_Stream->print(0);
            }
          } else if (cmd == "ls") {
            editFile = SFE_System->open(file);
            if (!editFile) {
              SFE_Stream->print(0);
            } else if (!editFile.isDirectory()) {
              SFE_Stream->print(0);
              editFile.close();
            } else {
              SFE_Stream->println(1);
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
              SFE_Stream->print(0);
            } else if (editFile.isDirectory()) {
              SFE_Stream->print(0);
              editFile.close();
            } else {
              SFE_Stream->println(1);
              while (editFile.available()) {
                Serial.write(editFile.read());
              }
              editFile.close();
            }
          } else if (cmd == "wr") {
            editFile = SFE_System->open(file, FILE_WRITE);
            state = SFE_EDITING;
          } else {
            SFE_Stream->print(0);
          }
        } else {
          SFE_Stream->print(0);
        }
        buffer = "";
      } else if (c != 0x0d) {
        buffer += c;
      }
    }
  } else if (state == SFE_EDITING) {
    while (SFE_Stream->available()) {
      char c = SFE_Stream->read();
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

class SFE_HWS : public SerialFileEdit {
  public:
    SFE_HWS(HardwareSerial *serial, FS *fs) : SerialFileEdit::SerialFileEdit(serial, fs), SFE_HWSerial(serial) {};
    void begin(uint32_t baud = 0);
    void end(bool endSerial = true);
  private:
    HardwareSerial *SFE_HWSerial;
};

void SFE_HWS::begin(uint32_t baud) {
  if (baud != 0) SFE_HWSerial->begin(baud);
  state = SFE_CONNECTED;
}

void SFE_HWS::end(bool endSerial) {
  if (endSerial) SFE_HWSerial->end();
  if (editFile) editFile.close();
  state = SFE_IDLE;
}

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
class SFE_USB : public SerialFileEdit {
  public:
    SFE_USB(HWCDC *serial, FS *fs) : SerialFileEdit::SerialFileEdit(serial, fs), SFE_HWCDC(serial) {};
    void begin(uint32_t baud = 0);
    void end(bool endSerial = true);
  private:
    HWCDC *SFE_HWCDC;
};

void SFE_USB::begin(uint32_t baud) {
  if (baud != 0) SFE_HWCDC->begin(baud);
  state = SFE_CONNECTED;
}

void SFE_USB::end(bool endSerial) {
  if (endSerial) SFE_HWCDC->end();
  if (editFile) editFile.close();
  state = SFE_IDLE;
}
#endif