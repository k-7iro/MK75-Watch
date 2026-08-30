#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "libs/NanaUI.hpp"
#include "libs/NanaTools.hpp"
#include "libs/NanaDrawPlus.hpp"

// [共通]

// アプリケーションタイプ / Application type enumeration
typedef enum {
  APP_NOTHING = 0,
  APP_ALARM = 1,
  APP_TIMER = 2,
  APP_STOPWATCH = 3,
  APP_TRAIN = 4,
  APP_EXT_DEVICE = 5,
  APP_RANDOM = 6,
  APP_SETTINGS = 7
} apptype_t;

// UI追加機能タイプ / Additional UI type
typedef enum {
  UI_NOTHING = 0,
  UI_TIME = 1,
  UI_RANDOM = 2
} uiaddtional_t;

// 時刻UI表示モード / Time display mode for UI
typedef enum {
  TIMEUI_MODE_HOURMIN = 0,
  TIMEUI_MODE_MINSEC = 1,
  TIMEUI_MODE_DATE = 2
} uiaddsettings_t;

// 外部デバイス電源制御設定 / External device power control settings
typedef enum {
  EXT_POWER_ALWAYS = 0,
  EXT_POWER_ACTIVE = 1,
  EXT_POWER_NEVER = 2
} extsettings_t;

// 時計盤デザインタイプ / Dial type for clock display
typedef enum {
  DIAL_ZERODIAL = 0,
  DIAL_CLASSIC = 1
} dialtype_t;

// 基本セット
extern UI appUI;
extern void appEnd();
extern m5::rtc_datetime_t dateTime;
extern uint8_t battery;
extern uint16_t getBatteryVoltage();
extern const int32_t sizeX;
extern const int32_t centerX;
extern const int32_t sizeY;
extern const int32_t centerY;
extern char lang[3];
extern bool writeSPIJson(String filename, JsonDocument *target);
extern int32_t prevLoopTime;

// UIAdditional用
extern uiaddtional_t UIAddtional;
extern uiaddsettings_t UIAddtionalSettings;
extern M5Canvas cv_uiadditional;

// UIAdditional-Time用
extern int8_t UItimeLeft;
extern int8_t UItimeRight;
extern void drawTimeUI();

// [タイマー用]
extern std::list<long> timers;

// [設定用]

// 1. 関数宣言 (Function Prototypes)
extern void setRTCAlarmIRQ();
extern void makeClockBase();
extern String getVersionString(uint32_t ver);
extern String connectWiFi(JsonDocument conf);
extern void notice(String title, String time);
extern void syncTime();

// 2. 定数 (Constants)
extern const uint32_t version;
extern m5::board_t modelType;

// 3. 文字列・JSON ドキュメント (Strings & JSON)
extern JsonDocument wifiJson;

// 4. 配列変数 (Arrays)
extern uint8_t sleepTimings[4];
extern uint32_t sleepTimings_sys[4];
extern bool doSleep[2];
extern extsettings_t extSettings[2];

// 5. 基本設定値 (uint8_t, dialtype_t)
extern uint8_t screenBrightness;
extern uint8_t speakerVolume;
extern uint8_t chargeCurrent;
extern uint8_t chargeVoltage;
extern dialtype_t dialType;
extern uint16_t lastSync;

// 6. ブール値 (bools)
extern bool vibration;
extern bool showVoltage;

// [交通時刻表用]
extern JsonDocument trainJson;

// [アラーム用]
extern JsonDocument alarmJson;

// [ストップウォッチ用]
extern M5Canvas cv_display;
extern M5Canvas cv_stwt1;
extern M5Canvas cv_stwt2;
extern M5Canvas cv_stwt3;
extern M5Canvas cv_stwt4;
extern M5Canvas cv_stwt5;
extern M5Canvas cv_stwt_top;
extern M5Canvas cv_dtime_bat;
extern void updateDateTimeBat();
extern void scrollsWhenTouch(m5::touch_detail_t detail, int32_t* target, bool vertical = false);
extern void scrollsWhenNotTouch(int32_t* target, int32_t indexes, int32_t distant, bool useAcc = true, float speed = 2, uint8_t maxAccMulti = 10);

// [初期設定用]
extern JsonDocument wifiJson;
extern bool wifiJsonAvailable;
