#include <M5Unified.h>
#include <M5GFX.h>

void setup() {
    auto config = M5.config();
    config.serial_baudrate = 115200;
    M5.begin(config);
}

void loop() {
    M5.update();
}