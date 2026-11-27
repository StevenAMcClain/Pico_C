// File: parser.c

#include "common.h"
#include "parser.h"

#include <stdio.h>

#include "atom.h"
#include "command.h"
#include "error.h"
#include "lexer.h"
#include "operator.h"

// ----------------------------
// Pratt Parser using atom_t directly
// ----------------------------

#define PREC_UNARY      90      // Binds tighter than * and /.
#define MAX_PARSE_DEPTH 16      // Recursion limit (protects the MCU stack).

typedef struct
{
    context_t* context;  // heap, etc.
    atom_t *cur;         // current atom
    int depth;           // current recursion depth (shared by nested lists)

} parser_t;


PRIVATE atom_t *parse_expression(parser_t *P, int rbp);

// ----------------------------
// Utility: advance atom
//
PRIVATE void next_atom(parser_t *P)
{
    if (P->cur && P->cur->next) { P->cur = RLINK_NEXT(P->cur); }
    else                        { P->cur = NULL; }
}


PRIVATE atom_t* new_expr(parser_t* p, operator_t op)
{
    atom_t* n = Atom_New(p->context);
    if (!n) { return NULL; }

    n->type         = ATOM_TYPE_EXPRESSION;
    n->data.expr.op = op;
    return n;
}


PRIVATE bool is_punct(const atom_t* a, char c)
{
    return a && a->type == ATOM_TYPE_PUNCT && a->data.word[0] == c;
}


PRIVATE bool append_command_arg(
    parser_t *P,
    atom_t *cmd,
    atom_t **tail,
    atom_t *arg)
{
    UNUSED(P);

    arg->next = 0;

    if (*tail) { (*tail)->next = PTR_RLINK(*tail, arg); }
    else       { cmd->data.call.args = arg; }

    *tail = arg;

    return true;
}


PRIVATE bool ends_expression(const atom_t *a)
{
    if (!a) { return true; }

    if (a->type != ATOM_TYPE_PUNCT) { return false; }

    char c = a->data.word[0];

    return c == ';' 
        || c == ')' 
        || c == ']' 
        || c == '\n';
}


PRIVATE bool command_args(parser_t* parser, atom_t* atom, const command_t* cmd)
{
    atom_t *tail = NIL;
    size_t argc = 0;

    atom->data.call.args = NIL;

    // Required arguments
    //
    while (argc < cmd->min_arity)
    {
        atom_t *arg = parse_expression(parser, 0);

        if (!arg) { return POST_ERROR("'%s' requires at least %u arguments", cmd->name, cmd->min_arity); }

        if (!append_command_arg(parser, atom, &tail, arg)) { return false; }

        argc++;
    }

    // Optional arguments
    //
    while (argc < cmd->max_arity)
    {
        if (!parser->cur || ends_expression(parser->cur)) { break; }

        // Variadic commands stop when another command starts.
        //
        if (cmd->max_arity == COMMAND_ARITY_VARIADIC &&
            parser->cur->type == ATOM_TYPE_COMMAND)
        {
            break;
        }

        atom_t *arg = parse_expression(parser, 0);

        if (!arg) { break; }    // while (argc < cmd->max_arity)

        if (!append_command_arg(parser, atom, &tail, arg)) { return false; }

        argc++;
    }

    return true;
}


PRIVATE bool is_list_sep(const atom_t *a)
{
    return a
        && a->type == ATOM_TYPE_OPERATOR
        && a->data.op == OP_LIST;
}


PRIVATE atom_t* parse_list_literal(parser_t* parser)
{
    atom_t* atom = Atom_New(parser->context);

    if (!atom) { return NULL; }

    atom->type = ATOM_TYPE_LIST;

    // Empty list: []
    //
    if (is_punct(parser->cur, ']'))
    {
        next_atom(parser);
        return atom;
    }

    atom_t* tail = NULL;

    for (;;)
    {
        atom_t* item = parse_expression(parser, 0);

        if (!item) { return NULL; }

        item->next = 0;

        if (tail) { tail->next = PTR_RLINK(tail, item); }
        else      { atom->data.list.head = item; }

        tail = item;

        if (is_punct(parser->cur, ']'))        // End of list?
        {
            next_atom(parser);
            return atom;
        }

        if (!is_list_sep(parser->cur)) { return POST_ERROR("Expected ',' or ']'"); }   // Require comma

        next_atom(parser);   // consume comma

        // Reject trailing comma:
        // [a,b,]
        //
        if (is_punct(parser->cur, ']')) { return POST_ERROR("Trailing comma in list"); }
    }
}

// ----------------------------
// NUD: prefix / primary
//
// On entry P->cur already points PAST 'a' (parse_expression advanced).
//
PRIVATE atom_t *nud_atom(parser_t *parser, atom_t *atom)
{
    if (!atom) { return POST_ERROR("Unexpected end of expression"); }

    switch (atom->type)
    {
        case ATOM_TYPE_BOOL: case ATOM_TYPE_INT: case ATOM_TYPE_UINT: case ATOM_TYPE_FLOAT:
        case ATOM_TYPE_BYTE: case ATOM_TYPE_WORD: case ATOM_TYPE_RSTRING: case ATOM_TYPE_STRING:
        case ATOM_TYPE_VARIABLE:
        {
            return atom;
        }

        case ATOM_TYPE_OPERATOR:
        {
            operator_t op = OP_UNKNOWN;

            switch (atom->data.op)
            {
                case OP_SUB:                                // unary -
                case OP_NEG:    { op = OP_NEG; break; }
                case OP_ADD:                                // unary +
                case OP_POS:    { op = OP_POS; break; }
                case OP_NOT:    { op = OP_NOT; break; }     // !
                case OP_BITNOT: { op = OP_BITNOT; break; }  // ~
                default:        { break; }
            }

            if (op != OP_UNKNOWN)
            {
                atom_t *new_atom = new_expr(parser, op);

                if (!new_atom) { return NULL; }

                atom_t* result = parse_expression(parser, PREC_UNARY);

                new_atom->data.expr.rhs = result;

                return result ? new_atom : NULL;
            }
            break;
        }
        case ATOM_TYPE_PUNCT:
        {
            if (atom->data.word[0] == '(')     // parser->cur already points at first inner atom
            {
                atom_t *inner = parse_expression(parser, 0);

                if (!inner) return NULL;        // error already posted

                if (!is_punct(parser->cur, ')')) { return POST_ERROR("Missing ')'"); }

                next_atom(parser);              // consume ')'
                return inner;
            }
            else if (atom->data.word[0] == '[')
            {
                return parse_list_literal(parser);
            }
            break;
        }
        case ATOM_TYPE_COMMAND:
        {
            const command_t* cmd = atom->data.call.cmd;

            atom->data.call.args = NULL;
            atom->flags          = ATOM_FLAG_NONE;      // no longer a "pointer to a value" atom

            if (!command_args(parser, atom, cmd)) { return NULL; }

            return atom;
        }
        default: { return POST_ERROR("Unknown atom type: %d", atom->type); }
    }
    return POST_ERROR("Unexpected token in nud");
}


// ----------------------------
// LED: infix operator
//
PRIVATE atom_t *led(parser_t *parser, atom_t *op_atom, atom_t *left)
{
    operator_t op = op_atom->data.op;
    int lbp = Operator_Precedence(op);

    next_atom(parser);    // Skip operator token

    // -----------------------------
    //  Normal infix operators
    // -----------------------------
    atom_t *new_atom = new_expr(parser, op);

    if (!new_atom) return NULL;

    atom_expr_t* expr = &new_atom->data.expr;

    expr->lhs = left;

    // Assignment is right-associative: "a = b = 3" is "a = (b = 3)".
    // A lower right binding power lets the next '=' bind into the rhs.
    int rbp = Is_Assignment_Op(op) ? lbp - 1 : lbp;

    expr->rhs = parse_expression(parser, rbp);

    if (!expr->rhs) return NULL;

    return new_atom;
}


// ----------------------------
// Main Pratt expression parser
//
PRIVATE atom_t* parse_expression(parser_t* parser, int rbp)
{
    if (parser->depth >= MAX_PARSE_DEPTH) { return POST_ERROR("Expression nested too deeply"); }
    if (!parser->cur)                     { return POST_ERROR("Unexpected end of expression"); }

    atom_t* tok = parser->cur;
    next_atom(parser);                               // consume first atom

    ++parser->depth;

    // Parse the primary (nud)
    atom_t* left = nud_atom(parser, tok);

    // --- Standard Pratt infix loop (comma excluded)
    while (left && parser->cur)
    {
        if (ends_expression(parser->cur)) { break; }

        if (parser->cur->type != ATOM_TYPE_OPERATOR) { break; }

        operator_t op = parser->cur->data.op;

        if (op == OP_LIST) { break; }     // not an error, list parser consumes it

        int lbp = Operator_Precedence(op);

        if (lbp <= rbp) { break; }

        left = led(parser, parser->cur, left);
    }

    --parser->depth;

    return left;
}


// ----------------------------
// Entry point
// ----------------------------

PUBLIC atom_t* Expr_List_Build(context_t* context, atom_t* first_atom)
{
    parser_t parser = 
    {
        .context = context,
        .cur   = first_atom,
        .depth = 0
    };

    atom_t* result = NIL;
    atom_t* tail   = NIL;

    while (parser.cur)
    {
        if (is_punct(parser.cur, ';') || is_punct(parser.cur, '\n')) { next_atom(&parser); continue; }

        size_t serial = Error_Serial;
        atom_t* expr = parse_expression(&parser, 0);

        if (!expr)
        {
            // Stop at the first failure: don't resume on leftover tokens.
            if (Error_Serial == serial) { POST_ERROR("Parse failed"); }
            return NIL;
        }

        // A statement must end at ';' or end of line.
        if (parser.cur && (is_punct(parser.cur, ';') || is_punct(parser.cur, '\n')))
        {
            next_atom(&parser);
        }
        else if (parser.cur) 
        {
            return POST_ERROR("Expected ';' or end of line after expression");
        }

        expr->next = 0;

        if (tail) { tail->next = PTR_RLINK(tail, expr); }
        else      { result = expr; }

        tail = expr;
    }

    return result;
}


// End of file: parser.c
