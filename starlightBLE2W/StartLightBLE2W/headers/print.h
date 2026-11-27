// File: print.h

#ifndef PRINT_H
#define PRINT_H

#include "atom.h"
#include "evaluator.h"

#include <stdio.h>
#include <string.h>


//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// BUFF_WRITER: append formatted text to a caller-supplied buffer.
//
//   BUFF_WRITER_START(buffp, sizep);       // opens a scope; declares 'buff' and 'buffsize'
//   BUFF_WRITER_PRINTF("fmt", args...);    // append; safe as a single statement
//   BUFF_WRITER_END(buffp, sizep);         // closes the scope and writes back pointer + size
//
// START and END open and close braces, so they must be paired in the same block.
// PRINTF, ADVANCE and WRITE are single statements and safe in unbraced if/while.
//
// 'buffsize' is the space left at 'buff', including room for the NUL.
// snprintf reserves the NUL itself, so START does not.

#define BUFF_WRITER_START(bp, bsp) \
    {\
        char*  buff     = *(bp);\
        size_t buffsize = *(bsp);\
        if (buff && buffsize > 1)\
        {\
            *buff = 0;

#define BUFF_WRITER_ADVANCE() \
    do { size_t bw_len = strlen(buff); buff += bw_len; buffsize -= bw_len; } while (0)

#define BUFF_WRITER_PRINTF(fmt, ...) \
    do { snprintf(buff, buffsize, fmt, ##__VA_ARGS__); BUFF_WRITER_ADVANCE(); } while (0)

#define BUFF_WRITER_END(bp, bsp) \
        }\
        *(bp)  = buff;\
        *(bsp) = buffsize;\
    }

#define BUFF_WRITER_WRITE(bp, bsp, fmt, ...) \
    do {\
        BUFF_WRITER_START(bp, bsp)\
        BUFF_WRITER_PRINTF(fmt, ##__VA_ARGS__);\
        BUFF_WRITER_END(bp, bsp)\
    } while (0)

//----------------------------------------------------------------------------------------------
extern void Print_Atom     (const atom_t* atom, char** buffp, size_t* sizep, bool plain);
extern void Print_Atom_List(const atom_t* atom, char** buffp, size_t* sizep, bool plain);
extern void Print_Atom_Type(const atom_t* atom, char** buffp, size_t* sizep);

extern void Dump_Expr     (const atom_t* expr,  char** buffp, size_t* sizep);
extern void Dump_Expr_List(const atom_t* list,  char** buffp, size_t* sizep);

#endif // PRINT_H

// End of file: print.h
