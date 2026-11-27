// File: evaluator.h

#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "atom.h"
#include "context.h"
#include "parser.h"

//--------------------------------------------------------------------------------
extern atom_t* Atom_New(context_t* context);
extern bool    Atom_Copy(context_t* context, atom_t* dst, const atom_t* src, bool persistent);

//--------------------------------------------------------------------------------
extern const atom_t *Eval_Expression(context_t* context, const atom_t* atom);
extern const atom_t *Eval_Expr_List (context_t* context, const atom_t* list);

//--------------------------------------------------------------------------------
extern const atom_t* Result_Bool   (context_t* context, bool val);
extern const atom_t* Result_CString(context_t* context, const char* str);
extern const atom_t* Result_Error  (context_t* context, const char* str);
extern const atom_t* Result_Null   (context_t* context);
extern const atom_t* Result_Printf (context_t* context, const char* fmt, ...);
extern const atom_t* Result_String (context_t* context, const char* str);
extern const atom_t* Result_UInt   (context_t* context, uint32_t value);

#endif // EVALUATOR_H

// End of file: evaluator.h
