#include "web.h"
#include "config.h"
#include "storage.h"
#include "apiResponse.h"
#include <LittleFS.h>

// Definición de la variable global del servidor web
AsyncWebServer webServer(80);

void setupRoutes()
{
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  webServer.onNotFound([](AsyncWebServerRequest *request)
                       {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404);
    } });

  // Power control
  webServer.on("/device/power", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
        JsonDocument body;
        JsonDocument payload;
        DeserializationError error = deserializeJson(body, data, len);

        if (error) {
            payload["success"] = false;
            payload["message"] = "Invalid JSON";
            sendRequestJson(request, payload, 400);
            return;
        }

        if (!body["state"].is<const char*>()) {
            payload["success"] = false;
            payload["message"] = "Missing or invalid 'state' field";
            sendRequestJson(request, payload, 400);
            return;
        }

        const char* state = body["state"];

        if (strcmp(state, "on") == 0) {
            if (animTaskHandle) vTaskResume(animTaskHandle);
            displayOn = true;
            saveMetaToFS();

            payload["success"] = true;
            payload["message"] = "Device turned on";
            sendRequestJson(request, payload, 200);
        } else if (strcmp(state, "off") == 0) {
            if (animTaskHandle) vTaskSuspend(animTaskHandle);
            ledMatrix.fillScreen(0);
            ledMatrix.show();
            displayOn = false;
            saveMetaToFS();

            payload["success"] = true;
            payload["message"] = "Device turned off";
            sendRequestJson(request, payload, 200);
        } else {
            payload["success"] = false;
            payload["message"] = "Invalid 'state' value, expected 'on' or 'off'";
            sendRequestJson(request, payload, 400);
        } });

  // Power control - Get status (Quick win #11)
  webServer.on("/device/power", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        JsonDocument payload;
        payload["success"] = true;
        payload["state"] = displayOn ? "on" : "off";
        sendRequestJson(request, payload, 200); });

  // Device config - Get configurations
  webServer.on("/device/config", HTTP_GET, [](AsyncWebServerRequest *request)
               {
        JsonDocument payload;
        payload["success"] = true;
        payload["repeats"] = repeatsPerAnim;
        payload["brightness"] = brightness;
        sendRequestJson(request, payload, 200); });

  // Device config - Set configurations
  webServer.on("/device/config", HTTP_PATCH, [](AsyncWebServerRequest *r) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
        JsonDocument body;
        JsonDocument payload;

        DeserializationError error = deserializeJson(body, data, len);

        if(error) {
            payload["success"] = false;
            payload["message"] = "Invalid JSON";
            sendRequestJson(request, payload, 400);
            return;
        }

        bool updated = false;
        
        // Validación de repeats (#15)
        if (body["repeats"].is<uint8_t>()) {
            uint8_t newRepeats = body["repeats"];
            if (newRepeats < 1 || newRepeats > 100) {
                payload["success"] = false;
                payload["message"] = "Invalid repeats value (must be 1-100)";
                sendRequestJson(request, payload, 400);
                return;
            }
            repeatsPerAnim = newRepeats;
            updated = true;
        }

        // Validación de brightness (#15)
        if(body["brightness"].is<uint8_t>()) {
            uint8_t newBrightness = body["brightness"];
            if (newBrightness < 1 || newBrightness > 255) {
                payload["success"] = false;
                payload["message"] = "Invalid brightness value (must be 1-255)";
                sendRequestJson(request, payload, 400);
                return;
            }
            brightness = newBrightness;
            ledMatrix.setBrightness(brightness);
            updated = true;
        }

        if (updated) {
            saveMetaToFS();
        }

        payload["success"] = true;
        payload["message"] = "Configuration updated";
        payload["repeats"] = repeatsPerAnim;
        payload["brightness"] = brightness;
        sendRequestJson(request, payload, 200); });

  // ── /upload ────────────────────────────────────────────────────────────
  webServer.on("/upload", HTTP_POST, [](AsyncWebServerRequest *r) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
        JsonDocument payload;
        if (index == 0) {
          uploadReceived = 0;

          // Validar tamaño máximo de upload (Quick win #14)
          // Máximo: 500KB (aproximadamente 65 frames de 16x16 RGB)
          const size_t MAX_UPLOAD_SIZE = 512000;
          if (total > MAX_UPLOAD_SIZE) {
            payload["success"] = false;
            payload["message"] = "File too large (max 500KB)";
            sendRequestJson(request, payload, 413);
            return;
          }

          // Timeout de 1 segundo (optimización #8)
          if (xSemaphoreTake(animMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
          {
            payload["success"] = false;
            payload["message"] = "Server busy, try again";
            sendRequestJson(request, payload, 503);
            return;
          }
          
          if (playlistSize >= MAX_ANIMS)
          {
            xSemaphoreGive(animMutex);

            payload["success"] = false;
            payload["message"] = "Playlist full";

            sendRequestJson(request, payload, 507);
            return;
          }
          uploadSlot = playlistSize;
          String tempID = generateUniqueID();  // Generar ID único
          strncpy(uploadID, tempID.c_str(), 7);
          uploadID[7] = '\0';
          xSemaphoreGive(animMutex);

          uploadBuffer = (uint8_t *)malloc(total);
          if (!uploadBuffer)
          {
            uploadSlot = -1;
            uploadID[0] = '\0';
            payload["success"] = false;
            payload["message"] = "Memory allocation failed";
            sendRequestJson(request, payload, 500);
            return;
          }
        }

        if (uploadSlot < 0 || !uploadBuffer) {
          if (uploadBuffer && index == 0) {
            free(uploadBuffer);
            uploadBuffer = nullptr;
          }
          return;
        }

        memcpy(uploadBuffer + index, data, len);
        uploadReceived += len;

        if (index + len == total)
        {
          if (total % FRAME_SIZE != 0)
          {
            free(uploadBuffer);
            uploadBuffer = nullptr;
            uploadSlot = -1;
            uploadID[0] = '\0';
            payload["success"] = false;
            payload["message"] = "Invalid length";
            sendRequestJson(request, payload, 400);
            return;
          }

          bool saved = saveAnimToFS(uploadID, uploadBuffer, total);
          free(uploadBuffer);
          uploadBuffer = nullptr;

          if (!saved)
          {
            uploadSlot = -1;
            uploadID[0] = '\0';
            payload["success"] = false;
            payload["message"] = "Error saving to flash";
            sendRequestJson(request, payload, 500);
            return;
          }

          // Timeout de 1 segundo (optimización #8)
          if (xSemaphoreTake(animMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
          {
            uploadSlot = -1;
            uploadID[0] = '\0';
            payload["success"] = false;
            payload["message"] = "Server busy, animation saved but not added to playlist";
            sendRequestJson(request, payload, 500);
            return;
          }
          strncpy(playlist[uploadSlot].id, uploadID, 7);
          playlist[uploadSlot].id[7] = '\0';
          playlist[uploadSlot].byteLength = total;
          playlist[uploadSlot].frameCount = total / FRAME_SIZE;
          playlistSize++;
          newAnimPending = true;
          xSemaphoreGive(animMutex);

          saveMetaToFS();

          uploadSlot = -1;
          uploadID[0] = '\0';
          payload["success"] = true;
          payload["message"] = "Animation uploaded successfully";
          sendRequestJson(request, payload, 200);
        } });

  // Playlist
  // Get playlist info
  webServer.on("/playlist", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    JsonDocument payload;
  
    payload["count"] = playlistSize;
    payload["maxAnims"] = MAX_ANIMS;
    JsonArray anims = payload["anims"].to<JsonArray>();
   
    for (int i = 0; i < playlistSize; i++) {
      JsonObject anim = anims.add<JsonObject>();
      anim["index"] = i;
      anim["id"] = playlist[i].id;  // Quick win #17: incluir ID
      anim["frames"] = playlist[i].frameCount;
      anim["bytes"] = playlist[i].byteLength;
    }

    sendRequestJson(request, payload, 200); });

  // Delete animation from playlist
  webServer.on("/playlist", HTTP_DELETE, [](AsyncWebServerRequest *request)
               {
    JsonDocument payload;
    bool hasIndex = request->hasParam("index");

    if (hasIndex) {
      int idx = request->getParam("index")->value().toInt();
      
      // Timeout de 1 segundo (optimización #8)
      if (xSemaphoreTake(animMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
      {
        payload["success"] = false;
        payload["message"] = "Server busy, try again";
        sendRequestJson(request, payload, 503);
        return;
      }
      if (idx < 0 || idx >= playlistSize) {
        xSemaphoreGive(animMutex);
        payload["success"] = false;
        payload["message"] = "Invalid index";
        sendRequestJson(request, payload, 400);
        return;
      }
      
      // Obtener ID del archivo a borrar
      char idToDelete[8];
      strncpy(idToDelete, playlist[idx].id, 7);
      idToDelete[7] = '\0';
      
      // Actualizar array en memoria
      for (int i = idx; i < playlistSize - 1; i++) {
        playlist[i] = playlist[i + 1];
      }
      playlist[playlistSize - 1].id[0] = '\0';
      playlist[playlistSize - 1].frameCount = 0;
      playlist[playlistSize - 1].byteLength = 0;
      playlistSize--;
      newAnimPending = true;
      xSemaphoreGive(animMutex);

      // Borrar solo el archivo eliminado (operación rápida)
      String path = playlistFilename(idToDelete);
      LittleFS.remove(path);
      
      saveMetaToFS();
      
      payload["success"] = true;
      payload["message"] = "Animation deleted";
      sendRequestJson(request, payload, 200);

    } else {
      // Timeout de 1 segundo (optimización #8)
      if (xSemaphoreTake(animMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
      {
        payload["success"] = false;
        payload["message"] = "Server busy, try again";
        sendRequestJson(request, payload, 503);
        return;
      }
      
      // Guardar lista de IDs para borrar
      char idsToDelete[MAX_ANIMS][8];
      int count = playlistSize;
      for (int i = 0; i < count; i++) {
        strncpy(idsToDelete[i], playlist[i].id, 7);
        idsToDelete[i][7] = '\0';
      }
      
      playlistSize = 0;
      newAnimPending = true;
      xSemaphoreGive(animMutex);
      
      // Borrar todos los archivos
      for (int i = 0; i < count; i++) {
        String path = playlistFilename(idsToDelete[i]);
        LittleFS.remove(path);
      }
      LittleFS.remove("/anim/meta.txt");

      payload["success"] = true;
      payload["message"] = "Playlist cleared";
      sendRequestJson(request, payload, 200);
    }
  });
}
