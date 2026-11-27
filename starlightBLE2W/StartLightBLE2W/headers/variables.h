// File: variables.h

#ifndef VARIABLES_H
#define VARIABLES_H

#define MAX_VARIABLES 50

#include "atom.h"

struct variable;
typedef atom_t* (*get_function_t)(struct variable* var);
typedef bool (*set_function_t)(struct variable* var, const atom_t* value);   // Returns true if the value was accepted and stored, false if rejected.

typedef enum var_flags
{
    VAR_FLAG_NONE    = 0,
    VAR_FLAG_WATCHED = 1 << 0,  // Variable is being watched for changes.

} var_flags_t;


typedef struct variable
{
    var_flags_t flags;                 // Variable flags (e.g., read-only, pointer, etc.)

    const char* name;
    atom_t atom;

    set_function_t set_function;   // Optional function to call when the variable is set.  Replaces all var processing except read-only.
    get_function_t get_function;   // Optional function to call when the variable is retrieved.  Replaces all var processing.

    void* user_data;               // Optional user data associated with the variable.

} variable_t;


extern void Var_Initialize(void);

extern variable_t* Var_Set(const char* name, atom_t* value);

extern variable_t* Var_Get(const char* name);

extern variable_t* Var_Get_Index(size_t index);

extern bool Var_SetValue(variable_t* var, const atom_t* value);

extern void Var_Notify(variable_t* var);

extern bool Var_Update(variable_t* var, const atom_t* value);

extern void Dump_Variables(char** buffp, size_t* buffsizep, const char* glob);

#endif // VARIABLES_H

// End of file: variables.h