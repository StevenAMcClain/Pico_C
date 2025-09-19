// File: bvar.c    -- Blob engine variables.

#include "common.h"
#include "bvar.h"

#include "beng.h"
#include "bvar_internal.h"

// #include <stdlib.h>
// #include <string.h>

// #include "beng.h"
// #include "bcmd.h"
// #include "debug.h"


PRIVATE BENG_VAR BEng_Global_Vars[MAX_BENG_GLOBAL];


PUBLIC void BVar_Set(BENG_VAR* var)
 {
 }


PUBLIC void BVar_Get(BENG_VAR* var)
{
}


PUBLIC BENG_VAR* BVar_Find_Global(uint32_t idx)
{
    return (idx < MAX_BENG_GLOBAL) ? BEng_Global_Vars + idx : NULL;
}


PUBLIC BENG_VAR* BVar_Find(void* bs, uint32_t idx)
{
    if (idx >= BENG_VAR_LOCAL_START && idx < BENG_VAR_LOCAL_END)
    {
        idx -= BENG_VAR_LOCAL_START;
        return BVar_Find_Local(bs, idx);
    }
    else if (idx >= BENG_VAR_GLOBAL_START && idx < BENG_VAR_GLOBAL_END)
    {
        idx -= BENG_VAR_GLOBAL_START;
        return BVar_Find_Global(idx);
    }
    else if (idx >= BENG_VAR_INTERNAL_START && idx < BENG_VAR_INTERNAL_END)
    {
        idx -= BENG_VAR_INTERNAL_START;
        return BVar_Find_Internal(idx);
    }
    return NULL;
}


PUBLIC int BVar_Get_int(BENG_VAR* var)
{
    int result = 0;
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = *(int*)var->ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = (int)*(unsigned int*)var->ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (int)*(float*)var->ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (int)*(double*)var->ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = var->i; break; }
            case BENG_VAR_TYPE_UINT: { result = (int)var->ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (int)var->f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (int)var->d; break; }
        }
    }
    return result;
}


PUBLIC unsigned int BVar_Get_uint(BENG_VAR* var)
{
    unsigned int result = 0;
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (unsigned int)*(int*)var->ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = *(unsigned int*)var->ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (unsigned int)*(float*)var->ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (unsigned int)*(double*)var->ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (unsigned int)var->i; break; }
            case BENG_VAR_TYPE_UINT: { result = var->ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (unsigned int)var->f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (unsigned int)var->d; break; }
        }
    }
    return result;
}


PUBLIC float BVar_Get_float(BENG_VAR* var)
{
    float result = 0;
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (float)*(int*)var->ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = (float)*(unsigned int*)var->ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = *(float*)var->ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (float)*(double*)var->ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (float)var->i; break; }
            case BENG_VAR_TYPE_UINT: { result = (float)var->ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = var->f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (float)var->d; break; }
        }
    }
    return result;
}


PUBLIC double BVar_Get_double(BENG_VAR* var)
{
    double result = 0;
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (double)*(int*)var->ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = (double)*(unsigned int*)var->ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (double)*(float*)var->ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = *(double*)var->ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (double)var->i; break; }
            case BENG_VAR_TYPE_UINT: { result = (double)var->ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (double)var->f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = var->d; break; }
        }
    }
    return result;
}


PUBLIC void* BVar_Get_pointer(BENG_VAR* var)
{
    void* result = 0;
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        result = var->ptr; 
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = &var->i; break; }
            case BENG_VAR_TYPE_UINT: { result = &var->ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = &var->f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = &var->d; break; }
        }
    }
    return result;
}


PUBLIC void BVar_Set_int(BENG_VAR* var, int val)
{
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { *(int*)var->ptr = val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->ptr = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->ptr = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->ptr = (double)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->i = val; break; }
            case BENG_VAR_TYPE_UINT: { var->ui = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->f = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->d = (double)val; break; }
        }
    }
}


PUBLIC void BVar_Set_uint(BENG_VAR* var, unsigned int val)
{
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { *(int*)var->ptr = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->ptr = val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->ptr = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->ptr = (double)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->i = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { var->ui = val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->f = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->d = (double)val; break; }
        }
    }
}

PUBLIC void BVar_Set_float(BENG_VAR* var, float val)
{
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { *(int*)var->ptr = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->ptr = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->ptr = val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->ptr = (double)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->i = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { var->ui = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->f = val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->d = (double)val; break; }
        }
    }
}

PUBLIC void BVar_Set_double(BENG_VAR* var, double val)
{
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { *(int*)var->ptr = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->ptr = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->ptr = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->ptr = val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->i = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { var->ui = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->f = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->d = val; break; }
        }
    }
}


PUBLIC void BVar_Set_pointer(BENG_VAR* var, void* val)
{
    BENG_VAR_TYPE base = var->type & BENG_VAR_TYPE_MASK;
    BENG_VAR_TYPE modifier = var->type & ~BENG_VAR_TYPE_MASK;

    if (modifier & BENG_VAR_TYPE_POINTER)
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { *(int*)var->ptr = *(int*)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->ptr = *(unsigned int*)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->ptr = *(float*)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->ptr = *(double*)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->i = *(int*)val; break; }
            case BENG_VAR_TYPE_UINT: { var->ui = *(unsigned int*)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->f = *(float*)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->d = *(double*)val; break; }
        }
    }
}



//PUBLIC BENG_VAR* BVar_Find_Local(BENG_STATE* state, uint32_t idx)
//{}


// EndFile: bvar.c
