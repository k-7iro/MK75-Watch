#include <M5Unified.h>
#include <M5GFX.h>

float mpu[3] = {0, 0, 0};
float prevmpu[3] = {0, 0, 0};

static M5Canvas cv_display(&M5.Display);

void setup() {
    auto config = M5.config();
    config.serial_baudrate = 115200;
    M5.begin(config);
    cv_display.createSprite(320, 240);
}

void loop() {
    M5.update();
    M5.Imu.getAccel(&mpu[0], &mpu[1], &mpu[2]);
    float vector = sqrt((mpu[0]*mpu[0])+(mpu[1]*mpu[1])+(mpu[2]*mpu[2]));
    cv_display.clear();
    cv_display.setFont(&fonts::Font4);
    cv_display.setCursor(0, 0);
    cv_display.print(vector);
    cv_display.pushSprite(0, 0);
    delay(100);
}