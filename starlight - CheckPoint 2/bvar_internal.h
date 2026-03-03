// File: bvar_internal.h

#ifndef BVAR_INTERNAL_H
#define BVAR_INTERNAL_H

#include "bvar.h"

typedef enum
{
    IVX_DEBUG,      // 0
    IVX_BRIGHT,     // 1
    IVX_EMASK,      // 2
    IVX_PMASK0,     // 3
    IVX_PMASK1,     // 4
    IVX_PMASK2,     // 5
    IVX_PMASK3,     // 6
    IVX_PMASK4,     // 7
    IVX_PMASK5,     // 8
    IVX_PMASK6,     // 9
    IVX_PMASK7,     // 10
    IVX_TIK0,       // 11
    IVX_TIK1,       // 12
    IVX_TIK2,       // 13
    IVX_TIK3,       // 14
    IVX_TIK4,       // 15
    IVX_TIK5,       // 16
    IVX_TIK6,       // 17
    IVX_TIK7,       // 18

    MAX_BENG_INTERNAL

} BVAR_INTERNAL_VAR_IDX;


extern BENG_VAR* BVar_Find_Internal_By_Index(uint32_t /*idx*/);

extern BENG_VAR* BVar_Find_Internal_By_Name(char* /*name*/);

#endif  // BVAR_INTERNAL_H

// Endfile: bvar_internal.h
