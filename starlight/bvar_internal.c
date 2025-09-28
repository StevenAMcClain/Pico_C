// File: bvar.c    -- Blob engine variables.

#include "common.h"
#include "bvar_internal.h"

#include <string.h>

// #include <stdlib.h>
// #include <string.h>

// #include "bvar.h"
// #include "bcmd.h"
// #include "debug.h"

PUBLIC int This_IS_A_Global = 99;


PRIVATE BENG_VAR BEng_Internal_Vars[MAX_BENG_INTERNAL] = 
{
    {BENG_VAR_TYPE_INT | BENG_VAR_TYPE_POINTER, 1000, "GLOB", .value.ptr = &This_IS_A_Global, NIL, NIL},
    {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
};


PUBLIC BENG_VAR* BVar_Find_Internal(uint32_t idx)
{
    return (idx < MAX_BENG_INTERNAL) ? BEng_Internal_Vars + idx : NULL;
}

PUBLIC BENG_VAR* BVar_Find_Internal_Name(char* name)
{
    BENG_VAR* varp = BEng_Internal_Vars;
    size_t n = MAX_BENG_INTERNAL;

    while (n--)
    {
        if (varp->name && strcmp(name, varp->name) == 0)
        {
            return varp;
        }
        ++varp;
    }
    return NIL;
}

// EndFile: bvar.c
