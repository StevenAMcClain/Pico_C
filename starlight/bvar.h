// File: bvar.h

#ifndef BVAR_H
#define BVAR_H

//#include "beng.h"

//#include "blob.h"
//#include "stack.h"
//#include "shifter.h"

#define MAX_BENG_GLOBAL 10
#define MAX_BENG_LOCAL 10
#define MAX_BENG_INTERNAL 11

#define BENG_VAR_LOCAL_START  0
#define BENG_VAR_LOCAL_END    (BENG_VAR_LOCAL_START + MAX_BENG_LOCAL)

#define BENG_VAR_GLOBAL_START  100
#define BENG_VAR_GLOBAL_END    (BENG_VAR_GLOBAL_START + MAX_BENG_GLOBAL)

#define BENG_VAR_INTERNAL_START 1000
#define BENG_VAR_INTERNAL_END   (BENG_VAR_INTERNAL_START + MAX_BENG_INTERNAL)


typedef enum
{
    BENG_VAR_TYPE_INT,
    BENG_VAR_TYPE_UINT,
    BENG_VAR_TYPE_FLOAT,
    BENG_VAR_TYPE_DOUBLE,

    BENG_VAR_SCOPE_TYPE_GLOBAL = (1 << 28),
    BENG_VAR_SCOPE_TYPE_INTERNAL = (1 << 29),

    BENG_VAR_TYPE_POINTER = (1 << 30)

} BENG_VAR_TYPE;

#define BENG_VAR_TYPE_MASK 0xFFFF


typedef struct Blob_Var
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;

} BLOB_VAR;

#define BLOB_VAR_SIZE 4


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
    // union
    // {
    //     void* ptr;
    //     int i;
    //     unsigned int ui;
    //     float f;
    //     double d;
    // } value;

    void (*Set)(struct Beng_Var* /*var*/);
    void (*Get)(struct Beng_Var* /*var*/);

} BENG_VAR;


extern BENG_VAR* BVar_Find_Global(uint32_t /*idx*/);
extern BENG_VAR* BVar_Find_Internal(uint32_t /*idx*/);

extern BENG_VAR* BVar_Find(void* /*bs*/, uint32_t /*idx*/);
extern BENG_VAR* BVar_Find_Name(void* /*bs*/, char* /*varname*/);

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


#endif  // BVAR_H

// Endfile: bvar.h
