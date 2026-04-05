#ifndef MATRIX_H
#define MATRIX_H

#include <Arduino.h>
#include <Adafruit_NeoMatrix.h>

// Declaración externa de la variable global matrix
extern Adafruit_NeoMatrix matrix;

void matrixBegin();
void matrixOff();

#endif
