#include "animations.h"
#include "config.h"
#include "matrix.h"
#include <WiFi.h>

void scrollText(String text, uint16_t color) {
  int textLen = text.length() * 6;
  int endX    = -textLen;
  ledMatrix.setTextWrap(false);
  ledMatrix.setTextSize(1);
  ledMatrix.setTextColor(color);
  for (int x = MATRIX_WIDTH; x >= endX; x--) {
    ledMatrix.fillScreen(0);
    ledMatrix.setCursor(x, 4);
    ledMatrix.print(text);
    ledMatrix.show();
    delay(40);
  }
}

void wifiConnectingAnimation() {
  uint16_t orange = ledMatrix.Color(255, 100, 0);
  uint8_t  pos    = 0;
  while (WiFi.status() != WL_CONNECTED) {
    ledMatrix.fillScreen(0);
    for (int x = 1; x <= 14; x++) {
      ledMatrix.drawPixel(x, 6, ledMatrix.Color(40, 40, 40));
      ledMatrix.drawPixel(x, 9, ledMatrix.Color(40, 40, 40));
    }
    ledMatrix.drawPixel(0,  7, ledMatrix.Color(40, 40, 40));
    ledMatrix.drawPixel(0,  8, ledMatrix.Color(40, 40, 40));
    ledMatrix.drawPixel(15, 7, ledMatrix.Color(40, 40, 40));
    ledMatrix.drawPixel(15, 8, ledMatrix.Color(40, 40, 40));
    for (int x = 1; x <= pos; x++) {
      ledMatrix.drawPixel(x, 7, orange);
      ledMatrix.drawPixel(x, 8, orange);
    }
    ledMatrix.show();
    delay(120);
    pos++;
    if (pos > 14) pos = 0;
  }
}

void wifiConnectedAnimation() {
  uint16_t green = ledMatrix.Color(0, 220, 80);
  ledMatrix.fillScreen(0);
  for (int x = 1; x <= 14; x++) {
    ledMatrix.drawPixel(x, 6, ledMatrix.Color(20, 60, 20));
    ledMatrix.drawPixel(x, 9, ledMatrix.Color(20, 60, 20));
    ledMatrix.drawPixel(x, 7, green);
    ledMatrix.drawPixel(x, 8, green);
  }
  ledMatrix.drawPixel(0,  7, ledMatrix.Color(20, 60, 20));
  ledMatrix.drawPixel(0,  8, ledMatrix.Color(20, 60, 20));
  ledMatrix.drawPixel(15, 7, ledMatrix.Color(20, 60, 20));
  ledMatrix.drawPixel(15, 8, ledMatrix.Color(20, 60, 20));
  ledMatrix.show();
  delay(2000);

  String   ip   = WiFi.localIP().toString();
  uint16_t cyan = ledMatrix.Color(0, 200, 220);
  unsigned long start = millis();
  while (millis() - start < 5000) {
    scrollText(ip, cyan);
  }
  ledMatrix.fillScreen(0);
  ledMatrix.show();
}
