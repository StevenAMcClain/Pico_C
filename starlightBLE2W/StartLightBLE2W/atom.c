// File: atom.c

#include "common.h"
#include "atom.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "cmd_table.h"
#include "error.h"
#include "lexer.h"
#include "operator.h"
#include "variables.h"


// ============================================================================
//  BASIC UTILITIES
// ============================================================================

PUBLIC void Atom_Zap(atom_t* atom)
{
    if (atom) { memset(atom, 0, sizeof(atom_t)); }
}


PUBLIC void Atom_Shallow_Copy(atom_t* dest, const atom_t* src)
{
    if (src && dest) { memcpy(dest, src, sizeof(atom_t)); }
}


// ============================================================================
//  SAFE CHECKERS + CONVERTERS
// ============================================================================

PRIVATE bool check_binary(const uint8_t* ptr)
{
    if (*ptr == 0) { return POST_ERROR("0b is not valid binary!"); }

    while (*ptr)
    {
        if (*ptr != '0' && *ptr != '1') { return POST_ERROR("invalid binary character '%c'!", *ptr); }
        ptr++;
    }
    return true;
}


PRIVATE bool convert_binary(const uint8_t* ptr, uint32_t* out)
{
    uint32_t val = 0;

    while (*ptr)
    {
        if (val > (UINT32_MAX >> 1)) { return POST_ERROR("binary literal overflow!"); }

        val = (val << 1) | (*ptr - '0');
        ptr++;
    }

    *out = val;
    return true;
}


PRIVATE bool check_hex(const uint8_t* ptr)
{
    if (*ptr == 0) { return POST_ERROR("0x is not valid hex!"); }

    while (*ptr)
    {
        uint8_t ch = tolower(*ptr);
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
        {
            return POST_ERROR("invalid hex character '%c'!", *ptr);
        }
        ptr++;
    }
    return true;
}


PRIVATE bool convert_hex(const uint8_t* ptr, uint32_t* out)
{
    uint32_t val = 0;

    while (*ptr)
    {
        uint8_t ch = tolower(*ptr);
        uint32_t digit = (ch <= '9') ? (ch - '0') : (ch - 'a' + 10);

        if (val > (UINT32_MAX >> 4)) { return POST_ERROR("hex literal overflow!"); }

        val = (val << 4) | digit;
        ptr++;
    }

    *out = val;
    return true;
}


PRIVATE bool check_decimal(const uint8_t* ptr)
{
    if (*ptr == 0) { return POST_ERROR("empty string!"); }

    while (*ptr)
    {
        if (*ptr < '0' || *ptr > '9') { return POST_ERROR("invalid decimal character '%c'!", *ptr); }
        ptr++;
    }
    return true;
}


PRIVATE bool convert_decimal(const uint8_t* ptr, uint32_t* out)
{
    uint32_t val = 0;

    while (*ptr)
    {
        uint32_t digit = *ptr - '0';

        if (   val >  UINT32_MAX / 10 
           || (val == UINT32_MAX / 10 && digit > UINT32_MAX % 10))
        {
            return POST_ERROR("decimal literal overflow!");
        }

        val = val * 10 + digit;
        ptr++;
    }

    *out = val;

    return true;
}


// ============================================================================
//  FLOAT CHECK + CONVERT
//
//  Accepted forms:
//
//      [sign] digits "." [digits] [exponent]
//      [sign] digits exponent
//
//  where:
//
//      sign     := '+' | '-'
//      exponent := ('e' | 'E') [sign] digits
//
//  At least one mantissa digit is required.
//  A float must contain either a decimal point or an exponent;
//  otherwise it is treated as an integer.
//
PRIVATE bool check_float(const char* s)
{
    const char* p = s;
    size_t digits = 0;
    bool has_dot = false, has_exp = false;

    if (*p == '-' || *p == '+') { ++p; }

    if (p[0] == '0' && (tolower((unsigned char)p[1]) == 'x' ||
                        tolower((unsigned char)p[1]) == 'b')) { return false; }   // hex / binary

    while (isdigit((unsigned char)*p)) { ++p; ++digits; }

    if (*p == '.')
    {
        ++p;
        has_dot = true;
        while (isdigit((unsigned char)*p)) { ++p; ++digits; }
    }

    if (digits == 0) { return false; }

    if (*p == 'e' || *p == 'E')
    {
        ++p;
        has_exp = true;
        if (*p == '+' || *p == '-') { ++p; }
        if (!isdigit((unsigned char)*p)) { return false; }
        while (isdigit((unsigned char)*p)) { ++p; }
    }

    return (has_dot || has_exp) && *p == '\0';
}


PRIVATE bool convert_float(const char* s, FLOAT_TYPE* out)
{
    char* end;

    errno = 0;
    double val = strtod(s, &end);

    if (end == s || *end != '\0' || errno == ERANGE || !isfinite(val))
    {
        return POST_ERROR("float literal out of range!");
    }

    *out = (FLOAT_TYPE)val;

    return true;
}


PRIVATE bool convert_octal(const uint8_t* ptr, uint32_t* out)
{
    uint32_t val = 0;

    while (*ptr)
    {
        if (*ptr < '0' || *ptr > '7')
        {
            return POST_ERROR("invalid octal character '%c'!", *ptr);
        }

        uint32_t digit = *ptr - '0';

        if (val > (UINT32_MAX >> 3))
        {
            return POST_ERROR("octal literal overflow!");
        }

        val = (val << 3) | digit;
        ptr++;
    }
    *out = val;

    return true;
}


// ============================================================================
//  PROMOTION LOGIC
// ============================================================================

PRIVATE void atom_set_val(atom_t* atom, uint32_t val, atom_flag_t flags)
{
    atom->flags = flags;

    if (val <= 0xFF)
    {
        atom->type = ATOM_TYPE_BYTE;
        atom->data.ui = 0;
        atom->data.b = val;
    }
    else
    {
        atom->type = ATOM_TYPE_UINT;
        atom->data.ui = val;
    }
}


// Parses an unsigned Int/Hex/Bin magnitude (no sign, no float, no char
// literal). Used both for plain unsigned literals and for the magnitude
// that follows an explicit Sign. Returns false (with an error already
// posted via check_*) if the text isn't a valid literal of any of these
// forms.
PRIVATE bool parse_unsigned_magnitude(const char* s, uint32_t* out, atom_flag_t* flag_out)
{
    if (s[0] == '0' && tolower((unsigned char)s[1]) == 'b')
    {
        *flag_out = ATOM_FLAG_IS_BINARY;
        return check_binary((const uint8_t*)s + 2) &&
               convert_binary((const uint8_t*)s + 2, out);
    }

    if (s[0] == '0' && tolower((unsigned char)s[1]) == 'x')
    {
        *flag_out = ATOM_FLAG_IS_HEXIDECIMAL;
        return check_hex((const uint8_t*)s + 2) &&
               convert_hex((const uint8_t*)s + 2, out);
    }

    if (s[0] == '0' && s[1] != '\0')            // 0NNN: octal (0b / 0x were handled above)
    {
        *flag_out = 0;
        return convert_octal((const uint8_t*)s + 1, out);
    }
    
    *flag_out = 0;
    return check_decimal((const uint8_t*)s) &&
           convert_decimal((const uint8_t*)s, out);
}


PRIVATE void process_word(atom_t* atom)
{
    if (atom->type == ATOM_TYPE_RSTRING || atom->type == ATOM_TYPE_STRING) { return; }
  
    char* chp = atom->data.word;
    operator_t op = Operator_Match(chp);

    if (op)
    {
        atom->type = ATOM_TYPE_OPERATOR;
        atom->flags = ATOM_FLAG_NONE;        
        atom->data.op = op;
    }
    else if (*chp && strchr("+-*/%<>=!&|^~?:", *chp))
    {
        POST_ERROR("unsupported operator '%s'", chp);       // stays a WORD; the line is blocked by the error check
    }
    else
    {
        if (Is_Punctuation_Char(*chp))
        {
            atom->type = ATOM_TYPE_PUNCT;
            atom->flags = ATOM_FLAG_NONE;
        }
        else
        {
            command_t* cmd = Command_Find_Name(&Command_Table, atom->data.word);

            if (cmd)
            {
                atom->type = ATOM_TYPE_COMMAND;
                atom->flags = ATOM_FLAG_IS_POINTER;
                atom->data.ptr = cmd;
            }
            else
            {
                variable_t* var = Var_Get(atom->data.word);

                if (var)
                {
                    atom_t* vatom = &var->atom;

                    atom->type = ATOM_TYPE_VARIABLE;
                    atom->flags = vatom->flags;
                    atom->data.ptr = var;
                }
                else
                {
                    atom->type = ATOM_TYPE_WORD;       // WORD (fallback)
                    atom->flags = ATOM_FLAG_NONE;
                }
            }
        }
    }
}


PRIVATE void atom_promote(atom_t* atom)
{
    if (!atom) { return; }

    if (atom->type != ATOM_TYPE_WORD) { return; }    // Only WORD atoms contain unpromoted text.

    char* word = atom->data.word;
    size_t len = strlen(word);

    if (len == 0) { return; }

    if (check_float(word))
    {
        FLOAT_TYPE value;

        if (convert_float(word, &value))
        {
            atom->type = ATOM_TYPE_FLOAT;
            atom->flags = ATOM_FLAG_NONE;
            atom->data.f = value;
        }
        return;
    }

    char ch = word[0];
    bool has_sign = (ch == '-' || ch == '+');
    bool negative = (ch == '-');

    const char* rest = has_sign ? word + 1 : word;

    // ---------------------------------------------------------
    // A standalone '+' or '-' is an operator.
    // ---------------------------------------------------------
    if (has_sign && *rest == '\0')
    {
        atom->type = ATOM_TYPE_OPERATOR;
        atom->flags = ATOM_FLAG_NONE;
        atom->data.op = negative ? OP_SUB : OP_ADD;
        return;
    }

    // A sign followed by anything other than a digit is an operator word (+=, ->, ...).
    if (has_sign && !isdigit((unsigned char)*rest))
    {
        process_word(atom);
        return;
    }

    // ---------------------------------------------------------
    // CHARACTER LITERAL: 'A'
    // Store the byte AFTER setting the type and flag. Do not
    // clear data.ui afterward, because it overlaps data.b.
    // ---------------------------------------------------------
    if (  !has_sign
       && ch == '\''
       && len == 3
       && word[2] == '\'')
    {
        atom->type = ATOM_TYPE_BYTE;
        atom->flags = ATOM_FLAG_IS_CHARACTER;
        atom->data.ui = 0;
        atom->data.b = (uint8_t)word[1];
        return;
    }

    // ---------------------------------------------------------
    // SIGNED INTEGER
    // Supports decimal, hexadecimal and binary magnitudes.
    // ---------------------------------------------------------
    if (has_sign)
    {
        uint32_t magnitude;
        atom_flag_t format_flag;

        if (!parse_unsigned_magnitude(rest, &magnitude, &format_flag))
        {
//            return process_word(atom);   // Does this work????

            process_word(atom);
            return;
        }

        // INT32_MIN has a magnitude of INT32_MAX + 1.
        // Positive signed values cannot exceed INT32_MAX.
        const uint32_t limit = negative
                             ? (uint32_t)INT32_MAX + 1u
                             : (uint32_t)INT32_MAX;

        if (magnitude > limit)
        {
            POST_ERROR("signed value too large!");
            return;
        }

        atom->type = ATOM_TYPE_INT;
        atom->flags = format_flag;

        if (negative)    // Avoid negating INT32_MIN or converting its magnitude
        {                // to int32_t before the special case is handled.

            if (magnitude == (uint32_t)INT32_MAX + 1u) { atom->data.i = INT32_MIN; }
            else                                       { atom->data.i = -(int32_t)magnitude; }
        }
        else                                           { atom->data.i = (int32_t)magnitude; }
        return;
    }

    // ---------------------------------------------------------
    // Unsigned zero is represented as BYTE.
    // ---------------------------------------------------------
    if (word[0] == '0' && word[1] == '\0')
    {
        atom->type = ATOM_TYPE_BYTE;
        atom->flags = ATOM_FLAG_NONE;
        atom->data.ui = 0;
        atom->data.b = 0;
        return;
    }

    // ---------------------------------------------------------
    // UNSIGNED INTEGER
    // Decimal, hexadecimal and binary.
    // ---------------------------------------------------------
    if (isdigit((unsigned char)ch))
    {
        uint32_t value;
        atom_flag_t format_flag;

        if (parse_unsigned_magnitude(word, &value, &format_flag))
        {
            atom_set_val(atom, value, format_flag);
        }

        return;
    }

    // ---------------------------------------------------------
    // Not a numeric or character literal: operator, punctuation,
    // command, variable, or ordinary word.
    // ---------------------------------------------------------
    process_word(atom);
}


PUBLIC void Atom_Promote_List(atom_t* atom)
{
    while (atom && !(atom->type == ATOM_TYPE_WORD && !atom->data.word[0]))
    {
        atom_promote(atom);
        atom = RLINK_NEXT(atom);
    }
}


//-----------------------------------------------------------------------------------
PUBLIC bool Atom_Is_Number(const atom_t* atom)
{
    if (!atom) { return false; }

    switch (atom->type)
    {
        case ATOM_TYPE_BOOL:
        case ATOM_TYPE_BYTE:
        case ATOM_TYPE_INT:
        case ATOM_TYPE_UINT:
        case ATOM_TYPE_FLOAT:
            return true;

        default:
            return false;
    }
}


// Numeric helpers: coerce to FLOAT for mixed arithmetic.
// (A NULL atom is a failure that was already reported: stay quiet.)
//
PUBLIC bool Atom_To_Number(const atom_t* atom, FLOAT_TYPE* value)
{
    if (!atom || !value) return false;

    switch (atom->type)
    {
        case ATOM_TYPE_BOOL:  { *value =             ATOM_VAL(atom, int32_t, b) ? 1 : 0;  break; }
        case ATOM_TYPE_INT:   { *value = (FLOAT_TYPE)ATOM_VAL(atom, int32_t,    i); break; }
        case ATOM_TYPE_UINT:  { *value = (FLOAT_TYPE)ATOM_VAL(atom, uint32_t,  ui); break; }
        case ATOM_TYPE_BYTE:  { *value = (FLOAT_TYPE)ATOM_VAL(atom, uint8_t,    b); break; }
        case ATOM_TYPE_FLOAT: { *value = (FLOAT_TYPE)ATOM_VAL(atom, FLOAT_TYPE, f); break; }
        default: { return POST_ERROR("Non-numeric atom in arithmetic"); }
    }
    return true;
}


// Boolean helper: treat non-zero as true.
//
PUBLIC bool Atom_To_Bool(const atom_t* atom, uint32_t* out)
{
    if (!atom || !out) { return false; }

    switch (atom->type)
    {
        case ATOM_TYPE_BOOL:  { *out = ATOM_VAL(atom, int32_t,    b)  != 0;   return true; }
        case ATOM_TYPE_INT:   { *out = ATOM_VAL(atom, int32_t,    i)  != 0;   return true; }
        case ATOM_TYPE_UINT:  { *out = ATOM_VAL(atom, uint32_t,   ui) != 0;   return true; }
        case ATOM_TYPE_BYTE:  { *out = ATOM_VAL(atom, uint8_t,    b)  != 0;   return true; }
        case ATOM_TYPE_FLOAT: { *out = ATOM_VAL(atom, FLOAT_TYPE, f)  != 0.0; return true; }

        default: { return POST_ERROR("boolean operand required"); }
    }
}

// Integer helpers.  BYTE is what small literals are, so it is always accepted.
//
PUBLIC bool Atom_To_U32(const atom_t* atom, uint32_t* out)
{
    if (!atom || !out) { return false; }

    switch (atom->type)
    {
        case ATOM_TYPE_BOOL: { *out = (uint32_t)ATOM_VAL(atom,  int32_t,  b); break; }
        case ATOM_TYPE_BYTE: { *out = (uint32_t)ATOM_VAL(atom,  uint8_t,  b); break; }
        case ATOM_TYPE_UINT: { *out = (uint32_t)ATOM_VAL(atom, uint32_t, ui); break; }
        case ATOM_TYPE_INT:  { *out = (uint32_t)ATOM_VAL(atom,  int32_t,  i); break; }

        default: { return POST_ERROR("integer operand required"); }
    }
    return true;
}


PUBLIC bool Atom_To_I64(const atom_t* atom, int64_t* out)
{
    if (!atom || !out) { return false; }

    switch (atom->type)
    {
        case ATOM_TYPE_BOOL: { *out = (int64_t)ATOM_VAL(atom,  int32_t,  b); break; }
        case ATOM_TYPE_BYTE: { *out = (int64_t)ATOM_VAL(atom,  uint8_t,  b); break; }
        case ATOM_TYPE_UINT: { *out = (int64_t)ATOM_VAL(atom, uint32_t, ui); break; }
        case ATOM_TYPE_INT:  { *out = (int64_t)ATOM_VAL(atom,  int32_t,  i); break; }

        default: { return POST_ERROR("integer operand required"); }
    }
    return true;
}



PUBLIC bool Atom_Equals(const atom_t* a, const atom_t* b)
{
    if (!a || !b) { return false; }

    if (Atom_Is_Number(a) && Atom_Is_Number(b))
    {
        FLOAT_TYPE av;
        FLOAT_TYPE bv;

        if (!Atom_To_Number(a, &av) ||
            !Atom_To_Number(b, &bv))
        {
            return false;
        }

        return FLOAT_COMPARE(av, bv);
    }

    if (a->type != b->type) { return false; }

    char* as = 0;
    char* bs = 0;

    switch (a->type)
    {
        case ATOM_TYPE_UNDEFINED:
            return true;

        case ATOM_TYPE_BOOL: return a->data.i  == b->data.i;
        case ATOM_TYPE_BYTE: return a->data.b  == b->data.b;
        case ATOM_TYPE_INT:  return a->data.i  == b->data.i;
        case ATOM_TYPE_UINT: return a->data.ui == b->data.ui;

        case ATOM_TYPE_FLOAT:
            return FLOAT_COMPARE(a->data.f, b->data.f);

        case ATOM_TYPE_WORD:
            as = (char*)a->data.word;
            bs = (char*)b->data.word;

            if (!as || !bs) { return as == bs; }
            return strcmp(as, bs) == 0;

        case ATOM_TYPE_RSTRING:
            as = (char*)RLINK_PTR(a, a->data.rptr);
            bs = (char*)RLINK_PTR(b, b->data.rptr);

            if (!as || !bs) { return as == bs; }
            return strcmp(as, bs) == 0;

        case ATOM_TYPE_STRING:
            as = a->data.ptr;
            bs = b->data.ptr;

            if (!as || !bs) { return as == bs; }
            return strcmp(as, bs) == 0;            

        case ATOM_TYPE_OPERATOR:
            return a->data.op == b->data.op;

        case ATOM_TYPE_PUNCT:
            return a->data.word[0] == b->data.word[0];

        case ATOM_TYPE_ATOM:
        case ATOM_TYPE_VARIABLE:
        case ATOM_TYPE_LIST:
        case ATOM_TYPE_COMMAND:
        case ATOM_TYPE_EXPRESSION:
            return a->data.ptr == b->data.ptr;

        default:
            return false;
    }
}

// End of file: atom.c
