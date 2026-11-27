// File: psram.h

#ifndef PSRAM_H
#define PSRAM_H

#include "balloc.h"

extern bool PSRAM_Ready;
extern balloc_h PSRAM_Balloc;

extern balloc_h PSram_Init(void);

bool Is_Psram_Ptr(const void* ptr);

#endif // PSRAM_H

// End of file: psram.h
