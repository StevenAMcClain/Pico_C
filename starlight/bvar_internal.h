// File: bvar_internal.h

#ifndef BVAR_INTERNAL_H
#define BVAR_INTERNAL_H

#include "bvar.h"

typedef enum
{
    IVX_ENGM,       // 0
    IVX_PHYM,       // 1
    IVX_BRIG,       // 2
    IVX_TIK0,       // 3
    IVX_TIK1,       // 4
    IVX_TIK2,       // 5
    IVX_TIK3,       // 6
    IVX_TIK4,       // 7
    IVX_TIK5,       // 8
    IVX_TIK6,       // 9
    IVX_TIK7,       // 10

} BVAR_INTERNAL_VAR_IDX;


extern BENG_VAR* BVar_Find_Internal(uint32_t /*idx*/);

#endif  // BVAR_INTERNAL_H

// Endfile: bvar_internal.h
