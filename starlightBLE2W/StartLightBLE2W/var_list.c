// File: var_list.c

#include "common.h"
#include "var_list.h"

#include "atom.h"

#include <string.h>

#include "BLECore.h"   // For: BLE_Set_Node_Name()
#include "error.h"
#include "variables.h"

extern const char* BLE_Address_String;

extern uint32_t Last_MTU;  // Last MTU value, updated by the Notifier module.;

extern volatile uint32_t now_ms;        // Current time in milliseconds.
extern volatile uint32_t Tick_Count;    // Incremented every tick interval (50ms).
PUBLIC variable_t* Tick_Var = NULL;     // Pointer to the TICK variable (for watching).

PRIVATE char node_name[32] = "ABLEBIT111";   // Default node name, can be changed via command.


PRIVATE void set_node_name(const char* name)
{
    if (name && strlen(name) < sizeof(node_name))
    {
        strcpy(node_name, name);
        BLE_Set_Node_Name(node_name);
    }
}


PRIVATE const char* get_var_string(const atom_t* value)
{
    const char* str = NULL;

    if (value && value->data.ptr)
    {
        if (value->type == ATOM_TYPE_STRING)
        {
            str = (const char*)value->data.ptr;
        }
        else if (value->type == ATOM_TYPE_WORD)
        {
            str = value->data.word;
        }
        else if (value->type == ATOM_TYPE_RSTRING)
        {
            str = RLINK_PTR(value, value->data.rptr);
        }
        else { POST_ERROR("Cannot convert atom to string for type %d", value->type); }
    }
    return str;
}


PRIVATE bool set_node_name_var(variable_t* var, const atom_t* value)
{
    UNUSED(var);  // var is not used in this function, but we keep it for potential future use.

    const char* name = get_var_string(value);

    if (name) 
    {
        set_node_name(name); 
        return true;
    }
    else { POST_ERROR("NODE_NAME value is NULL"); }

    return false;
}

PRIVATE atom_t* get_ble_addr_var(variable_t* var)
{
    var->atom.data.cptr = BLE_Address_String;
    return &var->atom;
}


PUBLIC void add_vars(lexer_h lexer)
{
    atom_t atom;

    Atom_Zap(&atom);

    atom.next = 0;
 
    atom.type = ATOM_TYPE_UINT;
    atom.flags = ATOM_FLAG_IS_READONLY | ATOM_FLAG_IS_POINTER;
    balloc_h bh = Lexer_Heap(lexer);
    size_t* sp = Balloc_SizeP(bh);
    atom.data.ptr = sp;
    Var_Set("B_SIZE", &atom);                                                   // SP = {lexer->heap->size}

    atom.type = ATOM_TYPE_FLOAT;
    atom.flags = ATOM_FLAG_IS_READONLY;
    atom.data.f = 3.1415927;
    Var_Set("PI", &atom);                                                       // PI = 3.1415 (READ-ONLY)
 
    static FLOAT_TYPE myFloat = 0;
    atom.type = ATOM_TYPE_FLOAT;
    atom.flags = ATOM_FLAG_IS_POINTER;
    atom.data.ptr = &myFloat;
    variable_t* var = Var_Set("MYVAR", &atom);                                  // MYVAR = 0  (WATCHED)
    if (var) { var->flags |= VAR_FLAG_WATCHED; }
 
    atom.type = ATOM_TYPE_UINT;
    atom.flags = ATOM_FLAG_IS_READONLY | ATOM_FLAG_IS_POINTER;
    atom.data.vptr = &now_ms;
    Var_Set("NOW", &atom);                                                      // NOW = {now_ms}

    atom.type = ATOM_TYPE_UINT;
    atom.flags = ATOM_FLAG_IS_READONLY | ATOM_FLAG_IS_POINTER;
    atom.data.ptr = &Last_MTU;
    Var_Set("MTU", &atom);                                                     // MTU = (527?)

    atom.type = ATOM_TYPE_UINT;
    atom.flags = ATOM_FLAG_IS_POINTER;
    atom.data.ptr = &Debug_Mask;
    Var_Set("DEBUG_MASK", &atom);                                                    // DEBUG = {Debug_Mask}

    atom.type = ATOM_TYPE_UINT;
    atom.flags = ATOM_FLAG_IS_POINTER;
    atom.data.ptr = &Debug_Level;
    Var_Set("DEBUG_LEVEL", &atom);                                                    // DEBUG = {Debug_Mask}

    atom.type = ATOM_TYPE_UINT;
    atom.flags = ATOM_FLAG_IS_POINTER;
    atom.data.vptr = &Tick_Count;
    Tick_Var = Var_Set("TICK", &atom);                                          // TICK = {Tick_Count}
    if (Tick_Var) { Tick_Var->flags |= VAR_FLAG_WATCHED; }

    atom.type = ATOM_TYPE_STRING;
    atom.flags = ATOM_FLAG_IS_POINTER;
    atom.data.ptr = node_name;
    var = Var_Set("NODE_NAME", &atom);                                          // NODE_NAME = {node_name}
    if (var) { var->set_function = set_node_name_var; } 

    atom.type = ATOM_TYPE_STRING;
    atom.flags = ATOM_FLAG_IS_READONLY | ATOM_FLAG_IS_POINTER;
    atom.data.cptr = BLE_Address_String;
    var = Var_Set("MAC", &atom);                                               // MAC = {ble_address}
    if (var) { var->get_function = get_ble_addr_var; } 
}


// End of file: var_list.c