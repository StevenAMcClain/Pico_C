// File: bvar_internal.c    -- Blob engine internal variables.

#include "common.h"
#include "bvar_internal.h"

#include <string.h>

#include "bvar.h"
#include "beng.h"

extern uint32_t Debug_Mask;
extern uint32_t LED_Brightness;
extern BENG_STATE Beng_State[MAX_BENG];


PRIVATE void set_brightness(BENG_VAR* var)
{
    LED_Needs_Update(ALL_PHYS);
    LEDS_Do_Update();
}


PRIVATE BENG_VAR BEng_Internal_Vars[MAX_BENG_INTERNAL] = 
{
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  0,  "DEBUG", .value.ptr = &Debug_Mask,               NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  1, "BRIGHT", .value.ptr = &LED_Brightness,           set_brightness, NIL},
    // {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  2, "ENGINE", .value.ptr = &Engine_Idx,              NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  2, "ENGINE", .value.ptr = 0,              NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  3, "PMASK0", .value.ptr = &Beng_State[0].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  4, "PMASK1", .value.ptr = &Beng_State[1].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  5, "PMASK2", .value.ptr = &Beng_State[2].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  6, "PMASK3", .value.ptr = &Beng_State[3].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  7, "PMASK4", .value.ptr = &Beng_State[4].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  8, "PMASK5", .value.ptr = &Beng_State[5].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL +  9, "PMASK6", .value.ptr = &Beng_State[6].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 10, "PMASK7", .value.ptr = &Beng_State[7].phy_idx,   NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 11,  "TICK0", .value.ptr = &Beng_State[0].Tick_Speed, NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 12,  "TICK1", .value.ptr = &Beng_State[1].Tick_Speed, NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 13,  "TICK2", .value.ptr = &Beng_State[2].Tick_Speed, NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 14,  "TICK3", .value.ptr = &Beng_State[3].Tick_Speed, NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 15,  "TICK4", .value.ptr = &Beng_State[4].Tick_Speed, NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 16,  "TICK5", .value.ptr = &Beng_State[5].Tick_Speed, NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 17,  "TICK6", .value.ptr = &Beng_State[6].Tick_Speed, NIL, NIL},
    {BENG_VAR_TYPE_UINT | BENG_VAR_TYPE_POINTER, BENG_VAR_IDX_SCOPE_INTERNAL + 18,  "TICK7", .value.ptr = &Beng_State[7].Tick_Speed, NIL, NIL},
};


PUBLIC BENG_VAR* BVar_Find_Internal_By_Index(uint32_t idx)
{
    idx &= BENG_VAR_IDX_MASK;
    return (idx < MAX_BENG_INTERNAL) ? BEng_Internal_Vars + idx : NULL;
}


// PUBLIC BENG_VAR* BVar_Find_Internal_By_Name(char* name)
// {
//     BENG_VAR* var = BEng_Internal_Vars;
//     int i = 0;

//     while (i < MAX_BENG_INTERNAL)
//     {
//         if (var->name && (strcmp(name, var->name) == 0))
//         {
//             return var;
//         }
//         ++i;  ++var;
//     }
//     return NIL;
// }


PUBLIC BENG_VAR* BVar_Find_Internal_By_Name(char* name)
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


// EndFile: bvar_internal.c
