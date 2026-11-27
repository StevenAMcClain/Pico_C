// File: evaluator.c
//
#include "common.h"
#include "evaluator.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include "atom.h"
#include "balloc.h"
#include "command.h"
#include "error.h"
#include "operator.h"
#include "variables.h"
#include "error.h"


// Allocate a new (zeroed, UNDEFINED) atom on the eval heap.
//
PUBLIC atom_t* Atom_New(context_t* context)
{
    atom_t* atom = Balloc(context->heap, sizeof(atom_t));

    if (!atom) { return POST_ERROR("Out of memory (eval atom)"); }

    Atom_Zap(atom);

    return atom;
}



PRIVATE bool atom_copy_list(
    context_t* context,
    atom_t* dst,
    const atom_t* src,
    bool persistent)
{
    atom_t* head = NULL;
    atom_t* tail = NULL;

    for (const atom_t* item = src->data.ptr; item; item = RLINK_NEXT(item))
    {
        atom_t* copy;

        if (persistent)
        {
            if (!PSRAM_Balloc) { return POST_ERROR("Atom_Copy: PSRAM allocator unavailable"); }

            copy = Balloc(PSRAM_Balloc, sizeof(atom_t));
        }
        else { copy = Atom_New(context); }

        if (!copy) { return POST_ERROR("Atom_Copy: Out of memory copying list"); }

        if (!Atom_Copy(context, copy, item, persistent)) { return false; }

        // Build the new relative linked list.
        //
        if (tail) { tail->next = PTR_RLINK(tail, copy); }
        else      { head = copy; }

        tail = copy;
    }

    dst->data.ptr = head;

    return true;
}


PUBLIC bool Atom_Copy(
    context_t* context,
    atom_t* dst,
    const atom_t* src,
    bool persistent)
{
    if (!dst || !src || (!persistent && !context))
    {
        POST_ERROR("Atom_Copy: Invalid argument");
        return false;
    }

    // Copy the basic atom contents.
    Atom_Shallow_Copy(dst, src);

    // A copied atom must not inherit its source list linkage.
    dst->next = 0;

    switch (src->type)
    {
        case ATOM_TYPE_RSTRING:
        {
            const char* str = RLINK_PTR(src, src->data.rptr);

            if (persistent)
            {
                if (!PSRAM_Balloc) { return POST_ERROR("Atom_Copy: PSRAM allocator unavailable"); }

                size_t len = strlen(str) + 1;
                char* copy = Balloc(PSRAM_Balloc, len);

                if (!copy) { return POST_ERROR("Atom_Copy: Out of memory copying string"); }

                memcpy(copy, str, len);

                dst->type = ATOM_TYPE_STRING;
                dst->flags = ATOM_FLAG_NONE;
                dst->data.ptr = copy;
            }
            else { dst->data.rptr = PTR_RLINK(dst, str); }     // Recalculate the relative offset for the new atom.

            break;
        }
        case ATOM_TYPE_LIST:
        {
            // The list must have independent atom nodes.
            dst->data.ptr = NULL;

            return atom_copy_list(context, dst, src, persistent);
        }
        default: { break; }       // Other atom types can retain their existing value/pointer.
    }
    return true;
}


// Dereference variables to their value atom.  Commands are not evaluated here.
// Plain (non-pointer) snapshot of a value atom; reads a live variable once.
//
PRIVATE const atom_t* eval_value(context_t* context, const atom_t* atom)
{
    if (!atom) { return NULL; }
    if (!(atom->flags & ATOM_FLAG_IS_POINTER) || atom->type == ATOM_TYPE_COMMAND) return atom;
    if (!atom->data.ptr) { return POST_ERROR("NULL pointer source!"); }

    atom_t* val = Atom_New(context);

    if (!val) { return NULL; }

    val->type = atom->type;
    val->flags = atom->flags & ATOM_FLAG_FORMAT_MASK;

    switch (atom->type)
    {
        case ATOM_TYPE_BOOL:
        case ATOM_TYPE_BYTE:    val->data.b    = *(volatile uint8_t    *)atom->data.ptr; break;
        case ATOM_TYPE_INT:     val->data.i    = *(volatile int32_t    *)atom->data.ptr; break;
        case ATOM_TYPE_UINT:    val->data.ui   = *(volatile uint32_t   *)atom->data.ptr; break;
        case ATOM_TYPE_FLOAT:   val->data.f    = *(volatile FLOAT_TYPE *)atom->data.ptr; break;
        case ATOM_TYPE_STRING:  val->data.ptr  = atom->data.ptr; break;
        case ATOM_TYPE_RSTRING:
        {
            const char* str = RLINK_PTR(atom, atom->data.rptr);
            val->data.rptr = PTR_RLINK(val, str);
            break;
        }
        case ATOM_TYPE_LIST:
        {
            val->data.ptr = atom->data.ptr;
            break;
        }
        default: { return POST_ERROR("Can't read pointer of this (%d) type!", atom->type); }
    }
    return val;
}


// Dereference variables (and same-line WORDs that became variables) to a value.
PRIVATE const atom_t* eval_resolve_atom(context_t* context, const atom_t* atom)
{
    if (!atom) return NULL;

    if (atom->type == ATOM_TYPE_WORD)              // e.g. "x = 1; x + 1"
    {
        variable_t* var = Var_Get(atom->data.word);

        if (var) return eval_value(context, &var->atom);
    }
    else if (atom->type == ATOM_TYPE_VARIABLE)
    {
        variable_t *var = (variable_t *)atom->data.ptr;

        if (!var) { return POST_ERROR("Variable is NULL"); }

        return eval_value(context, &var->atom);
    }
    return atom;
}


// Create a numeric result atom from FLOAT (prefers INT when exact).
//
PRIVATE atom_t* eval_from_float(context_t* context, FLOAT_TYPE val)
{
    atom_t* atom = Atom_New(context);

    if (!atom) return NULL;

    // Exact integer that fits in INT32.
    //
    if (val >= (FLOAT_TYPE)INT32_MIN && val <= (FLOAT_TYPE)INT32_MAX)
    {
        int32_t i = (int32_t)val;

        if ((FLOAT_TYPE)i == val)
        {
            atom->type = ATOM_TYPE_INT;
            atom->data.i = i;

            return atom;
        }
    }
    // Exact integer in 2^31 .. 2^32-1.
    //
    else if (val > (FLOAT_TYPE)INT32_MAX && val <= (FLOAT_TYPE)UINT32_MAX)
    {
        uint32_t u = (uint32_t)val;

        if ((FLOAT_TYPE)u == val)
        {
            atom->type = ATOM_TYPE_UINT;
            atom->data.ui = u;

            return atom;
        }
    }

    atom->type = ATOM_TYPE_FLOAT;
    atom->data.f = (FLOAT_TYPE)val;

    return atom;
}


// INT if it fits, else UINT if it fits, else FLOAT.
//
PRIVATE atom_t* eval_from_i64(context_t* context, int64_t val)
{
    if (val >= INT32_MIN && val <= INT32_MAX)
    {
        atom_t *atom = Atom_New(context);

        if (atom) { atom->type = ATOM_TYPE_INT; atom->data.i = (int32_t)val; }

        return atom;
    }

    if (val >= 0 && val <= (int64_t)UINT32_MAX)
    {
        atom_t *atom = Atom_New(context);

        if (atom) { atom->type = ATOM_TYPE_UINT; atom->data.ui = (uint32_t)val; }

        return atom;
    }
    return eval_from_float(context, (FLOAT_TYPE)val);
}


PRIVATE atom_t *eval_bool(context_t *context, bool val)
{
    atom_t* atom = Atom_New(context);

    if (atom) 
    {
        atom->type = ATOM_TYPE_BOOL; 
        atom->data.i = val ? 1 : 0; 
    }

    return atom;
}


// -----------------------------------------------------------------------------
// Forward declaration
// -----------------------------------------------------------------------------
PRIVATE const atom_t* eval_expr (context_t* context, const atom_t* expr);
PRIVATE const atom_t* eval_cmd  (context_t* context, const atom_t* expr);
PRIVATE const atom_t* eval_child(context_t* context, const atom_t* child);


PRIVATE const atom_t* eval_list (context_t* context, const atom_t* atom)
{
    atom_t* new_item = Atom_New(context);

    if (new_item)
    {
        const atom_list_t* list = &atom->data.list;

        new_item->type = ATOM_TYPE_LIST;

        atom_t* out_head = NULL;
        atom_t* out_tail = NULL;

        for (const atom_t* item = list->head; item; item = RLINK_NEXT(item))
        {
            const atom_t* val = eval_child(context, item);

            if (!val) { return NULL; }

            atom_t* copy = Atom_New(context);

            if (!copy) { return NULL; }

            if (Atom_Copy(context, copy, val, false))
            {
                copy->next = 0;

                if (out_tail) { out_tail->next = PTR_RLINK(out_tail, copy); }
                else          { out_head       = copy; }

                out_tail = copy;
            }
            else { return NULL; }
        }
        new_item->data.ptr = out_head;
    }
    return new_item;
}


// Evaluate any node: plain atom, parsed command call, or expression.
//
PRIVATE const atom_t* eval_child(context_t* context, const atom_t* child)
{
    if (!child) { return NULL; }

    switch (child->type)
    {
        case ATOM_TYPE_COMMAND:    { return eval_cmd         (context, child); }
        case ATOM_TYPE_EXPRESSION: { return eval_expr        (context, child); }
        case ATOM_TYPE_LIST:       { return eval_list        (context, child); }
        default:                   { return eval_resolve_atom(context, child); }
    }
}


// -----------------------------------------------------------------------------
// Unary operators (operand already evaluated)
// 
PRIVATE const atom_t* eval_unary(context_t* context, operator_t op, const atom_t* rhs)
{
    switch (op)
    {
        case OP_SUB: case OP_NEG: case OP_POS:              // parser's unary minus
        {
            FLOAT_TYPE v;

            if (!Atom_To_Number(rhs, &v)) return NULL;

            if (op != OP_POS) v = -v;

            return eval_from_float(context, v);
        }
        case OP_NOT:
        {
            uint32_t b;

            if (!Atom_To_Bool(rhs, &b)) return NULL;

            return eval_bool(context, !b);
        }
        case OP_BITNOT:
        {
            uint32_t u;

            if (!Atom_To_U32(rhs, &u)) return NULL;

            atom_t *r = Atom_New(context);

            if (!r) return NULL;

            if (rhs->type == ATOM_TYPE_INT) { r->type = ATOM_TYPE_INT;  r->data.i  = ~u; }
            else                            { r->type = ATOM_TYPE_UINT; r->data.ui = ~u; }

            return r;
        }
        default: { return POST_ERROR("unsupported unary operator %d", op); }
    }
}


// -----------------------------------------------------------------------------
// Binary operators (both operands already evaluated)
// 
PRIVATE const atom_t* eval_binary(context_t* context, operator_t op, const atom_t* lhs, const atom_t* rhs)
{
    switch (op)
    {
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV:        // Arithmetic
        {
            FLOAT_TYPE lv, rv;

            if (!Atom_To_Number(lhs, &lv)) return NULL;
            if (!Atom_To_Number(rhs, &rv)) return NULL;

            FLOAT_TYPE r = 0.0;

            switch (op)
            {
                case OP_ADD: r = lv + rv; break;
                case OP_SUB: r = lv - rv; break;
                case OP_MUL: r = lv * rv; break;
                default:
                {
                    if (rv == 0.0) { return POST_ERROR("divide by zero"); }

                    r = lv / rv;
                    break;
                }
            }
            return eval_from_float(context, r);
        }
        case OP_MOD:
        {
            int64_t lv, rv;
            if (!Atom_To_I64(lhs, &lv)) return NULL;
            if (!Atom_To_I64(rhs, &rv)) return NULL;

            if (rv == 0) { return POST_ERROR("modulo by zero"); }

            return eval_from_i64(context, lv % rv);       // 64-bit: INT32_MIN % -1 is safe
        }
        case OP_SHL: case OP_SHR:
        {
            uint32_t v, s;

            if (!Atom_To_U32(lhs, &v)) { return NULL; }
            if (!Atom_To_U32(rhs, &s)) { return NULL; }

            if (s >= 32) { return POST_ERROR("shift count out of range");  }

            const bool is_signed = (lhs->type == ATOM_TYPE_INT);

            atom_t *r = Atom_New(context);

            if (!r) return NULL;

            if (op == OP_SHL)                        { v <<= s;          }    // unsigned shift: no signed-overflow UB
            else if (is_signed && (v & 0x80000000u)) { v   = ~(~v >> s); }    // arithmetic shift: replicate the sign bit
            else                                     { v >>= s;          }

            if (is_signed) { r->type = ATOM_TYPE_INT;  r->data.i  = (int32_t)v; }
            else           { r->type = ATOM_TYPE_UINT; r->data.ui = v; }

            return r;
        }
        case OP_LT: case OP_LE: case OP_GT: case OP_GE: case OP_EQ: case OP_NE:        // Relational
        {
            FLOAT_TYPE lv, rv;

            if (!Atom_To_Number(lhs, &lv)) return NULL;
            if (!Atom_To_Number(rhs, &rv)) return NULL;

            bool b = false;

            switch (op)
            {
                case OP_LT: b = (lv <  rv); break;
                case OP_LE: b = (lv <= rv); break;
                case OP_GT: b = (lv >  rv); break;
                case OP_GE: b = (lv >= rv); break;
                case OP_EQ: b = (lv == rv); break;
                default:    b = (lv != rv); break;
            }
            return eval_bool(context, b);
        }
        case OP_AND: case OP_XOR: case OP_OR:
        {
            uint32_t lv, rv;

            if (!Atom_To_U32(lhs, &lv)) return NULL;
            if (!Atom_To_U32(rhs, &rv)) return NULL;

            uint32_t res;

            switch (op)
            {
                case OP_AND: res = lv & rv; break;
                case OP_XOR: res = lv ^ rv; break;
                default:     res = lv | rv; break;
            }

            atom_t *r = Atom_New(context);

            if (!r) return NULL;

            if (lhs->type == ATOM_TYPE_INT && rhs->type == ATOM_TYPE_INT)
            {
                r->type   = ATOM_TYPE_INT;
                r->data.i = (int32_t)res;
            }
            else
            {
                r->type    = ATOM_TYPE_UINT;
                r->data.ui = res;
            }
            return r;
        }
        default: { return POST_ERROR("Unsupported operator %d!", op); }
    }
}


// Types that make sense as a stored variable value.
//
PRIVATE bool is_storable_type(atom_type_t t)
{
    switch (t)
    {
        case ATOM_TYPE_BOOL:   
        case ATOM_TYPE_BYTE:   case ATOM_TYPE_INT:
        case ATOM_TYPE_UINT:   case ATOM_TYPE_FLOAT:
        case ATOM_TYPE_STRING: case ATOM_TYPE_RSTRING:
        case ATOM_TYPE_LIST:                              { return true; }
        default:                                          { return false; }
    }
}


// Assignment: lhs is an existing VARIABLE, or a WORD (existing variable
// defined earlier on this line, or a new one).  The lhs is never evaluated
// as a value; the rhs is evaluated once.
//
PRIVATE const atom_t* eval_assign(context_t* context, const atom_t* atom)
{
    const atom_expr_t* expr = &atom->data.expr;

    const atom_t* target = expr->lhs;

    if (!target) { return POST_ERROR("Assignment requires variable lhs!"); }

    variable_t* var = NULL;
    const char* name = NULL;

    if (target->type == ATOM_TYPE_VARIABLE)
    {
        var = (variable_t*)target->data.ptr;

        if (!var) { return POST_ERROR("NULL variable in assignment!"); }

        name = var->name;
    }
    else if (target->type == ATOM_TYPE_WORD)
    {
        name = target->data.word;
        var = Var_Get(name);        // "x = 1; x = 2": the 2nd x was lexed as a WORD
    }
    else { return POST_ERROR("Assignment requires variable lhs!"); }

    if (var && (var->atom.flags & ATOM_FLAG_IS_READONLY))
    {
        return POST_ERROR("Variable '%s' is read-only!", name);
    }

    const atom_t* src = eval_child(context, expr->rhs);

    if (!src) { return NULL; }

    if (expr->op != OP_ASSIGN)      // Compound assignment: x += y
    {
        if (!var) { return POST_ERROR("Compound assignment to undefined variable '%s'!", name); }

        const atom_t* cur = eval_value(context, &var->atom);

        if (!cur) { return NULL; }

        src = eval_binary(context, Compound_Base_Op(expr->op), cur, src);

        if (!src) { return NULL; }
    }

    if (src->type == ATOM_TYPE_WORD)        // eval_resolve_atom left it unresolved
    {
        return POST_ERROR("Undefined name '%s'!", src->data.word);
    }

    if (!is_storable_type(src->type))
    {
        return POST_ERROR("Can't assign a value of this type!");
    }

    // Snapshot into a temporary; strings move to persistent storage here.
    //
    atom_t value;

    if (!Atom_Copy(context, &value, src, true)) return NULL;

    variable_t* result = Var_Set(name, &value);

    if (!result) return NULL;               // Var_Set already posted the reason

    return eval_value(context, &result->atom);
}


// Logical && and || with short-circuit evaluation.
//
PRIVATE const atom_t* eval_logical(context_t* context, const atom_t* atom)
{
    const atom_expr_t* expr = &atom->data.expr;
    const atom_t* lhs = eval_child(context, expr->lhs);

    if (!lhs) return NULL;

    uint32_t bval;

    if (!Atom_To_Bool(lhs, &bval)) return NULL;

    bool lb = bval;

    if (expr->op == OP_LAND && !lb) return eval_bool(context, false);
    if (expr->op == OP_LOR  &&  lb) return eval_bool(context, true);

    const atom_t* rhs = eval_child(context, expr->rhs);

    if (!rhs || !Atom_To_Bool(rhs, &bval)) { return NULL; }

    lb = bval;

    return eval_bool(context, lb);
}


PRIVATE const atom_t* eval_cmd(context_t* context, const atom_t* atom)
{
    const command_t* cmd = atom->data.ptr;

    if (!cmd) { return POST_ERROR("NULL command pointer!"); }

    size_t count = 0;

    for (const atom_t* arg = atom->data.call.args; arg; arg = RLINK_NEXT(arg)) { ++count; }

    const atom_t* ret = NULL;

    if (cmd->arg_mode == COMMAND_ARGS_RAW)
    {
        const atom_t** params = NULL;

        if (count)
        {
            params = Balloc(context->heap, sizeof(*params) * count);

            if (!params) { return POST_ERROR("Out of memory (command params)!"); }
        }

        size_t idx = 0;

        for (const atom_t* arg = atom->data.call.args; arg; arg = RLINK_NEXT(arg))
        {
            params[idx++] = arg;
        }

        uint32_t error_serial = Error_Serial;

        ret = Command_Run_Raw(context, cmd, count, params);

        // A command that posted an error has failed.
        // Do not convert its failure into an UNDEFINED result.

        if (Error_Serial != error_serial) { return NULL; }
    }
    else
    {
        atom_t* params = NULL;

        if (count)
        {
            params = Balloc(context->heap, sizeof(atom_t) * count);

            if (!params) { return POST_ERROR("Out of memory (command params)!"); }
        }

        size_t idx = 0;

        for (const atom_t* arg = atom->data.call.args; arg; arg = RLINK_NEXT(arg), ++idx)
        {
            const atom_t* val = eval_child(context, (const atom_t *)arg);

            if (!val) { return POST_ERROR("Command arg failed!"); }

            if (!Atom_Copy(context, &params[idx], val, false)) { return NULL; }
        }

        uint32_t error_serial = Error_Serial;

        ret = Command_Run_Cooked(context, cmd, count, params);

        // A command that posted an error has failed.
        // Do not convert its failure into an UNDEFINED result.

        if (Error_Serial != error_serial) { return NULL; }
    }

    if (!ret)
    {
        ret = Atom_New(context);
        if (!ret) { return NULL; }
    }

    return ret;
}


// -----------------------------------------------------------------------------
// Core expression evaluator
// -----------------------------------------------------------------------------
PRIVATE const atom_t* eval_expr(context_t* context, const atom_t* atom)
{
    if (!atom) { return POST_ERROR("null expression"); }

    const atom_expr_t* expr = &atom->data.expr;

    operator_t op = expr->op;

//    if (op == OP_COMMAND)              { return eval_cmd(context, expr); }

    if (Is_Assignment_Op(op))          { return eval_assign(context, atom); }

    if (op == OP_LAND || op == OP_LOR) { return eval_logical(context, atom); }

    if (!expr->lhs)                     // unary: rhs holds the operand
    {
        const atom_t* rhs = eval_child(context, expr->rhs);

        if (!rhs)
        {
            if (!expr->rhs) { POST_ERROR("Unary operator missing operand!"); }

            return NULL;                // otherwise the failed child already reported
        }
        return eval_unary(context, op, rhs);
    }

    if (!expr->rhs) { return POST_ERROR("Binary operator missing operand!"); }

    const atom_t* lhs = eval_child(context, expr->lhs);

    if (!lhs) { return NULL; }

    const atom_t* rhs = eval_child(context, expr->rhs);

    if (!rhs) { return NULL; }

    return eval_binary(context, op, lhs, rhs);
}


// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

PUBLIC const atom_t* Eval_Expression(context_t* context, const atom_t *expr)
{
    return eval_child(context, (const atom_t *)expr);        // may be a bare atom
}


PUBLIC const atom_t* Eval_Expr_List(context_t* context, const atom_t *list)
{
    const atom_t* last = NULL;

    while (list)
    {
        last = eval_child(context, (const atom_t *)list);

        if (!last) { return NULL; }      // stop at first failure

        list = RLINK_NEXT(list);
    }
    return last;    // value of the last expression
}

//---------------------------------------------------------------------------------------

PUBLIC const atom_t* Result_Bool(context_t* context, bool value)
{
    atom_t* result = Balloc(context->heap, sizeof(*result));

    if (result)
    {
        Atom_Zap(result);

        result->type = ATOM_TYPE_BOOL;
        result->flags = 0;
        result->data.ui = value ? 1u : 0u;
    }

    return result;
}


PUBLIC const atom_t* Result_Null(context_t* context)
{
    UNUSED(context);
    return NULL;
}


PUBLIC const atom_t* Result_Error(context_t* context, const char* str)
{
    UNUSED(context);
    return POST_ERROR("%s", str);
}


PUBLIC const atom_t* Result_CString(context_t* context, const char* str)
{
    if (!str) return NULL;

    atom_t* result = Balloc(context->heap, sizeof(atom_t));

    if (result)
    {
        Atom_Zap(result);
        result->type = ATOM_TYPE_STRING;
        result->data.ptr = (void*)str;
    } 

    return result;
}


PUBLIC const atom_t* Result_Printf(context_t* context, const char* fmt, ...)
{
    if (Balloc_Reserve(context->heap))
    {
        char* msg = Balloc_Top(context->heap);
        size_t len = Balloc_Left(context->heap);

        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, len, fmt, args);
        va_end(args);

        len = strlen(msg) + 1;           // Including the null.

        Balloc_Release(context->heap);

        if (len > 1) 
        {
            Balloc(context->heap, len);
            return Result_CString(context, msg);
        }
    }
    return NULL;  // Empty message: ignore.
}


PUBLIC const atom_t* Result_String(context_t* context, const char* str)
{
    if (!str) { return NULL; }

    size_t len = strlen(str) + 1;   // include NUL

    if (len < 2) { return NULL; }  // empty string is not valid

    const char* copy = Balloc(context->heap, len);

    if (copy)
    {
        strncpy((char*)copy, str, len);
        return Result_CString(context, copy);
    }
    
    return Result_CString(context, "");
}


PUBLIC const atom_t* Result_UInt(context_t* context, uint32_t value)
{
    atom_t* result = Balloc(context->heap, sizeof(atom_t));

    if (result)
    {
        Atom_Zap(result);
        result->type = ATOM_TYPE_UINT;
        result->data.ui = value;
    } 

    return result;
}


// End of file: evaluator.c
