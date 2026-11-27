// File: atom.h

#ifndef ATOM_H
#define ATOM_H

#include <math.h>
#include "common.h"

#include "operator.h"

//--------------------------------------
typedef int32_t RLINK;
#define PTR_RLINK(base, dest) ((ptrdiff_t)((char*)(dest) - (char*)(base)))
#define RLINK_PTR(base, offs) ((void*)((char*)(base) + (ptrdiff_t)(offs)))
#define RLINK_NEXT(p) ((p)->next ? RLINK_PTR((p), (p)->next) : NULL)

//--------------------------------------
typedef double FLOAT_TYPE;
typedef uint32_t INT_TYPE;

#define USE_EPSILON_FLOAT_COMPARE
#ifdef USE_EPSILON_FLOAT_COMPARE
#define FLOAT_COMPARE(a,b) (fabs((a) - (b)) < 0.000001)
#else
#define FLOAT_COMPARE(a,b) ((a) == (b))
#endif

//--------------------------------------
typedef enum
{
    ATOM_TYPE_UNDEFINED,    // 0 - nothing.
    ATOM_TYPE_BOOL,         // 1 - boolean value (stored in i)
    ATOM_TYPE_BYTE,         // 2 - unsigned 8-bit integer.
    ATOM_TYPE_INT,          // 3 - signed 32-bit integer.
    ATOM_TYPE_UINT,         // 4 - unsigned 32-bit integer.
    ATOM_TYPE_FLOAT,        // 5 - Floating point.
    ATOM_TYPE_WORD,         // 6 - ASCII word (may be name).
    ATOM_TYPE_OPERATOR,     // 7 - Operator
    ATOM_TYPE_PUNCT,  // 8 - Punctuation
    ATOM_TYPE_RSTRING,      // 9 - Relative string (stored in i)
    ATOM_TYPE_STRING,       // 10 - String (stored in ptr)
    ATOM_TYPE_ATOM,         // 11 - A pointer to an atom
    ATOM_TYPE_VARIABLE,     // 12 - A pointer to a variable.
    ATOM_TYPE_LIST,         // 13 - A list of atoms (stored in ptr)
    ATOM_TYPE_COMMAND,      // 14 - A pointer to a Command.
    ATOM_TYPE_EXPRESSION,   // 15 - A pointer to an Expression.

} atom_type_t;


typedef enum
{
    ATOM_FLAG_NONE           = 0,   // No extras.

    ATOM_FLAG_IS_CHARACTER   = 1,   // Character literal mode: 'c'
    ATOM_FLAG_IS_BINARY      = 2,   // Binary mode: 0b1010
    ATOM_FLAG_IS_HEXIDECIMAL = 4,   // Hex mode: 0xFF

    ATOM_FLAG_IS_POINTER     = 16,
    ATOM_FLAG_IS_READONLY    = 32

} atom_flag_t;
//    ATOM_FLAG_IS_NEGATIVE    = 2,   // Negative sign: -123

#define ATOM_FLAG_FORMAT_MASK 0x0007

typedef struct atom_expr
{
    struct atom* lhs;           // left hand side.
    operator_t   op;            // operator
    struct atom* rhs;           // right hand side.

} atom_expr_t;


typedef struct atom_call
{
    struct command*    cmd;     // must stay first, so it aliases data.cmd / data.ptr
    struct atom*      args;     // head of the argument chain (NULL if none)

} atom_call_t;


typedef struct atom_list
{
    struct atom*   head;        // head of the list chain (NULL if none)

} atom_list_t;


typedef struct atom
{
    RLINK next;                   // Relative offset to next atom.

    atom_type_t type;
    atom_flag_t flags;

    union
    {
        uint8_t b;                  // ATOM_TYPE_BYTE
        int32_t i;                  // ATOM_TYPE_INT
        uint32_t ui;                // ATOM_TYPE_UINT
        FLOAT_TYPE f;               // ATOM_TYPE_FLOAT
        operator_t op;              // ATOM_TYPE_OPERATOR

        void*           ptr;        // ATOM_TYPE_(*)
        const void*    cptr;        // ATOM_TYPE_(*) Constant
        volatile void* vptr;        // ATOM_TYPE_(*) Volatile
        ptrdiff_t      rptr;        // ATOM_TYPE_RSTRING
        
        struct command* cmd;        // ATOM_TYPE_COMMAND
        struct variable* var;       // ATOM_TYPE_VARIABLE

        atom_call_t call;           // ATOM_TYPE_COMMAND after parsing a call
        atom_expr_t expr;           // ATOM_TYPE_EXPRESSION
        atom_list_t list;           // ATOM_TYPE_LIST after parsing a call

        char word[MAX_WORD_SIZE];   // ATOM_TYPE_WORD
        
    } data;
    
} atom_t;


#define ATOM_VAL(a, t, v) (((a)->flags & ATOM_FLAG_IS_POINTER) ? *((volatile t*)(a)->data.ptr) : (a)->data.v)

extern void Atom_Zap(atom_t* atom);

extern void Atom_Shallow_Copy(atom_t* dest, const atom_t* src);

extern void Atom_Promote_List(atom_t* atom);

extern bool Atom_Is_Number(const atom_t* atom);
extern bool Atom_Equals(const atom_t* a, const atom_t* b);

//--------------------------------------------------------------------------------
extern bool Atom_To_Bool  (const atom_t* atom,   uint32_t* value);
extern bool Atom_To_Number(const atom_t* atom, FLOAT_TYPE* value);
extern bool Atom_To_I64   (const atom_t* atom,    int64_t* value);
extern bool Atom_To_U32   (const atom_t* atom,   uint32_t* value);


extern const char* Atom_TypeString(const atom_t* atom);

//extern void Atom_Promote(atom_t* atom);     // Try to convert word to 'better' type.
// extern void Atom_Dump(atom_t* atom, char** buffp, size_t* sizep);
// extern  void Atom_Dump_List(atom_t* atom, char** buffp, size_t* sizep);
// extern const char* Operator_String(operator_t op);

#endif // ATOM_H

// End of file: atom.h
