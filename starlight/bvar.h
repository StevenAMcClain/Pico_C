// File: bvar.h

#ifndef BVAR_H
#define BVAR_H

//#include "beng.h"

//#include "blob.h"
//#include "stack.h"
//#include "shifter.h"

#define MAX_BENG_GLOBAL 10
#define MAX_BENG_LOCAL 10

#define BENG_VAR_IDX_MASK          ((1 << 27) - 1)
#define BENG_VAR_IDX_SCOPE_GLOBAL   (1 << 28)
#define BENG_VAR_IDX_SCOPE_INTERNAL (1 << 29)

#define BENG_VAR_TYPE_MASK        0x0000FFFF
#define BENG_VAR_TYPE_FORMAT_MASK 0x00FF0000
#define BENG_VAR_TYPE_SCOPE_MASK  0xFF000000


typedef enum
{
    BENG_VAR_TYPE_INT    = 0,       // 32 bit signed.
    BENG_VAR_TYPE_UINT   = 1,       // 32 bit unsigned.
    BENG_VAR_TYPE_LONG   = 2,       // 64 bit signed.
    BENG_VAR_TYPE_FLOAT  = 3,       // 32 bit float.
    BENG_VAR_TYPE_DOUBLE = 4,       // 64 bit float.

    BENG_VAR_TYPE_FORMAT_BINARY_8     = (1 << 16),
    BENG_VAR_TYPE_FORMAT_BINARY_16    = BENG_VAR_TYPE_FORMAT_BINARY_8 + 1,
    BENG_VAR_TYPE_FORMAT_BINARY_32    = BENG_VAR_TYPE_FORMAT_BINARY_8 + 2,
    BENG_VAR_TYPE_FORMAT_BINARY_64    = BENG_VAR_TYPE_FORMAT_BINARY_8 + 3,

    BENG_VAR_TYPE_FORMAT_DECIMAL_8    = (1 << 17),
    BENG_VAR_TYPE_FORMAT_DECIMAL_16   = BENG_VAR_TYPE_FORMAT_DECIMAL_8 + 1,
    BENG_VAR_TYPE_FORMAT_DECIMAL_32   = BENG_VAR_TYPE_FORMAT_DECIMAL_8 + 2,
    BENG_VAR_TYPE_FORMAT_DECIMAL_64   = BENG_VAR_TYPE_FORMAT_DECIMAL_8 + 3,

    BENG_VAR_TYPE_FORMAT_HEX_8        = (1 << 18),
    BENG_VAR_TYPE_FORMAT_HEX_16       = BENG_VAR_TYPE_FORMAT_HEX_8 + 1,
    BENG_VAR_TYPE_FORMAT_HEX_32       = BENG_VAR_TYPE_FORMAT_HEX_8 + 2,
    BENG_VAR_TYPE_FORMAT_HEX_64       = BENG_VAR_TYPE_FORMAT_HEX_8 + 3,

    BENG_VAR_TYPE_POINTER             = (1 << 30)

} BENG_VAR_TYPE;



typedef struct Blob_Var
{
    uint32_t name_strdx;
    uint32_t var_idx;
    uint32_t var_type;
    uint32_t var_value;

} BLOB_VAR;

#define BLOB_VAR_SIZE 4   // number of uint32_t for one BLOB_VAR


typedef union bvar
{
    void* ptr;
    int i;
    unsigned int ui;
    float f;
    double d;

} BVAR;



typedef struct Beng_Var
{
    BENG_VAR_TYPE type;

    int id;
    char* name;

    BVAR value;

    void (*Set)(struct Beng_Var* /*var*/);
    void (*Get)(struct Beng_Var* /*var*/);

} BENG_VAR;


extern BENG_VAR* BVar_Find_Global_By_Index(uint32_t /*idx*/);
extern BENG_VAR* BVar_Find_Internal_By_Index(uint32_t /*idx*/);

extern BENG_VAR* BVar_Find(void* /*bs*/, uint32_t /*idx*/);
extern BENG_VAR* BVar_Find_By_Name(void* /*bs*/, char* /*varname*/);

extern void BVar_Set(BENG_VAR* /*var*/);
extern void BVar_Get(BENG_VAR* /*var*/);

extern int          BVar_Get_int(BENG_VAR* /*var*/);
extern unsigned int BVar_Get_uint(BENG_VAR* /*var*/);
extern float        BVar_Get_float(BENG_VAR* /*var*/);
extern double       BVar_Get_double(BENG_VAR* /*var*/);
extern void*        BVar_Get_pointer(BENG_VAR* /*var*/);

extern void BVar_Set_int(BENG_VAR* /*var*/, int /*val*/);
extern void BVar_Set_uint(BENG_VAR* /*var*/, unsigned int /*val*/);
extern void BVar_Set_float(BENG_VAR* /*var*/, float /*val*/);
extern void BVar_Set_double(BENG_VAR* /*var*/, double /*val*/);
extern void BVar_Set_pointer(BENG_VAR* /*var*/, void* /*val*/);

extern size_t BVar_To_String(BENG_VAR* /*var*/, char* /*buff*/, size_t /*buffsize*/);

#endif  // BVAR_H

// Endfile: bvar.h
