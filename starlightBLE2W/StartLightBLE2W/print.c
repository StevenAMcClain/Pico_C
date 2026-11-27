// File: print.c

#include "common.h"
#include "print.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "command.h"
#include "error.h"
#include "variables.h"


// ============================================================================
// ============                                                   =============
//              =========== S T R I N G   O U T P U T ===========
// ============                                                   =============
// ============================================================================

PRIVATE char* to_binary_trim(uint32_t v, char* buf)
{
    int pos = 0;
    int started = 0;

    for (int i = 31; i >= 0; i--)
    {
        if (v & (1u << i)) { started = 1; }

        if (started) { buf[pos++] = (v & (1u << i)) ? '1' : '0'; }
    }

    if (!started) { buf[pos++] = '0'; }

    buf[pos] = '\0';

    return buf;
}


PUBLIC const char* Atom_TypeString(const atom_t* atom)
{
    const char* result = "(?!?)";
    atom_flag_t fmt = atom->flags & ATOM_FLAG_FORMAT_MASK;

    switch (atom->type)
    {
        case ATOM_TYPE_BYTE:
        {
                 if (fmt == ATOM_FLAG_IS_BINARY)      { result = "(bin)";  }
            else if (fmt == ATOM_FLAG_IS_CHARACTER)   { result = "(char)"; }
            else if (fmt == ATOM_FLAG_IS_HEXIDECIMAL) { result = "(hex)";  }
            else                                      { result = "(byte)"; }
            break;
        }
        case ATOM_TYPE_INT:
        {
            // A signed value keeps its HEX/BIN flag (see ATOM_Promote), so
            // this has to branch on flag the same way ATOM_toString does -
            // otherwise the two disagree on the prefix for a signed
            // hex/bin parameter.
                 if (fmt == ATOM_FLAG_IS_BINARY)      { result = "(bin)"; }
            else if (fmt == ATOM_FLAG_IS_HEXIDECIMAL) { result = "(hex)"; }
            else                                      { result = "(int)"; }
            break;
        }
        case ATOM_TYPE_UINT:
        {
                 if (fmt == ATOM_FLAG_IS_BINARY)      { result = "(ubin)"; }
            else if (fmt == ATOM_FLAG_IS_HEXIDECIMAL) { result = "(uhex)"; }
            else                                      { result = "(uint)"; }
            break;
        }
        case ATOM_TYPE_BOOL:        { result = "(bool)";  break; }
        case ATOM_TYPE_FLOAT:       { result = "(float)"; break; }
        case ATOM_TYPE_WORD:        { result = "(word)";  break; }
        case ATOM_TYPE_OPERATOR:    { result = "(op)";    break; }
        case ATOM_TYPE_UNDEFINED:   { result = "(undef)"; break; }
        case ATOM_TYPE_COMMAND:     { result = "(cmd)";   break; }
        case ATOM_TYPE_EXPRESSION:  { result = "(expr)";  break; }
        case ATOM_TYPE_VARIABLE:    { result = "(var)";   break; }
        case ATOM_TYPE_STRING:      { result = "(str)";   break; }
        case ATOM_TYPE_RSTRING:     { result = "(rstr)";  break; }
        case ATOM_TYPE_PUNCT:       { result = "(punct)"; break; }
        case ATOM_TYPE_LIST:        { result = "(list)";  break; }
        case ATOM_TYPE_ATOM:        { result = "(atom)";  break; }
        default:                    { result = "(?*?)";   break; }
    }
    return result;
}


PUBLIC void Print_Atom_Type(const atom_t* atom, char** buffp, size_t* sizep)
{
    if (!atom || !buffp || !sizep || *sizep < 1) { return; }

    size_t size = *sizep;
    char* buff = *buffp;

    *buff = 0;

    const char* type_str = Atom_TypeString(atom);

    snprintf(buff, size, "%s", type_str);

    size_t len = strlen(buff);

    *buffp = buff + len;
    *sizep = size - len;
}


PRIVATE void print_atom_helper(const atom_t* atom, char** buffp, size_t* sizep, bool plain)
{
    if (!atom || !buffp || !sizep || *sizep < 1) { return; }

    atom_flag_t fmt = atom->flags & ATOM_FLAG_FORMAT_MASK;
    size_t size = *sizep;
    char* buff = *buffp;

    *buff = 0;

    switch (atom->type)
    {
        case ATOM_TYPE_UNDEFINED:
        {
            if (!plain)  { snprintf(buff, size, "<undefined>"); }
            break;
        }
        case ATOM_TYPE_BOOL:
        {
            if (plain) { snprintf(buff, size, "%s", ATOM_VAL(atom, int32_t, i) ? "1" : "0"); }
            else       { snprintf(buff, size, "%s", ATOM_VAL(atom, int32_t, i) ? "true" : "false"); }
            break;
        }
        case ATOM_TYPE_OPERATOR:
        {
            snprintf(buff, size, (plain) ? "%s" : "<%s>", Operator_String(atom->data.op));
            break;
        }
        case ATOM_TYPE_BYTE:
        {
            uint8_t val = ATOM_VAL(atom, uint8_t, b);

            if (fmt == ATOM_FLAG_IS_BINARY) 
            {
                char bin[33];
                snprintf(buff, size, "0b%s", to_binary_trim(val, bin));
            }
            else if (fmt == ATOM_FLAG_IS_CHARACTER) 
            {
                if (isprint(val)) { snprintf(buff, size,  (plain) ? "%c" : "'%c'", val); }
                else              { snprintf(buff, size, "0x%02X", val); }
            }
            else if (fmt == ATOM_FLAG_IS_HEXIDECIMAL) { snprintf(buff, size, "0x%X", (unsigned)val); }
            else                                      { snprintf(buff, size, "%u", val); }
            break;
        }
        case ATOM_TYPE_INT:
        {
            int32_t val = ATOM_VAL(atom, int32_t, i);

            uint32_t mag = (val < 0) ? (uint32_t)(-(int64_t)val) : (uint32_t)val;
            const char* sign = (val < 0) ? "-" : "";

            if (fmt == ATOM_FLAG_IS_BINARY) 
            {
                char bin[33];
                snprintf(buff, size, "%s0b%s", sign, to_binary_trim(mag, bin));
            }
            else if (fmt == ATOM_FLAG_IS_HEXIDECIMAL) 
            {
                snprintf(buff, size, "%s0x%X", sign, (unsigned)mag);
            }
            else { snprintf(buff, size, "%" PRId32 "", val); }
            break;
        }
        case ATOM_TYPE_UINT:
        {
            uint32_t val = ATOM_VAL(atom, uint32_t, ui);

            if (fmt == ATOM_FLAG_IS_BINARY) 
            {
                char bin[33];
                snprintf(buff, size, "0b%s", to_binary_trim(val, bin));
            }
            else if (fmt == ATOM_FLAG_IS_HEXIDECIMAL) { snprintf(buff, size, "0x%X", (unsigned)val); }
            else                                      { snprintf(buff, size, "%u", (unsigned)val); }
            break;
        }
        case ATOM_TYPE_FLOAT:
        {
            FLOAT_TYPE val = ATOM_VAL(atom, FLOAT_TYPE, f);

            snprintf(buff, size, "%g", (double)val);
            break;
        }
        case ATOM_TYPE_WORD:
        {
            snprintf(buff, size, (plain) ? "%s" : "\"%s\"", atom->data.word);
            break;
        }
        case ATOM_TYPE_COMMAND:
        {
            volatile command_t* cmd = atom->data.ptr;
            if (cmd) { snprintf(buff, size, "%s", cmd->name); }
            else     { if (!plain) { snprintf(buff, size, "UNDEF"); } }
            break;
        }
        case ATOM_TYPE_VARIABLE:
        {
            volatile variable_t* var = atom->data.ptr;
            if (var) { snprintf(buff, size, "%s", var->name); }
            else     { if (!plain) { snprintf(buff, size, "UNDEF"); } }
            break;
        }
        case ATOM_TYPE_STRING:
        {
            volatile const char* str = atom->data.ptr;
            if (str) { snprintf(buff, size, (plain) ? "%s" : "\"%s\"", str); }
            break;
        }
        case ATOM_TYPE_RSTRING:
        {
            if (atom->data.rptr)
            {
                const char* str = RLINK_PTR(atom, atom->data.rptr);
                if (str) { snprintf(buff, size, (plain) ? "%s" : "\"%s\"", str); }
                else     { if (!plain) { snprintf(buff, size, "\"???\""); } }
            }
            break;
        }
        case ATOM_TYPE_PUNCT:
        {
            snprintf(buff, size, "%s", atom->data.word);
            break;
        }
        case ATOM_TYPE_LIST:
        {
            const atom_list_t *list = &atom->data.list;

            snprintf(buff, size, "[");

            size_t len = strlen(buff); buff += len; size -= len; 

            for (const atom_t* expr = list->head; expr && size > 1; expr = RLINK_NEXT(expr))
            {
                print_atom_helper(expr, &buff, &size, plain);

                if (expr->next && size > 2) { *buff++ = ','; *buff++ = ' '; *buff = 0; size -= 2; }
            }

            if (size > 1) { *buff++ = ']'; *buff = 0; --size; }

            *buffp = buff; *sizep = size;
            return;
        }
        default: { if (!plain) { snprintf(buff, size, "?%d?", atom->type); break; } }
    }

    size_t len = strlen(buff);

    *buffp = buff + len; *sizep = size - len;
}


PUBLIC void Print_Atom(const atom_t* atom, char** buffp, size_t* sizep, bool plain)
{
    if (atom && !(atom->type == ATOM_TYPE_WORD && !atom->data.word[0]) && buffp && sizep && *sizep >= 1)
    {
        print_atom_helper(atom, buffp, sizep, plain);
    }
}


PUBLIC void Print_Atom_List(const atom_t* atom, char** buffp, size_t* sizep, bool plain)
{
    if (buffp && sizep && *sizep >= 1)
    {
        while (atom && !(atom->type == ATOM_TYPE_WORD && !atom->data.word[0]))
        {
            print_atom_helper(atom, buffp, sizep, plain);

            if (*sizep > 1) { **buffp = '\n';  (*buffp)++;  **buffp = 0;  (*sizep)--; }

            atom = RLINK_NEXT(atom);
        }
    }
}


// ----------------------------
// Expression Dump Utilities
// ----------------------------

PRIVATE void dump_indent(int depth, char** buffp, size_t* buffsizep)
{
    BUFF_WRITER_START(buffp, buffsizep);
    while (depth-- > 0) BUFF_WRITER_PRINTF("\t");
    BUFF_WRITER_END(buffp, buffsizep);
}


PRIVATE void dump_atom(const atom_t *atom, int depth, char** buffp, size_t* buffsizep)
{
    dump_indent(depth, buffp, buffsizep);

    BUFF_WRITER_START(buffp, buffsizep);

    if (atom)
    {
        switch (atom->type)
        {
            case ATOM_TYPE_BOOL:     { snprintf(buff, buffsize, "BOOL(%s)\n",    (atom->data.i) ? "1" : "0");              break; }
            case ATOM_TYPE_INT:      { snprintf(buff, buffsize, "INT(%d)\n",     (int)atom->data.i);                       break; }
            case ATOM_TYPE_UINT:     { snprintf(buff, buffsize, "UINT(%u)\n",    (unsigned int)atom->data.ui);             break; }
            case ATOM_TYPE_FLOAT:    { snprintf(buff, buffsize, "FLOAT(%f)\n",   (FLOAT_TYPE)atom->data.f);                break; }
            case ATOM_TYPE_BYTE:     { snprintf(buff, buffsize, "BYTE(%u)\n",    (unsigned int)atom->data.b);              break; }
            case ATOM_TYPE_WORD:     { snprintf(buff, buffsize, "WORD(%s)\n",    atom->data.word);                         break; }
            case ATOM_TYPE_PUNCT:    { snprintf(buff, buffsize, "PUNCT(%s)\n",   atom->data.word);                         break; }
            case ATOM_TYPE_OPERATOR: { snprintf(buff, buffsize, "OP(%s)\n",      Operator_String(atom->data.op));          break; }
            case ATOM_TYPE_RSTRING:  { snprintf(buff, buffsize, "RSTRING(%s)\n", (char*)RLINK_PTR(atom, atom->data.rptr)); break; }
            case ATOM_TYPE_STRING:   { snprintf(buff, buffsize, "STRING(%s)\n",  (const char*)atom->data.ptr);             break; }
            case ATOM_TYPE_LIST:     { snprintf(buff, buffsize, "LIST\n");                                                 break; }
            case ATOM_TYPE_COMMAND: 
            {
                volatile command_t* cmd = atom->data.ptr;
                snprintf(buff, buffsize, "CMD(%s)\n", (cmd) ? cmd->name : "???");
                break;
            }
            default: { snprintf(buff, buffsize, "ATOM(type=%d)\n", atom->type); break; }
        }
    }
    else { snprintf(buff, buffsize, "(null atom)\n"); }

    BUFF_WRITER_ADVANCE();

    BUFF_WRITER_END(buffp, buffsizep);
}


PRIVATE void dump_expr(const atom_t* expr, int depth, char** buffp, size_t* buffsizep);

// Dump a node that may be an atom_t* or an expression_t* (or NULL).
//
PRIVATE void dump_child(const char* label, const atom_t* child, const char* null_label, int depth, char** buffp, size_t* buffsizep)
{
    if (label) { BUFF_WRITER_WRITE(buffp, buffsizep, "%s", label); }

    if (!child)
    {
        dump_indent(depth, buffp, buffsizep);
        BUFF_WRITER_WRITE(buffp, buffsizep, "%s\n", null_label);     // e.g. unary minus has lhs == NULL
    }
    else if (child->type == ATOM_TYPE_COMMAND || child->type == ATOM_TYPE_EXPRESSION)
         { dump_expr(child, depth, buffp, buffsizep); }

    else { dump_atom(child, depth, buffp, buffsizep); }
}


PRIVATE void dump_expr(const atom_t* node, int depth, char** buffp, size_t* buffsizep)
{
    if (!node)
    {
        dump_indent(depth, buffp, buffsizep);
        BUFF_WRITER_WRITE(buffp, buffsizep, "(null expr)\n");
        return;
    }

    if (node->type == ATOM_TYPE_COMMAND)                    // parsed call
    {
        dump_atom(node, depth, buffp, buffsizep);           // prints CMD(name)

        for (const atom_t* arg = node->data.call.args; arg; arg = RLINK_NEXT(arg))
        {
            dump_child("CMD:", arg, "(null arg)", depth + 1, buffp, buffsizep);
        }
    }
    // else if (node->type == ATOM_TYPE_EXPRESSION && node->data.expr.op == OP_LIST)
    // {
    //     dump_indent(depth, buffp, buffsizep);
    //     BUFF_WRITER_WRITE(buffp, buffsizep, "LIST\n");

    //     for (const atom_t* item = node->data.list.head; item; item = RLINK_NEXT(item))
    //     {
    //         dump_child("LST:", item, "(null)", depth + 1, buffp, buffsizep);
    //     }
    // }
    else if (node->type == ATOM_TYPE_EXPRESSION)            // unary / binary operator
    {
        dump_indent(depth, buffp, buffsizep);
        BUFF_WRITER_WRITE(buffp, buffsizep, "EXPR(op=%s)\n", Operator_String(node->data.expr.op));

        dump_child("LHS:", node->data.expr.lhs, "(null lhs)", depth + 1, buffp, buffsizep);
        dump_child("RHS:", node->data.expr.rhs, "(null rhs)", depth + 1, buffp, buffsizep);
    }
    else { dump_atom(node, depth, buffp, buffsizep); }      // bare atom handed in directly
}


PUBLIC void Dump_Expr(const atom_t* expr, char** buffp, size_t* buffsizep)
{
    BUFF_WRITER_WRITE(buffp, buffsizep, "=== Expression Dump ===\n");

    dump_child("EXPR:", (const atom_t *)expr, "(null expr)", 0, buffp, buffsizep);     // may be a bare atom

    BUFF_WRITER_WRITE(buffp, buffsizep, "=======================\n");
}

#define EACH_ITEM_IN(type, list, head) type list = (head); (list); (list) = RLINK_NEXT((list))


PUBLIC void Dump_Expr_List(const atom_t* list, char** buffp, size_t* buffsizep)
{
    for (EACH_ITEM_IN(const atom_t*, atom, list))
//    for (const atom_t* expr = list; expr; expr = RLINK_NEXT(expr))
    {
        Dump_Expr(atom, buffp, buffsizep);
    }
}


// End of file: print.c
