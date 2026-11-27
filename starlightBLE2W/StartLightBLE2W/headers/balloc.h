// File: balloc.h

#ifndef BALLOC_H
#define BALLOC_H

#include "common.h"

typedef void* balloc_h;

extern balloc_h PSRAM_Balloc;

extern balloc_h PSram_Init(void);

extern balloc_h Balloc_Init(uint8_t* base, size_t size);

extern void Balloc_Clear(balloc_h bh);

extern void* Balloc(balloc_h bp, size_t size);
extern void* Ballocz(balloc_h bp, size_t size);

extern bool Balloc_Reserve(balloc_h bh);
extern void Balloc_Release(balloc_h bh);

extern   void* Balloc_Base (balloc_h bh);
extern   void* Balloc_Top  (balloc_h bh);
extern   void* Balloc_Next (balloc_h bh);
extern size_t  Balloc_Left (balloc_h bh);
extern size_t  Balloc_Size (balloc_h bh);
extern size_t* Balloc_SizeP(balloc_h bh);

#endif // BALLOC_H

// End of file: balloc.h
