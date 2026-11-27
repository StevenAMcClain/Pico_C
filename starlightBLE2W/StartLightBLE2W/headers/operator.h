// File: operator.h

#ifndef OPERATOR_H
#define OPERATOR_H

#include <stdbool.h>


typedef enum operator
{
    OP_UNKNOWN = 0,      // Always must be zero.

    //OP_COMMAND,

    // Postfix / primary
    OP_CALL,        // ()
    OP_INDEX,       // []
    OP_MEMBER,      // .

    // Unary
    OP_NEG,         // unary -
    OP_POS,         // unary +
    OP_NOT,         // !
    OP_BITNOT,      // ~

    // Multiplicative
    OP_MUL,
    OP_DIV,
    OP_MOD,

    // Additive
    OP_ADD,
    OP_SUB,

    // Shift
    OP_SHL,
    OP_SHR,

    // Relational
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,

    // Equality
    OP_EQ,
    OP_NE,

    // Bitwise
    OP_AND,
    OP_XOR,
    OP_OR,

    // Logical
    OP_LAND,
    OP_LOR,

    // Assignment
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_SHL_ASSIGN,
    OP_SHR_ASSIGN,
    OP_AND_ASSIGN,
    OP_XOR_ASSIGN,
    OP_OR_ASSIGN,
    OP_MOD_ASSIGN,

    OP_LIST,        // Comma operator (lowest precedence)
    OP_MAX
} operator_t;


extern operator_t Operator_Match(const char* opstring);

extern const char* Operator_String(operator_t op);

extern bool Is_Assignment_Op(operator_t op);
extern operator_t Compound_Base_Op(operator_t op);

extern int Operator_Precedence(operator_t op);

extern bool Is_Operator_Char(char c);

extern bool Is_Valid_Operator(const char* s);

#endif // OPERATOR_H

// End of file: operator.h
