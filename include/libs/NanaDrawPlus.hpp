/*
  [ NanaDrawPlus ] by K-Nana
   Additional M5Stack drawing libraries.
  Say goodbye to Jaggy.
  MIT License https://opensource.org/license/mit
*/

#pragma once
#include <M5Unified.h>
#include <M5GFX.h>

/* I may or may not use it in the future.
int32_t mixColor888(int32_t color1, int32_t color2, float ratio) {
    if (ratio <= 0) return color2;
    if (ratio >= 1) return color1;
    uint8_t red = ((color1 >> 24)*(ratio))+((color2 >> 24)*(1-ratio));
    uint8_t green = (((color1 >> 16) & 0xff)*(ratio))+(((color2 >> 16) & 0xff)*(1-ratio));
    uint8_t blue = (((color1 >> 8) & 0xff)*(ratio))+(((color2 >> 8) & 0xff)*(1-ratio));
    uint8_t alpha = ((color1 & 0xff)*(ratio))+((color2 & 0xff)*(1-ratio));
    return (red << 24)+(green << 16)+(blue << 8)+alpha;
}
*/

uint16_t mixColor(uint16_t color1, uint16_t color2, float ratio) {
    if (ratio <= 0) return color1;
    if (ratio >= 1) return color2;
    uint8_t red = ((color1 >> 11)*(1-ratio))+((color2 >> 11)*(ratio));
    uint8_t green = (((color1 >> 5) & 0x3f)*(1-ratio))+(((color2 >> 5) & 0x3f)*(ratio));
    uint8_t blue = ((color1 & 0x1f)*(1-ratio))+((color2 & 0x1f)*(ratio));
    return (red << 11)+(green << 5)+blue;
}

void drawCircleWithAA(LovyanGFX *target, int32_t x, int32_t y, int32_t radius, int16_t color, int16_t outColor) {
    int32_t startX = max(0, x-radius);
    int32_t startY = max(0, y-radius);
    int32_t endX = min(target->width(), x+radius);
    int32_t endY = min(target->height(), y+radius);
    target->startWrite();
    float prevBorder = radius;
    for (int32_t ix = 0; ix <= radius; ix++) {
        float border = sqrt(pow(radius, 2)-pow(ix, 2));
        int32_t borderInt = floor(border);
        int32_t borderIntCeil = borderInt+1;
        for (int32_t iy = 0; iy <= radius; iy++) {
            if (borderIntCeil == iy || (prevBorder > iy && iy > border)) {
                int16_t mixedColor;
                if (iy >= radius*(0.70710678118)) { // 1/√2
                    float borderDeci = border-borderInt;
                    mixedColor = mixColor(outColor, color, borderDeci);
                } else {
                    float border2 = sqrt(pow(radius, 2)-pow(iy, 2));
                    int32_t border2Int = floor(border2);
                    float border2Deci = border2-border2Int;
                    mixedColor = mixColor(outColor, color, border2Deci);
                }
                target->drawPixel(x-ix, y-iy, mixedColor);
                target->drawPixel(x+ix, y-iy, mixedColor);
                target->drawPixel(x-ix, y+iy, mixedColor);
                target->drawPixel(x+ix, y+iy, mixedColor);
            } else if (borderIntCeil > iy) {
                target->drawPixel(x-ix, y-iy, color);
                target->drawPixel(x+ix, y-iy, color);
                target->drawPixel(x-ix, y+iy, color);
                target->drawPixel(x+ix, y+iy, color);
            }
        }
        prevBorder = border;
    }
    target->endWrite();
}
