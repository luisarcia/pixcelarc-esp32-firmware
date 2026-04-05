#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoMatrix.h>
#include <LittleFS.h>

// Matrix LED Config
#define MATRIX_WIDTH 16
#define MATRIX_HEIGHT 16
#define MATRIX_PIN 32
#define MATRIX_BRIGHTNESS_DEFAULT 40

// Animation Config
#define MAX_ANIMS 15
constexpr uint8_t FPS = 12; // frames per second
constexpr uint32_t FRAME_DELAY = 1000 / FPS;
constexpr size_t FRAME_SIZE = MATRIX_WIDTH * MATRIX_HEIGHT * 3;

// struct
struct Animation {
  char id[8];          // ID único de la animación (max 7 chars + null)
  size_t frameCount;
  size_t byteLength;
  
  // Constructor por defecto
  Animation() : frameCount(0), byteLength(0) {
    id[0] = '\0';
  }
};

// Variables globales
extern const char *ssid;
extern const char *password;

extern bool displayOn;
extern uint8_t brightness;
extern uint8_t repeatsPerAnim;

extern bool newAnimPending;
extern Animation playlist[MAX_ANIMS];
extern int playlistSize;

extern TaskHandle_t animTaskHandle;
extern SemaphoreHandle_t animMutex;
extern Adafruit_NeoMatrix ledMatrix;

extern AsyncWebServer webServer;
extern uint8_t *uploadBuffer;
extern size_t uploadReceived;
extern int uploadSlot;
extern char uploadID[8];

// Buffer global para renderizado (evita stack overflow)
extern uint8_t renderFrameBuffer[MATRIX_WIDTH * MATRIX_HEIGHT * 3];

#endif