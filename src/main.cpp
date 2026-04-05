#include <Arduino.h>
#include "config.h"
#include "credentials.h"
#include "matrix.h"
#include "animations.h"
#include "storage.h"
#include "render.h"
#include "web.h"
#include <ESPmDNS.h>

// ── Definición de variables globales ──────────────────────────────────────

bool               displayOn       = true;
uint8_t            brightness      = MATRIX_BRIGHTNESS_DEFAULT;
uint8_t            repeatsPerAnim  = 2;
bool               newAnimPending  = false;
Animation          playlist[MAX_ANIMS];
int                playlistSize    = 0;
TaskHandle_t       animTaskHandle  = NULL;
SemaphoreHandle_t  animMutex;

uint8_t           *uploadBuffer    = nullptr;
size_t             uploadReceived  = 0;
int                uploadSlot      = -1;
char               uploadID[8]     = "";

// Buffer global para renderizado
uint8_t            renderFrameBuffer[MATRIX_WIDTH * MATRIX_HEIGHT * 3];

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  matrixBegin();

  if (!LittleFS.begin(true)) {
    Serial.println("❌ LittleFS falló");
  } else {
    Serial.println("✅ LittleFS montado");
    
    // Crear directorio con validación (Quick win #12)
    if (!LittleFS.mkdir("/anim")) {
      // El directorio ya existe o falló, verificar si existe
      File dir = LittleFS.open("/anim");
      if (!dir || !dir.isDirectory()) {
        Serial.println("⚠️ No se pudo crear /anim");
      }
      if (dir) dir.close();
    }
    
    loadMetaFromFS();  // Ahora carga tanto config como playlist
  }

  ledMatrix.setBrightness(brightness);  // aplicar brillo cargado desde flash

  // Configurar mDNS (#19)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifiConnectingAnimation();
  Serial.println("\nWiFi conectado — IP: " + WiFi.localIP().toString());
  
  // Iniciar mDNS
  if (MDNS.begin("pixcelarc")) {
    Serial.println("✅ mDNS iniciado: http://pixcelarc.local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("⚠️ Error iniciando mDNS");
  }
  
  wifiConnectedAnimation();

  animMutex = xSemaphoreCreateMutex();
  startAnimation();

  if (!displayOn) {
    vTaskSuspend(animTaskHandle);
    matrixOff();
  }

  setupRoutes();
  webServer.begin();
}

void loop() {
  // En ESP32, mDNS funciona automáticamente sin update()
  delay(10);
}
