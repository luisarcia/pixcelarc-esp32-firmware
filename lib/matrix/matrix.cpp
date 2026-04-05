#include "matrix.h"
#include "config.h"

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// Define la variable global
Adafruit_NeoMatrix ledMatrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH, MATRIX_HEIGHT, 1, 1, MATRIX_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

void matrixBegin() {
  ledMatrix.begin();
  ledMatrix.setBrightness(MATRIX_BRIGHTNESS_DEFAULT);
  ledMatrix.fillScreen(0);
  ledMatrix.show();
}

void matrixOff() {
  ledMatrix.fillScreen(0);
  ledMatrix.show();
}
