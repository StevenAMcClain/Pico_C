// File: bvar.c    -- Blob engine variables.

#include "common.h"
#include "bvar_internal.h"

// #include <stdlib.h>
// #include <string.h>

// #include "bvar.h"
// #include "bcmd.h"
// #include "debug.h"

PUBLIC int This_IS_A_Global = 99;


PRIVATE BENG_VAR BEng_Internal_Vars[MAX_BENG_INTERNAL] = 
{
    {BENG_VAR_TYPE_INT | BENG_VAR_TYPE_POINTER, &This_IS_A_Global, NIL, NIL},
    {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
};


PUBLIC BENG_VAR* BVar_Find_Internal(uint32_t idx)
{
    return (idx < MAX_BENG_INTERNAL) ? BEng_Internal_Vars + idx : NULL;
}

// EndFile: bvar.c
