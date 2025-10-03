// File: bvar.c    -- Blob engine variables.

#include "common.h"
#include "bvar.h"

#include "beng.h"
#include "btstdio.h"
#include "bvar_internal.h"

#include <stdio.h>
#include <string.h>

// #include "bcmd.h"
// #include "debug.h"


PRIVATE BENG_VAR BEng_Global_Vars[MAX_BENG_GLOBAL];


PUBLIC void BVar_Set(BENG_VAR* var)
{
}


PUBLIC void BVar_Get(BENG_VAR* var)
{
}


PUBLIC BENG_VAR* BVar_Find_Global_By_Index(uint32_t idx)
{
    idx &= BENG_VAR_IDX_MASK;
    return (idx < MAX_BENG_GLOBAL) ? BEng_Global_Vars + idx : NULL;
}


PUBLIC BENG_VAR* BVar_Find(void* bs, uint32_t idx)
{
         if (idx & BENG_VAR_IDX_SCOPE_INTERNAL) { return BVar_Find_Internal_By_Index(idx);  }
    else if (idx & BENG_VAR_IDX_SCOPE_GLOBAL)   { return BVar_Find_Global_By_Index(idx);    }
    else                                        { return BVar_Find_Local_By_Index(bs, idx); }

    return NULL;
}


PUBLIC BENG_VAR* BVar_Find_By_Name(void* bs, char* name)
{
    BENG_VAR* bvar = NIL;

    if (name)
    {
        bvar = BVar_Find_Internal_By_Name(name);

        if (!bvar && Blob_Is_Loaded)
        {
            BLOB_VAR* varp = Blob.VarTab_Base;
            uint32_t n = Blob.Num_VarRecs;
            uint8_t* strs = Blob.StrindX;

            while (n--)
            {
                char* varname = varp->name_strdx ? (char*)(strs + varp->name_strdx - 1) : "<name?>";
    
                if (strcmp(name, varname) == 0)
                {
                    bvar = (varp->var_idx & BENG_VAR_IDX_SCOPE_GLOBAL) ? BVar_Find_Global_By_Index(varp->var_idx) : BVar_Find_Local_By_Index(bs, varp->var_idx);
                    break;
                }
    
//                BTPRINTF("%d) '%s' %d %d\n", varp->b, name, varp->c, varp->d);
                ++varp;
            }
        }
    }
    return bvar;
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
            case BENG_VAR_TYPE_INT: { result = *(int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = (int)*(unsigned int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (int)*(float*)var->value.ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (int)*(double*)var->value.ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = var->value.i; break; }
            case BENG_VAR_TYPE_UINT: { result = (int)var->value.ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (int)var->value.f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (int)var->value.d; break; }
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
            case BENG_VAR_TYPE_INT: { result = (unsigned int)*(int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = *(unsigned int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (unsigned int)*(float*)var->value.ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (unsigned int)*(double*)var->value.ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (unsigned int)var->value.i; break; }
            case BENG_VAR_TYPE_UINT: { result = var->value.ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (unsigned int)var->value.f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (unsigned int)var->value.d; break; }
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
            case BENG_VAR_TYPE_INT: { result = (float)*(int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = (float)*(unsigned int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = *(float*)var->value.ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (float)*(double*)var->value.ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (float)var->value.i; break; }
            case BENG_VAR_TYPE_UINT: { result = (float)var->value.ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = var->value.f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = (float)var->value.d; break; }
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
            case BENG_VAR_TYPE_INT: { result = (double)*(int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_UINT: { result = (double)*(unsigned int*)var->value.ptr; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (double)*(float*)var->value.ptr; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = *(double*)var->value.ptr; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = (double)var->value.i; break; }
            case BENG_VAR_TYPE_UINT: { result = (double)var->value.ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = (double)var->value.f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = var->value.d; break; }
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
        result = var->value.ptr; 
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { result = &var->value.i; break; }
            case BENG_VAR_TYPE_UINT: { result = &var->value.ui; break; }
            case BENG_VAR_TYPE_FLOAT: { result = &var->value.f; break; }
            case BENG_VAR_TYPE_DOUBLE: { result = &var->value.d; break; }
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
            case BENG_VAR_TYPE_INT: { *(int*)var->value.ptr = val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->value.ptr = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->value.ptr = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->value.ptr = (double)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->value.i = val; break; }
            case BENG_VAR_TYPE_UINT: { var->value.ui = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->value.f = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->value.d = (double)val; break; }
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
            case BENG_VAR_TYPE_INT: { *(int*)var->value.ptr = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->value.ptr = val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->value.ptr = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->value.ptr = (double)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->value.i = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { var->value.ui = val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->value.f = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->value.d = (double)val; break; }
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
            case BENG_VAR_TYPE_INT: { *(int*)var->value.ptr = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->value.ptr = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->value.ptr = val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->value.ptr = (double)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->value.i = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { var->value.ui = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->value.f = val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->value.d = (double)val; break; }
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
            case BENG_VAR_TYPE_INT: { *(int*)var->value.ptr = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->value.ptr = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->value.ptr = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->value.ptr = val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->value.i = (int)val; break; }
            case BENG_VAR_TYPE_UINT: { var->value.ui = (unsigned int)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->value.f = (float)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->value.d = val; break; }
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
            case BENG_VAR_TYPE_INT: { *(int*)var->value.ptr = *(int*)val; break; }
            case BENG_VAR_TYPE_UINT: { *(unsigned int*)var->value.ptr = *(unsigned int*)val; break; }
            case BENG_VAR_TYPE_FLOAT: { *(float*)var->value.ptr = *(float*)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { *(double*)var->value.ptr = *(double*)val; break; }
        }
    }
    else
    {
        switch (base)
        {
            case BENG_VAR_TYPE_INT: { var->value.i = *(int*)val; break; }
            case BENG_VAR_TYPE_UINT: { var->value.ui = *(unsigned int*)val; break; }
            case BENG_VAR_TYPE_FLOAT: { var->value.f = *(float*)val; break; }
            case BENG_VAR_TYPE_DOUBLE: { var->value.d = *(double*)val; break; }
        }
    }
}

PUBLIC size_t BVar_To_String(BENG_VAR* var, char* buff, size_t buffsize)
{
    size_t result = 0;

    *buff = 0;

    if (var)
    {
        BENG_VAR_TYPE fmt = var->type & BENG_VAR_TYPE_FORMAT_MASK;
        BENG_VAR_TYPE type = var->type & BENG_VAR_TYPE_MASK;

        if (fmt)
        {
            result = snprintf(buff, buffsize, "fmt vars not ready!");
        }
        else
        {
            switch (type)
            {
                case BENG_VAR_TYPE_INT:
                {
                    int val = BVar_Get_int(var);
                    result = snprintf(buff, buffsize, "%d", val);
                    break;
                }
                case BENG_VAR_TYPE_UINT:
                {
                    unsigned int val = BVar_Get_uint(var);
                    result = snprintf(buff, buffsize, "%u", val);
                    break;
                }
                case BENG_VAR_TYPE_FLOAT:
                {
                    float val = BVar_Get_float(var);
                    result = snprintf(buff, buffsize, "%3.3f", val);
                    break;
                }
                case BENG_VAR_TYPE_DOUBLE:
                {
                    double val = BVar_Get_double(var);
                    result = snprintf(buff, buffsize, "%3.3lf", val);
                    break;
                }
            }
        }
    }
    return result;
}


PUBLIC bool BVar_From_String(BENG_VAR* var, char* buff)
{
    bool result = false;

    if (var && buff)
    {
        BENG_VAR_TYPE type = var->type & BENG_VAR_TYPE_MASK;
        void* ptr = (var->type & BENG_VAR_TYPE_POINTER) ? var->value.ptr : &var->value;
        char* fmt = "<bad>";

        switch (type)
        {
            case BENG_VAR_TYPE_INT:    { fmt = "%d";   break; }
            case BENG_VAR_TYPE_UINT:   { fmt = "%u";   break; }
            case BENG_VAR_TYPE_FLOAT:  { fmt = "%f";   break; }
            case BENG_VAR_TYPE_DOUBLE: { fmt = "%lf";  break; }
        }
        result = (1 == sscanf(buff, fmt,  ptr));
    }
    return result;
}


// EndFile: bvar.c
