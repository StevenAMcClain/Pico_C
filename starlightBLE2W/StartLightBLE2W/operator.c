// File: operator.c

#include "common.h"
#include "operator.h"

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "error.h"
#include "lexer.h"
#include "variables.h"


typedef struct op
{
    const char* match;      // String for lexing.
    int precedence;         // Operator rank.
    bool do_match;          // True if ok to match.

} op_t;


PRIVATE const op_t operators[] =
{
    [OP_UNKNOWN]      = {.match = "???", .precedence = 0, .do_match = false},

    // Postfix / primary
    [OP_CALL]         = {.match = "()",  .precedence = 100, .do_match = false},  // function call
    [OP_INDEX]        = {.match = "[]",  .precedence = 100, .do_match = false},  // indexing
    [OP_MEMBER]       = {.match = ".",   .precedence = 100, .do_match = false},  // member access

    // Unary
    [OP_NEG]          = {.match = "-",   .precedence = 90, .do_match = true},  // unary -
    [OP_POS]          = {.match = "+",   .precedence = 90, .do_match = true},  // unary +
    [OP_NOT]          = {.match = "!",   .precedence = 90, .do_match = true }, // logical not
    [OP_BITNOT]       = {.match = "~",   .precedence = 90, .do_match = true }, // bitwise not

    // Multiplicative
    [OP_MUL]          = {.match = "*",   .precedence = 80, .do_match = true },
    [OP_DIV]          = {.match = "/",   .precedence = 80, .do_match = true },
    [OP_MOD]          = {.match = "%",   .precedence = 80, .do_match = true },

    // Additive
    [OP_ADD]          = {.match = "+",   .precedence = 70, .do_match = true },
    [OP_SUB]          = {.match = "-",    .precedence = 70, .do_match = true },

    // Shift
    [OP_SHL]          = {.match = "<<",  .precedence = 60, .do_match = true },
    [OP_SHR]          = {.match = ">>",  .precedence = 60, .do_match = true },

    // Relational
    [OP_LT]           = {.match = "<",   .precedence = 50, .do_match = true },
    [OP_LE]           = {.match = "<=",  .precedence = 50, .do_match = true },
    [OP_GT]           = {.match = ">",   .precedence = 50, .do_match = true },
    [OP_GE]           = {.match = ">=",  .precedence = 50, .do_match = true },

    // Equality
    [OP_EQ]           = {.match = "==",  .precedence = 40, .do_match = true },
    [OP_NE]           = {.match = "!=",  .precedence = 40, .do_match = true },

    // Bitwise
    [OP_AND]          = {.match = "&",   .precedence = 35, .do_match = true },
    [OP_XOR]          = {.match = "^",   .precedence = 34, .do_match = true },
    [OP_OR]           = {.match = "|",   .precedence = 33, .do_match = true },

    // Logical
    [OP_LAND]         = {.match = "&&",  .precedence = 30, .do_match = true },
    [OP_LOR]          = {.match = "||",  .precedence = 20, .do_match = true },

    // Assignment
    [OP_ASSIGN]       = {.match = "=",   .precedence = 10, .do_match = true },
    [OP_ADD_ASSIGN]   = {.match = "+=",  .precedence = 10, .do_match = true },
    [OP_SUB_ASSIGN]   = {.match = "-=",  .precedence = 10, .do_match = true },
    [OP_MUL_ASSIGN]   = {.match = "*=",  .precedence = 10, .do_match = true },
    [OP_DIV_ASSIGN]   = {.match = "/=",  .precedence = 10, .do_match = true },
    [OP_SHL_ASSIGN]   = {.match = "<<=", .precedence = 10, .do_match = true },
    [OP_SHR_ASSIGN]   = {.match = ">>=", .precedence = 10, .do_match = true },
    [OP_AND_ASSIGN]   = {.match = "&=",  .precedence = 10, .do_match = true },
    [OP_XOR_ASSIGN]   = {.match = "^=",  .precedence = 10, .do_match = true },
    [OP_OR_ASSIGN]    = {.match = "|=",  .precedence = 10, .do_match = true },
    [OP_MOD_ASSIGN]   = {.match = "%=",  .precedence = 10, .do_match = true },

    [OP_LIST]         = {.match = ",",   .precedence = 1, .do_match = true },
};


PUBLIC const char* Operator_String(operator_t op)
{
    if (op >= OP_MAX) { op = OP_UNKNOWN; }
    return operators[op].match;
}


// ----------------------------
// Operator precedence
// (unknown operators return 0, which ends the expression at the top level)
//
PUBLIC int Operator_Precedence(operator_t op)
{
    if (op >= OP_MAX) { op = OP_UNKNOWN; }
    return operators[op].precedence;
}


PUBLIC operator_t Operator_Match(const char* opstring)
{
    const op_t* op_rec = operators;
    operator_t op_code = OP_UNKNOWN;

    while (op_code < OP_MAX)
    {
        if (strcmp(opstring, op_rec->match) == 0) { break; }

        ++op_code;  ++op_rec;
    }

    if (op_code == OP_MAX) { return OP_UNKNOWN; }

    return (op_rec->do_match) ? op_code : OP_UNKNOWN;
}


PUBLIC bool Is_Assignment_Op(operator_t op)
{
    switch (op)
    {
        case OP_ASSIGN:
        case OP_ADD_ASSIGN: case OP_SUB_ASSIGN: case OP_MUL_ASSIGN:
        case OP_DIV_ASSIGN: case OP_MOD_ASSIGN:
        case OP_SHL_ASSIGN: case OP_SHR_ASSIGN:
        case OP_AND_ASSIGN: case OP_XOR_ASSIGN: case OP_OR_ASSIGN:
            return true;

        default: return false;
    }
}


PUBLIC operator_t Compound_Base_Op(operator_t op)
{
    switch (op)
    {
        case OP_ADD_ASSIGN: return OP_ADD;
        case OP_SUB_ASSIGN: return OP_SUB;
        case OP_MUL_ASSIGN: return OP_MUL;
        case OP_DIV_ASSIGN: return OP_DIV;
        case OP_MOD_ASSIGN: return OP_MOD;
        case OP_SHL_ASSIGN: return OP_SHL;
        case OP_SHR_ASSIGN: return OP_SHR;
        case OP_AND_ASSIGN: return OP_AND;
        case OP_XOR_ASSIGN: return OP_XOR;
        case OP_OR_ASSIGN:  return OP_OR;
        default:            return OP_UNKNOWN;
    }
}


PUBLIC bool Is_Operator_Char(char c)
{
    switch (c)
    {
        case '+': case '-': case '*': case '/': case '%':
        case '<': case '>': case '=': case '!':
        case '&': case '|': case '^': case '~':
        case '?': case ':': 
        { return true; }
    }
    return false;
}

PUBLIC bool Is_Valid_Operator(const char* s)
{
    // Multi-character operators
    static const char* const otab[] = { "==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "+=", "-=",
                                        "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "->", "++", "--" };

    // Single-char operators
    if (strlen(s) == 1) { return Is_Operator_Char(s[0]); }

    for (size_t i= 0; i < ARRAY_SIZE(otab); ++i)
    {
        if (!strcmp(s, otab[i])) { return true; }
    }
    return false;
}


// End of file: operator.c
