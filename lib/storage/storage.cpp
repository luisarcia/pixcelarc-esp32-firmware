#include "storage.h"
#include "config.h"
#include <LittleFS.h>

String generateUniqueID()
{
  // Genera un ID hexadecimal de 4 caracteres basado en millis() y random
  randomSeed(millis() + esp_random());
  uint16_t num = random(0x1000, 0xFFFF);
  String id = String(num, HEX);

  // Verificar que no exista (muy raro, pero por seguridad)
  while (LittleFS.exists("/anim/anim_" + id + ".bin"))
  {
    num = random(0x1000, 0xFFFF);
    id = String(num, HEX);
  }

  return id;
}

String playlistFilename(String id)
{
  return "/anim/anim_" + id + ".bin";
}

bool saveAnimToFS(String id, uint8_t *data, size_t length)
{
  String path = playlistFilename(id);
  File f = LittleFS.open(path, "w");
  if (!f)
  {
    Serial.printf("❌ No se pudo abrir %s\n", path.c_str());
    return false;
  }
  size_t written = f.write(data, length);
  f.close();
  Serial.printf("💾 Anim %s guardada (%u bytes)\n", id.c_str(), written);
  return written == length;
}

void saveMetaToFS()
{
  File f = LittleFS.open("/anim/meta.txt", "w");
  if (!f)
    return;

  // Guardar configuración general
  f.printf("%d\n%d\n%d\n%d\n", playlistSize, (int)displayOn, brightness, repeatsPerAnim);

  // Guardar orden de IDs
  for (int i = 0; i < playlistSize; i++)
  {
    f.printf("%s\n", playlist[i].id);
  }

  f.close();
  Serial.println("💾 Meta guardada");
}

void loadMetaFromFS()
{
  if (!LittleFS.exists("/anim/meta.txt"))
  {
    Serial.println("⚠️ No existe meta.txt");
    return;
  }

  File f = LittleFS.open("/anim/meta.txt", "r");
  if (!f)
    return;

  // Leer configuración general con validación (optimización #9)
  int tempPlaylistSize = f.readStringUntil('\n').toInt();
  int tempDisplayOn = f.readStringUntil('\n').toInt();
  int tempBrightness = f.readStringUntil('\n').toInt();
  int tempRepeats = f.readStringUntil('\n').toInt();

  // Validar rangos
  if (tempPlaylistSize < 0 || tempPlaylistSize > MAX_ANIMS)
  {
    Serial.printf("⚠️ playlistSize inválido: %d, usando 0\n", tempPlaylistSize);
    tempPlaylistSize = 0;
  }

  if (tempBrightness < 1 || tempBrightness > 255)
  {
    Serial.printf("⚠️ brightness inválido: %d, usando %d\n", tempBrightness, MATRIX_BRIGHTNESS_DEFAULT);
    tempBrightness = MATRIX_BRIGHTNESS_DEFAULT;
  }

  if (tempRepeats < 1 || tempRepeats > 100)
  {
    Serial.printf("⚠️ repeats inválido: %d, usando 2\n", tempRepeats);
    tempRepeats = 2;
  }

  // Asignar valores validados
  playlistSize = tempPlaylistSize;
  displayOn = (tempDisplayOn != 0);
  brightness = tempBrightness;
  repeatsPerAnim = tempRepeats;

  // Leer IDs en orden y reconstruir playlist
  for (int i = 0; i < playlistSize; i++)
  {
    String idStr = f.readStringUntil('\n');
    idStr.trim(); // Quitar espacios/saltos de línea

    if (idStr.length() == 0)
      break;

    // Copiar ID al array
    strncpy(playlist[i].id, idStr.c_str(), 7);
    playlist[i].id[7] = '\0';

    // Buscar archivo con ese ID
    String path = playlistFilename(playlist[i].id);
    if (LittleFS.exists(path))
    {
      File animFile = LittleFS.open(path, "r");
      if (animFile)
      {
        size_t size = animFile.size();
        animFile.close();

        if (size > 0 && size % FRAME_SIZE == 0)
        {
          playlist[i].byteLength = size;
          playlist[i].frameCount = size / FRAME_SIZE;
          Serial.printf("📂 Anim %s cargada (%u bytes, %u frames)\n", playlist[i].id, size, size / FRAME_SIZE);
        }
      }
    }
    else
    {
      Serial.printf("⚠️ Archivo no encontrado: %s\n", path.c_str());
    }
  }

  f.close();
  Serial.printf("📋 Playlist: %d animaciones cargadas\n", playlistSize);
}

size_t calcFsUsedReal()
{
  size_t total = 0;
  File root = LittleFS.open("/anim");
  if (!root || !root.isDirectory())
    return 0;
  File f = root.openNextFile();
  while (f)
  {
    total += f.size();
    f = root.openNextFile();
  }
  return total;
}
