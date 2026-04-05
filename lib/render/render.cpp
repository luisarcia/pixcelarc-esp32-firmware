#include "render.h"
#include "config.h"
#include "storage.h"
#include <LittleFS.h>

bool renderAnimation(int index, uint8_t repeats, uint32_t delay_ms)
{
  String path = playlistFilename(playlist[index].id);
  
  // Abrir archivo una sola vez (optimización #6)
  File f = LittleFS.open(path, "r");
  if (!f)
    return true;

  for (uint8_t r = 0; r < repeats; r++)
  {
    // Volver al inicio del archivo para cada repeat
    if (r > 0)
      f.seek(0);

    while (f.available() >= (int)FRAME_SIZE)
    {
      if (newAnimPending)
      {
        newAnimPending = false;
        f.close();
        return false;
      }

      // Usar buffer global en lugar de stack (optimización #7)
      size_t bytesRead = f.read(renderFrameBuffer, FRAME_SIZE);
      if (bytesRead != FRAME_SIZE)
        break;

      for (int y = 0; y < MATRIX_HEIGHT; y++)
      {
        for (int x = 0; x < MATRIX_WIDTH; x++)
        {
          int idx = (y * MATRIX_WIDTH + x) * 3;
          ledMatrix.drawPixel(x, y, ledMatrix.Color(renderFrameBuffer[idx], renderFrameBuffer[idx + 1], renderFrameBuffer[idx + 2]));
        }
      }
      ledMatrix.show();
      vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
  }
  
  f.close();
  return true;
}

void animateLoop(void *)
{
  while (true)
  {
    // Timeout de 1 segundo en semáforo (optimización #8)
    if (xSemaphoreTake(animMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
      Serial.println("⚠️ Timeout animMutex");
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }
    
    uint32_t delay_ms = FRAME_DELAY;
    int count = playlistSize;
    uint8_t repeats = repeatsPerAnim;
    xSemaphoreGive(animMutex);

    if (count == 0)
    {
      ledMatrix.fillScreen(0);
      ledMatrix.show();
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    for (int i = 0; i < count; i++)
    {
      bool completed = renderAnimation(i, repeats, delay_ms);
      if (!completed)
        break;
    }
  }
}

void startAnimation()
{
  if (animTaskHandle == NULL)
  {
    xTaskCreatePinnedToCore(animateLoop, "AnimTask", 4096, NULL, 1, &animTaskHandle, 1);
  }
}
