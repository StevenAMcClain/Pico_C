// File: variables.c

#include "common.h"
#include "variables.h"

#include <stdio.h>
#include <string.h>

#include "balloc.h"
#include "error.h"
#include "evaluator.h"
#include "globmatch.h"
#include "hashtab.h"
#include "print.h"
#include "watcher.h"


#define WATCHER_BUFF_SIZE 256

PRIVATE variable_t* variables = NULL;   // Pointer to the array of variables.
PRIVATE size_t variable_count = 0;      // Count of variables in the array.

PRIVATE hash_table_t* hashtab = NULL;   // Use a hash table to map names to ATT_VALUE pointers for quick lookup.


PRIVATE char* my_strdup(const char* s)
{
    if (!s) { return NULL; }

    size_t len = strlen(s) + 1;
    char* p = Balloc(PSRAM_Balloc, len);

    if (p) { memcpy(p, s, len); }

    return p;
}


PRIVATE variable_t* new_variable(const char* name, atom_type_t type)
{
    if (!variables || !name || !*name ||
        type == ATOM_TYPE_UNDEFINED ||
        variable_count >= MAX_VARIABLES) { return NULL; }

    char* copy = my_strdup(name);
    if (!copy) { return NULL; }

    variable_t* var = &variables[variable_count];
    memset(var, 0, sizeof(*var));

    var->name = copy;
    var->atom.type = type;

    ++variable_count;
    return var;
}


PUBLIC void Var_Initialize(void)
{
    variable_count = 0;
    variables = NULL;
    hashtab = NULL;

    if (!PSRAM_Balloc)
    {
        POST_ERROR("PSRAM allocator is unavailable!");
        return;
    }

    variables = Ballocz(PSRAM_Balloc, MAX_VARIABLES * sizeof(*variables));

    if (!variables)
    {
        POST_ERROR("Failed to allocate variable list!");
        return;
    }

    hashtab = Hash__Initialize(MAX_VARIABLES);

    if (!hashtab)
    {
        variables = NULL;
        POST_ERROR("Failed to initialize hash table!");
        return;
    }
}


PRIVATE bool atom_pointer_store_through(const atom_t *dst, const atom_t *src)
{
    if (!dst || !src) { return POST_ERROR("Invalid pointer assignment!"); } 

    void *val = dst->data.ptr;

    if (!val) { return POST_ERROR("NULL pointer destination!"); }

    switch (dst->type)
    {
        case ATOM_TYPE_BOOL:
        {
            uint32_t u;

            if (!Atom_To_Bool(src, &u)) { return false; }

            *(volatile bool*)val = (u != 0);
            return true;
        }
        case ATOM_TYPE_BYTE:
        {
            uint32_t u;

            if (!Atom_To_U32(src, &u)) { return false; }

            if (u > UINT8_MAX)
            {
                return POST_ERROR("Value out of range for BYTE!");
            }

            *(volatile uint8_t*)val = (uint8_t)u;
            return true;
        }
        case ATOM_TYPE_UINT:
        {
            uint32_t u;

            // Do not allow a negative signed integer to wrap around.
            if (src->type == ATOM_TYPE_INT &&
                ATOM_VAL(src, int32_t, i) < 0)
            {
                return POST_ERROR("Negative value cannot be assigned to UINT!");
            }

            if (!Atom_To_U32(src, &u)) { return false; }

            *(volatile uint32_t*)val = u;
            return true;
        }
        case ATOM_TYPE_INT:
        {
            int64_t i;

            if (!Atom_To_I64(src, &i)) { return false; }

            if (i < INT32_MIN || i > INT32_MAX)
            {
                return POST_ERROR("Value out of range for INT!");
            }

            *(volatile int32_t*)val = (int32_t)i;
            return true;
        }
        case ATOM_TYPE_FLOAT:
        {
            FLOAT_TYPE d;

            if (!Atom_To_Number(src, &d)) { return false; }

            *(volatile FLOAT_TYPE*)val = d;
            return true;
        }
        default: { return POST_ERROR("Can't assign through pointer of this type!"); }
    }
}

//     switch (dst->type)
//     {
//         case ATOM_TYPE_BOOL:  { uint32_t   u; if (!Atom_To_Bool  (src, &u)) return false; *(volatile uint8_t    *)val = (uint8_t)u;  return true; }
//         case ATOM_TYPE_BYTE:  { uint32_t   u; if (!Atom_To_U32   (src, &u)) return false; *(volatile uint8_t    *)val = (uint8_t)u;  return true; }
//         case ATOM_TYPE_UINT:  { uint32_t   u; if (!Atom_To_U32   (src, &u)) return false; *(volatile uint32_t   *)val = u;           return true; }
//         case ATOM_TYPE_INT:   {  int64_t   i; if (!Atom_To_I64   (src, &i)) return false; *(volatile  int32_t   *)val = (int32_t)i;  return true; }
//         case ATOM_TYPE_FLOAT: { FLOAT_TYPE d; if (!Atom_To_Number(src, &d)) return false; *(volatile FLOAT_TYPE *)val = d;           return true; }
//         default: { break; }
//     }

//     return POST_ERROR("Can't assign through pointer!");
// }


PUBLIC bool Var_SetValue(variable_t* var, const atom_t* value)
{
    if (!var || !value) { return false; }

    if (var->atom.flags & ATOM_FLAG_IS_READONLY) { return false; }

    if (var->set_function)   
    {
//        if (!var->set_function(var, value)) { return false; }
        return var->set_function(var, value);
    }

    if (var->atom.flags & ATOM_FLAG_IS_POINTER)
    {
        return atom_pointer_store_through(&var->atom, value);
    }

    atom_t updated;
    // Atom_Shallow_Copy(&updated, value);
    Atom_Copy(NULL, &updated, value, true);

    updated.next = 0;

    if (value->type == ATOM_TYPE_RSTRING)
    {
        const char* src = (const char*)RLINK_PTR(value, value->data.rptr);

        if (!src || !PSRAM_Balloc) { return false; }

        size_t len = strlen(src) + 1;
        char* copy = Balloc(PSRAM_Balloc, len);

        if (!copy) { return false; }

        memcpy(copy, src, len);

        updated.type = ATOM_TYPE_STRING;
        updated.flags = ATOM_FLAG_NONE;
        updated.data.ptr = copy;
    }

    var->atom = updated;

    return true;
}


PUBLIC void Var_Notify(variable_t* var)
{
    if (!(var->flags & VAR_FLAG_WATCHED)) { return; }

    char value_str[WATCHER_BUFF_SIZE] = {0};
    char* value_ptr = value_str;
    size_t value_size = sizeof(value_str);

    
    Print_Atom(&var->atom, &value_ptr, &value_size, false);
    Watcher_Post("{%s=%s}", var->name, value_str);
}


PUBLIC bool Var_Update(variable_t* var, const atom_t* value)
{
    if (!var || !value) { return POST_ERROR("Invalid argument!"); }

    if (var->atom.flags & ATOM_FLAG_IS_READONLY) { return POST_ERROR("Variable '%s' is read-only!", var->name); }

    if (!Var_SetValue(var, value)) { return false; }

    Var_Notify(var);

    return true;
}


PUBLIC variable_t* Var_Set(const char* name, atom_t* value)
{
    if (!hashtab || !variables || !name || !*name || !value)
    {
        return POST_ERROR("Invalid argument or uninitialized variables!");
    }

    // Existing name: update in place.  Never fall through to creating a second
    // variable; a rejected update must leave the original untouched.
    NAME hash = Hash__Lookup(hashtab, name, false);

    if (hash != 0)
    {
        variable_t* existing = Hash__Get_Value(hashtab, hash);

        if (!existing) { return POST_ERROR("Name '%s' has no variable!", name); }

        return Var_Update(existing, value) ? existing : NULL;   // Var_Update posts its own reason
    }

    // New name: reserve a hash slot, then the variable.
    hash = Hash__Lookup(hashtab, name, true);

    if (hash == 0) { return POST_ERROR("Failed to insert variable '%s'!", name); }

    variable_t* var = new_variable(name, value->type);

    if (!var)
    {
        Hash__Remove(hashtab, hash);                            // Drop the empty hash entry.
        return POST_ERROR("Failed to allocate variable '%s'!", name);
    }

    Hash__Set_Value(hashtab, hash, var);

    if (!Var_Update(var, value))
    {
        Hash__Remove(hashtab, hash);

        if (variable_count > 0) { --variable_count; }

        return POST_ERROR("Failed to initialize variable '%s'!", name);
    }

    return var;
}

// PUBLIC variable_t* Var_Set(const char* name, atom_t* value)
// {
//     if (!hashtab || !variables || !name || !*name || !value)
//     {
//         return POST_ERROR("Invalid argument or uninitialized variables!");
//     }

//     NAME hash = Hash__Lookup(hashtab, name, false);    // Try to update an existing variable first.

//     if (hash != 0)
//     {
//         variable_t* var = Hash__Get_Value(hashtab, hash);

//         if (!var) { return NULL; }

//         if (var && Var_Update(var, value)) { return var; }

// //        return (var && Var_Update(var, value)) ? var : POST_ERROR("Failed to update variable '%s'!", name);

//         /* Existing name with no associated variable: repair it below. */
//     }
//     else
//     {
//         hash = Hash__Lookup(hashtab, name, true);        // Insert the name. Zero means insertion failed.

//         if (hash == 0) { return POST_ERROR("Failed to insert variable '%s'!", name); }
//     }

//     variable_t* var = new_variable(name, value->type);

//     if (!var)
//     {
//         Hash__Remove(hashtab, hash);        // Delete empty hashtab entry.
//         return POST_ERROR("Failed to allocate variable '%s'!", name);
//     }

//     Hash__Set_Value(hashtab, hash, var);

//     if (!Var_Update(var, value))
//     {
//         Hash__Remove(hashtab, hash);

//         if (variable_count > 0) { --variable_count; }

//         return POST_ERROR("Failed to initialize variable '%s'!", name);
//     }

//     return var;    
// }


PUBLIC variable_t* Var_Get(const char* name)
{
    if (!hashtab || !name || !*name) { return NULL; }

    NAME hash = Hash__Lookup(hashtab, name, false);

    if (hash == 0) { return NULL; }

    variable_t* var = Hash__Get_Value(hashtab, hash);

    if (!var) { return POST_ERROR("Name '%s' found in hash table but no variable associated!", name); }

    if (var->get_function) { var->get_function(var); }

    return var;
}


PUBLIC variable_t* Var_Get_Index(size_t index)
{
    return (index < variable_count) ? &variables[index] : NULL;
}

PRIVATE void strip_trailing_newlines(char *s)
{
    size_t len = strlen(s);

    while (len > 0 &&
           (s[len - 1] == '\n' ||
            s[len - 1] == '\r'))
    {
        s[--len] = '\0';
    }
}


PUBLIC void Dump_Variables(char** buffp, size_t* buffsizep, const char* glob)
{
    BUFF_WRITER_START(buffp, buffsizep);
    BUFF_WRITER_PRINTF("Var_List:\n");

    const char* last_name  = NULL;   // last name printed
    size_t      last_index = 0;      // tie-break for duplicate names

    while (1)
    {
        const variable_t* best = NULL;
        size_t best_index = 0;

        for (size_t index = 0; ; ++index)
        {
            const variable_t* var = Var_Get_Index(index);
            if (!var) break;

            if (glob && globmatch(glob, var->name, 0) != 0) continue;

            // must sort strictly after the last one printed
            if (last_name)
            {
                int c = strcmp(var->name, last_name);
                if (c < 0 || (c == 0 && index <= last_index)) continue;
            }

            // keep the smallest candidate (earlier index wins ties)
            if (best && strcmp(var->name, best->name) >= 0) continue;

            best = var;
            best_index = index;
        }

        if (!best) break;

        BUFF_WRITER_PRINTF("%u '%s'\n", (unsigned)best_index, best->name);

        last_name  = best->name;
        last_index = best_index;
    }
    BUFF_WRITER_END(buffp, buffsizep);

    strip_trailing_newlines(*buffp);
}


// End of file: variables.c
