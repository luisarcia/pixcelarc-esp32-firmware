#ifndef RENDER_H
#define RENDER_H

#include <Arduino.h>

bool renderAnimation(int index, uint8_t repeats, uint32_t delay_ms);
void animateLoop(void *param);
void startAnimation();

#endif
