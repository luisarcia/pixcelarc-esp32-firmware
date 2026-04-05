#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

String generateUniqueID();
String playlistFilename(String id);
bool   saveAnimToFS(String id, uint8_t *data, size_t length);
void   saveMetaToFS();
void   loadMetaFromFS();
size_t calcFsUsedReal();

#endif
