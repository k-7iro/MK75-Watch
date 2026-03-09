#include <M5Unified.h>
#include <M5GFX.h>

float mpu[3] = {0, 0, 0};
float prevmpu[3] = {0, 0, 0};

static M5Canvas cv_display(&M5.Display);

bool checkAccel_sum() {
  M5.Imu.getAccel(&mpu[0], &mpu[1], &mpu[2]);
  float change = 0;
  for (uint8_t i = 0; i < 3; i++) {
    change += abs(mpu[i]-prevmpu[i]);
    prevmpu[i] = mpu[i];
  }
  return (change < 0.05);
}

void setup() {
    auto config = M5.config();
    config.serial_baudrate = 115200;
    M5.begin(config);
    cv_display.createSprite(320, 240);
}

void loop() {
    M5.update();
    M5.Imu.getAccel(&mpu[0], &mpu[1], &mpu[2]);
    float change = 0;
    for (uint8_t i = 0; i < 3; i++) {
        change += abs(mpu[i]-prevmpu[i]);
        prevmpu[i] = mpu[i];
    }
    cv_display.clear();
    cv_display.setFont(&fonts::Font4);
    cv_display.setCursor(0, 0);
    cv_display.println(change);
    if (checkAccel_sum()) {
        cv_display.println("okf");
    }
    if (change < 0.05) {
        cv_display.println("ok");
    }
    cv_display.pushSprite(0, 0);
    delay(1000);
}