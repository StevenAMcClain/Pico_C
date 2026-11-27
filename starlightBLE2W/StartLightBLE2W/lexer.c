// File: lexer.c
//

#include "common.h"
#include "lexer.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "atom.h"
#include "balloc.h"
#include "command.h"
#include "cmd_table.h"
#include "error.h"
#include "evaluator.h"
#include "parser.h"
#include "print.h"

#ifndef ATOM_MAX
#define ATOM_MAX 100                 // Max real tokens per line.
#endif
#define ATOM_LIMIT (ATOM_MAX + 1)   // +1: trailing empty atom is always present.


typedef enum lexer_state
{
    LEXSTATE_SKIP_WHITE,
    LEXSTATE_SKIP_LINE,
    LEXSTATE_READ_STRING,
    LEXSTATE_READ_STRING_ESCAPE,
    LEXSTATE_READ_STRING_HEX,
    LEXSTATE_READ_WORD,
    LEXSTATE_READ_OPERATOR,
    LEXSTATE_READ_NUMBER,
    LEXSTATE_COMMENT_LINE,
    LEXSTATE_BLOCK_COMMENT,
    LEXSTATE_BLOCK_COMMENT_STAR,
    LEXSTATE_READ_CHAR,             // after the opening '
    LEXSTATE_READ_CHAR_ESCAPE,      // after a backslash in 'c'
    LEXSTATE_READ_CHAR_END,         // expecting the closing '

} lexer_state_t;


typedef struct lexer
{
    lexer_state_t state;         // State for lexing.

    balloc_h heap;               // Heap for atoms and expressions.

    atom_t* first;               // Pointer to first atom.
    size_t  atom_count;          // Number of atoms in list.
    atom_t* atom;                // Points to current atom.
    atom_t* prev;                // Atom before the current one (NULL at start of line).    

    char*   atom_wordp;          // Pointer to next place in word buffer.
    size_t  atom_word_left;      // Number of character left in word buffer.

    bool    number_has_dot;
    bool    number_has_exponent;
    bool    number_exponent_digit;
    bool    number_exponent_sign;
    bool    number_signed;       // Word buffer starts with a '+' / '-' sign.

    bool    pending_line_splice;

    int     hex_count;           // Digits seen in current \xNN escape.
    int     hex_val;             // Value of current \xNN escape.

    bool    dead;                // Set when Lexer_Reset failed; cleared by a good reset.

    bool    string_open;         // True when string is read.

    char*   buff;                // Pointer to next byte to process.
    size_t  buff_left;           // Bytes left to process.

    process_f process;
//    void (*process)(balloc_h* heap);    // Called to process lexed atoms.

    size_t last_error_serial;

} lexer_t;

// ---

#define LEX_ERROR_SKIP(msg)   do { POST_ERROR(msg); lp->state = LEXSTATE_SKIP_LINE; } while (0)


PRIVATE void lexer_clear_atom(lexer_t* lp)
//
// Make the current (WORD) atom empty and clear number-scan flags.
{
    atom_t* atom = lp->atom;

    if (atom)
    {
        atom->type = ATOM_TYPE_WORD;
        atom->flags = ATOM_FLAG_NONE;
        atom->next = 0;

        lp->atom_word_left = sizeof(atom->data.word);
        lp->atom_wordp = atom->data.word;
       *lp->atom_wordp = 0;

        lp->number_has_dot = false;
        lp->number_has_exponent = false;
        lp->number_exponent_digit = false;
        lp->number_exponent_sign = false;
        lp->number_signed = false;
    }
}


PRIVATE bool lexer_commit_string(lexer_t* lp)
{
    if (!lp->string_open) { return true; }
    lp->string_open = false;
    const char* s = RLINK_PTR(lp->atom, lp->atom->data.rptr);
    size_t len = (strlen(s) + 1 + 7) & ~(size_t)7;     // keep 8-byte alignment
    Balloc_Release(lp->heap);
    return Balloc(lp->heap, len) != NULL;               // returns s: claims the bytes
}


PRIVATE bool lexer_next_atom(lexer_t* lp)
//
// Allocate the next atom.
{
    bool result = false;
    atom_t* last_atom = lp->atom;

    if (!lp->atom || lp->atom->type != ATOM_TYPE_WORD || lp->atom->data.word[0])
    {
        if (lexer_commit_string(lp))
        {
            if (lp->atom_count < ATOM_LIMIT)
            {
                atom_t* atom = Ballocz(lp->heap, sizeof(atom_t));

                if (atom)
                {
                    if (!lp->first)
                    {
                        lp->first = atom;
                    }

                    lp->atom = atom;
                    lp->prev = last_atom;

                    if (last_atom) 
                    {
                        last_atom->next = PTR_RLINK(last_atom, atom);         // Save relative link.
                    }
                    lexer_clear_atom(lp);
                    ++lp->atom_count;
                    result = true;
                }
            }
            else { POST_ERROR("TOO MANY ATOMS!"); }
        }
        else { POST_ERROR("Can't commit string!"); }
    }
    else  // current atom already empty, reuse silently.
    {
        lexer_clear_atom(lp);
        result = true; 
    }

    return result;
}


PRIVATE bool is_end_atom(const atom_t* a)
{
    return a->type == ATOM_TYPE_WORD && a->data.word[0] == '\0';
}


PUBLIC atom_t* Lexer_Trim_End(atom_t* first)
//
// Unlink the trailing empty atom.  Returns NULL if the line has no real atoms.
{
    if (!first || is_end_atom(first)) { return NULL; }      // Blank line.

    atom_t* atom = first;

    while (atom->next)
    {
        atom_t* next = RLINK_NEXT(atom);

        if (is_end_atom(next)) { atom->next = 0; break; }      // Cut the sentinel off.

        atom = next;
    }
    return first;
}


PRIVATE bool Lexer_Reset(lexer_h lh)
//
// Reset lexer state.
{
    bool result = false;

    if (lh)
    {
        lexer_t* lp = lh;

        lp->pending_line_splice = false;

        lp->dead = true;                        // Until proven otherwise.
        lp->string_open = false;

        Balloc_Clear(lp->heap);                 // Start with empty heap.
        lp->atom_count = 0;                     // No atoms in list.
        lp->atom = NULL;

        result = lexer_next_atom(lh);           // Allocate the first atom. 

        if (result)
        {
            lp->state = LEXSTATE_SKIP_WHITE;    // Reset lex state.
            lp->dead = false;
        }
        else { POST_ERROR("can't get next atom!"); }

        lp->last_error_serial = Error_Serial;   // Remember last error serial for this line.
    }

    if (!result) { POST_ERROR("Can't reset lexer!"); }

    return result;
}


PUBLIC lexer_h Lexer_Initialize(size_t heap_size, process_f process)
{
    if (heap_size)
    {
        lexer_t* lp = Ballocz(PSRAM_Balloc, sizeof(lexer_t));

        if (lp)
        {
            lp->process = process;

            balloc_h heap = Ballocz(PSRAM_Balloc, heap_size);

            if (heap) 
            {
                lp->heap = Balloc_Init(heap, heap_size);

                if (!Lexer_Reset(lp)) { lp = NULL; }       // TODO: leaks lp and lp->heap (needs a free API).
            }
            else { lp = NULL; }                            // TODO: leaks lp.
        }
        return lp;
    }
    return NULL;
}


PUBLIC balloc_h Lexer_Heap(lexer_h lh)
{
    return (lh) ? ((lexer_t*)lh)->heap : NIL;
}


PUBLIC bool Is_Punctuation_Char(char c)
{
    switch (c)
    {
        case '(': case ')': case '{': case '}': 
        case '[': case ']': case ';': case '#': case ',':
        { return true; }
    }
    return false;
}


PRIVATE bool is_hex_prefix(const char *s)
{
    return s[0] == '0' 
       && (s[1] == 'x' || s[1] == 'X');
}


PRIVATE bool is_binary_prefix(const char *s)
{
    return s[0] == '0' 
       && (s[1] == 'b' || s[1] == 'B');
}


PRIVATE int hex_digit_value(char c)
{
    if (isdigit((unsigned char)c)) { return c - '0'; }
    return (tolower((unsigned char)c) - 'a') + 10;
}


PRIVATE bool string_add_char(lexer_t* lp, char c)
{
    if (lp->atom_word_left > 1)
    {
        *lp->atom_wordp++ = c;
        *lp->atom_wordp   = 0;
        lp->atom_word_left--;
        return true;
    }
    return POST_ERROR("String too long!");
}


PRIVATE bool string_add_escaped(lexer_t* lp, int val)
//
// Add the result of a numeric escape.  NUL can't be stored in a C string.
{
    if (val == 0)
    {
        POST_ERROR("NUL ESCAPE NOT SUPPORTED IN STRING!");
        return true;                // Reported, but not fatal to the string.
    }
    return string_add_char(lp, (char)val);
}


// The digits of the number being scanned, past any leading sign.
PRIVATE const char* number_body(const lexer_t* lp)
{
    return lp->atom->data.word + (lp->number_signed ? 1 : 0);
}


// A '+' / '-' starts a signed literal only where an operand is expected:
// start of line, after an operator, after '(' ';' etc., or after a command.
// After a number, string, char literal, ')' or a non-command identifier it is
// the binary operator ("x -1" is x minus 1).
PRIVATE bool lexer_sign_allowed(const lexer_t* lp)
{
    const atom_t* p = lp->prev;

    if (!p)                           { return true; }
    if (p->type == ATOM_TYPE_RSTRING) { return false; }

    const char* w = p->data.word;
    char c = w[0];

    if (c == '\'')                               { return false; }   // char literal
    if (isdigit((unsigned char)c) || c == '.')   { return false; }   // number
    if (Is_Punctuation_Char(c))                  { return c != ')' && c != ']'; }

    if (c == '+' || c == '-')                    // a signed number is an operand
    {
        return !(isdigit((unsigned char)w[1]) || w[1] == '.');
    }
    if (Is_Operator_Char(c))                     { return true; }

    return Command_Find_Name(&Command_Table, w) != NULL;             // identifier
}


PRIVATE bool lexer_token_complete(lexer_t* lp)
//
// Called when input ends (LF or end of data) to validate whatever is half-scanned.
// Returns false (after posting an error) if the partial token is invalid.
{
    switch (lp->state)
    {
        case LEXSTATE_BLOCK_COMMENT:
        case LEXSTATE_BLOCK_COMMENT_STAR: { return POST_ERROR("UNTERMINATED BLOCK COMMENT!"); }

        case LEXSTATE_READ_STRING:
        case LEXSTATE_READ_STRING_ESCAPE:
        case LEXSTATE_READ_STRING_HEX:    { return POST_ERROR("UNTERMINATED STRING LITERAL!"); }

        case LEXSTATE_READ_CHAR:
        case LEXSTATE_READ_CHAR_ESCAPE:
        case LEXSTATE_READ_CHAR_END:      { return POST_ERROR("UNTERMINATED CHARACTER LITERAL!"); }

        case LEXSTATE_READ_NUMBER:
        {
            const char* w = number_body(lp);
//            char first = w[0];
            size_t len = (size_t)(lp->atom_wordp - w);

            if (len == 1 && w[0] == '.')
            {
                POST_ERROR("INVALID CHARACTER!");
                lexer_clear_atom(lp);
                return false;
            }
            if (len == 2 && is_hex_prefix(w))    { POST_ERROR("INVALID HEXADECIMAL NUMBER!"); return false; }
            if (len == 2 && is_binary_prefix(w)) { POST_ERROR("INVALID BINARY NUMBER!");      return false; }
            if (lp->number_has_exponent && !lp->number_exponent_digit)
            {
                return POST_ERROR("INVALID EXPONENT!");
            }
            break;
        }

        default: break;
    }
    return true;
}


PRIVATE bool lexer_process_character(lexer_t* lp)
{
    if (lp->pending_line_splice)
    {
        lp->pending_line_splice = false;

        if (lp->buff_left)
        {
            if (lp->buff[0] == '\n')
            {
                ++lp->buff;
                --lp->buff_left;
                return false;
            }

            if (lp->buff_left >= 2 &&
                lp->buff[0] == '\r' &&
                lp->buff[1] == '\n')
            {
                lp->buff += 2;
                lp->buff_left -= 2;
                return false;
            }
        }

        POST_ERROR("Backslash not followed by newline");
    }

    char ch = *lp->buff;

    // ------------------------------------------------------------
    if (ch == '\\')
    {
        if (lp->buff_left == 1)
        {
            lp->pending_line_splice = true;

            ++lp->buff;
            --lp->buff_left;

            return false;
        }

        if (lp->buff[1] == '\n')
        {
            lp->buff += 2;
            lp->buff_left -= 2;

            return false;
        }

        if (lp->buff_left >= 3 &&
            lp->buff[1] == '\r' &&
            lp->buff[2] == '\n')
        {
            lp->buff += 3;
            lp->buff_left -= 3;

            return false;
        }
    }

    char* bptr = lp->atom_wordp;
    size_t left = lp->atom_word_left;

    bool addchar = false;
    bool end_atom = false;
    bool advance = true;   // whether caller should advance buffer

    if ( (ch == ASCII_LF || ch == ASCII_CR)
      && lp->state != LEXSTATE_BLOCK_COMMENT
      && lp->state != LEXSTATE_BLOCK_COMMENT_STAR
      && lp->state != LEXSTATE_READ_STRING
      && lp->state != LEXSTATE_READ_STRING_ESCAPE
      && lp->state != LEXSTATE_READ_STRING_HEX)     // Handle LF as line terminator
    {
        if (lp->state != LEXSTATE_COMMENT_LINE)
        {
            (void)lexer_token_complete(lp);         // Report half-finished numbers.

            if (!lexer_next_atom(lp))  { POST_ERROR("Can't get next atom!"); }
        }
        lp->state = LEXSTATE_SKIP_WHITE;

        ++lp->buff;         // consume LF
        --lp->buff_left;

        if (lp->buff_left && ch == '\r' && *lp->buff == '\n')
        {
            ++lp->buff;
            --lp->buff_left;
        }

        return true;        // Process atom list.
    }

    switch (lp->state)
    {
        // ------------------------------------------------------------
        // SKIP WHITE
        // 
        case LEXSTATE_SKIP_WHITE:
        {
            if (ch == '"')
            {
                if (Balloc_Reserve(lp->heap))      // Reserve the rest of the heap.
                {
                    size_t room = Balloc_Left(lp->heap);

                    if (room > 1)                  // Need room for at least the null.
                    {
                        void* top = Balloc_Top(lp->heap);
                        lp->atom_wordp = top;
                       *lp->atom_wordp = 0;
                        lp->atom_word_left = room - 1;   // With room for null.

                        lp->atom->type = ATOM_TYPE_RSTRING;
                        lp->atom->flags = ATOM_FLAG_NONE;
                        lp->atom->data.rptr = PTR_RLINK(lp->atom, top);

                        lp->string_open = true;
                        lp->state = LEXSTATE_READ_STRING;
                    }
                    else 
                    { 
                        POST_ERROR("No room for string");
                        lp->state = LEXSTATE_SKIP_LINE;
                        Balloc_Release(lp->heap);
                    }
                }
                else 
                { 
                    POST_ERROR("Can't reserve");
                    lp->state = LEXSTATE_SKIP_LINE;
                }
                break;                              // advance is true: consume the opening quote.
            }

            // Note: '/' starts an operator; "//" and "/*" are recognised in READ_OPERATOR.
            if (isalpha((unsigned char)ch) || ch == '_') 
            {
                lp->state = LEXSTATE_READ_WORD;
                addchar = true;
            }
            else if (isdigit((unsigned char)ch)) 
            {
                lp->state = LEXSTATE_READ_NUMBER;
                addchar = true;
            }
            else if (ch == '.')                     // Must be followed by a digit (checked in READ_NUMBER).
            {
                lp->state = LEXSTATE_READ_NUMBER;
                lp->number_has_dot = true;
                addchar = true;
            }
            else if (ch == '\'')
            {
                lp->state = LEXSTATE_READ_CHAR;
                addchar = true;                     // keep the opening quote
            }
            else if (Is_Operator_Char(ch)) 
            {
                lp->state = LEXSTATE_READ_OPERATOR;
                addchar = true;
            }
            else if (Is_Punctuation_Char(ch)) 
            {
                addchar = true;
                end_atom = true;                    // Punctuation is always a single-char atom.
            }
            else if (!isspace((unsigned char)ch))       // Whitespace (incl. CR) is skipped silently.
            {
                POST_ERROR("INVALID CHARACTER!");
                lp->state = LEXSTATE_SKIP_LINE;
            }
            break;
        }

        // ------------------------------------------------------------
        // STRING
        // 
        case LEXSTATE_READ_STRING:
        {
            if (ch == '\\')
            {
                lp->state = LEXSTATE_READ_STRING_ESCAPE;
            }
            else if (ch == '"')
            {
                lp->state = LEXSTATE_SKIP_WHITE;
                end_atom  = true;
            }
            else if (ch == ASCII_LF)
            {
                // LF is illegal unless escaped
                POST_ERROR("LF is illegal unless escaped");
                lp->state = LEXSTATE_SKIP_WHITE;
                end_atom  = true;
                advance   = false;      // Leave LF for the line-terminator handler.
            }
            else { addchar = true; }
            break;
        }

        case LEXSTATE_READ_STRING_ESCAPE:
        {
            char esc = 0;

            lp->state = LEXSTATE_READ_STRING;

            switch (ch)
            {
                case 'n':      esc = '\n'; break;
                case 'r':      esc = '\r'; break;
                case 't':      esc = '\t'; break;
                case '\\':     esc = '\\'; break;
                case '"':      esc = '"';  break;
                case '\'':     esc = '\''; break;
                case '0':      POST_ERROR("NUL ESCAPE NOT SUPPORTED IN STRING!"); break;
                case ASCII_LF: break;       // Backslash-LF: line splice, adds nothing.

                case 'x':                   // \xNN hex escape (1 or 2 digits)
                {
                    lp->hex_count = 0;
                    lp->hex_val   = 0;
                    lp->state = LEXSTATE_READ_STRING_HEX;
                    break;
                }

                default:
                    POST_ERROR("Unknown escape sequence");
                    esc = ch;   // fallback: literal
                    break;
            }

            if (esc && !string_add_char(lp, esc))
            {
                lp->state = LEXSTATE_SKIP_LINE;     // Don't re-lex the rest of the string as code.
            }
            break;
        }

        case LEXSTATE_READ_STRING_HEX:
        {
            if (isxdigit((unsigned char)ch))
            {
                lp->hex_val = (lp->hex_val * 16) + hex_digit_value(ch);

                if (++lp->hex_count == 2)
                {
                    lp->state = string_add_escaped(lp, lp->hex_val) ? LEXSTATE_READ_STRING
                                                                    : LEXSTATE_SKIP_LINE;
                }
            }
            else
            {
                lp->state = LEXSTATE_READ_STRING;
                advance   = false;                  // Reprocess as ordinary string char.

                if (lp->hex_count == 0)
                {
                    POST_ERROR("Invalid \\x escape");
                }
                else if (!string_add_escaped(lp, lp->hex_val))
                {
                    lp->state = LEXSTATE_SKIP_LINE;
                }
            }
            break;
        }
        case LEXSTATE_READ_CHAR:
        {
            if (ch == '\\')      { lp->state = LEXSTATE_READ_CHAR_ESCAPE; }
            else if (ch == '\'') { LEX_ERROR_SKIP("EMPTY CHARACTER LITERAL!"); }
            else                 { addchar = true; lp->state = LEXSTATE_READ_CHAR_END; }
            break;
        }
        case LEXSTATE_READ_CHAR_ESCAPE:
        {
            char esc = 0;

            switch (ch)
            {
                case 'n':  esc = '\n'; break;
                case 'r':  esc = '\r'; break;
                case 't':  esc = '\t'; break;
                case '\\': esc = '\\'; break;
                case '\'': esc = '\''; break;
                case '"':  esc = '"';  break;
                default:   break;                   // includes '\0': NUL can't be stored
            }

            if (!esc)                          { LEX_ERROR_SKIP("Unknown character escape"); }
            else if (string_add_char(lp, esc)) { lp->state = LEXSTATE_READ_CHAR_END; }
            else                               { lp->state = LEXSTATE_SKIP_LINE; }
            break;
        }
        case LEXSTATE_READ_CHAR_END:
        {
            if (ch == '\'')
            {
                addchar = true;                     // closing quote
                end_atom = true;
            }
            else { LEX_ERROR_SKIP("INVALID CHARACTER LITERAL!"); }
            break;
        }

        // ------------------------------------------------------------
        // WORD
        // 
        case LEXSTATE_READ_WORD:
        {
            if (isalnum((unsigned char)ch) || ch == '_') 
            {
                addchar = true;
            }
            else 
            {
                end_atom = true;
                advance = false;   // reprocess this char
            }
            break;
        }

        // ------------------------------------------------------------
        // NUMBER (float, hex, octal, binary)
        // 
        case LEXSTATE_READ_NUMBER:
        {
            const char* w = number_body(lp);
            char first = w[0];
            size_t len = (size_t)(lp->atom_wordp - w);
            bool alnum_tail = isalnum((unsigned char)ch) || ch == '_';

            // Leading '.' must be followed by a digit.
            if (first == '.' && len == 1 && !isdigit((unsigned char)ch))
            {
                lexer_clear_atom(lp);
                LEX_ERROR_SKIP("INVALID CHARACTER!");
                break;
            }

            if (len == 1 && first == '0' && (ch == 'x' || ch == 'X'))
            { addchar = true; break; }

            // Once a hexadecimal prefix is present, accept only hex digits.
            if (is_hex_prefix(w)) 
            {
                if (isxdigit((unsigned char)ch))   { addchar = true; }
                else if (len == 2)                 { LEX_ERROR_SKIP("INVALID HEXADECIMAL NUMBER!"); }
                else if (alnum_tail)               { LEX_ERROR_SKIP("INVALID HEXADECIMAL DIGIT!"); }
                else                               { end_atom = true; advance = false; }
                break;
            }

            if (len == 1 && first == '0' && (ch == 'b' || ch == 'B'))
            { addchar = true; break; }

            // Once a binary prefix is present, reject digits other than 0/1.
            if (is_binary_prefix(w)) 
            {
                if (ch == '0' || ch == '1')        { addchar = true; }
                else if (len == 2)                 { LEX_ERROR_SKIP("INVALID BINARY NUMBER!"); }
                else if (alnum_tail)               { LEX_ERROR_SKIP("INVALID BINARY DIGIT!"); }
                else                               { end_atom = true; advance = false; }
                break;
            }

            // Octal: 0NNN -- first digit after the leading 0.
            if (first == '0' && len == 1 && isdigit((unsigned char)ch))
            {
                addchar = true;
                break;
            }

            if (first == '0' && len > 1 && isdigit((unsigned char)w[1])
                && !lp->number_has_dot && !lp->number_has_exponent)
            {
                     if (isdigit((unsigned char)ch)) { addchar = true; }
                else if (ch == '.')                  { lp->number_has_dot = true; addchar = true; }
                else if (ch == 'e' || ch == 'E')     { lp->number_has_exponent = true; addchar = true; }
                else if (alnum_tail)                 { LEX_ERROR_SKIP("INVALID OCTAL NUMBER!"); }
                else                                 { end_atom = true; advance = false; }
                break;
            }

            // Decimal / float.
            if (isdigit((unsigned char)ch))
            {
                if (lp->number_has_exponent) { lp->number_exponent_digit = true; }
                addchar = true;
            }
            else if (ch == '.')
            {
                if (!lp->number_has_dot && !lp->number_has_exponent) { lp->number_has_dot = true; addchar = true; }
                else { end_atom = true; advance = false; }
            }
            else if ((ch == 'e' || ch == 'E') && !lp->number_has_exponent)
            {
                lp->number_has_exponent = true;
                lp->number_exponent_digit = false;
                lp->number_exponent_sign = false;
                addchar = true;
            }
            else if ((ch == '+' || ch == '-') && lp->number_has_exponent
                     && !lp->number_exponent_digit && !lp->number_exponent_sign
                     && len && (w[len-1] == 'e' || w[len-1] == 'E'))
            {
                lp->number_exponent_sign = true;
                addchar = true;
            }
            else if (lp->number_has_exponent && !lp->number_exponent_digit)
            { LEX_ERROR_SKIP("INVALID EXPONENT!"); }
            else if (alnum_tail)
            { LEX_ERROR_SKIP("INVALID NUMBER!"); }
            else { end_atom = true; advance = false; }
            break;
        }

        // ------------------------------------------------------------
        // OPERATOR
        // 
        case LEXSTATE_READ_OPERATOR:
        {
            const char* ow = lp->atom->data.word;
            size_t olen = (size_t)(lp->atom_wordp - ow);

            // "-" / "+" directly followed by a digit or '.' where an operand
            // is expected: continue as a signed number (same as SKIP_WHITE does).
            if (   olen == 1 && (ow[0] == '+' || ow[0] == '-')
               && (isdigit((unsigned char)ch) || ch == '.')
               && lexer_sign_allowed(lp))
            {
                lp->number_signed = true;
                lp->state = LEXSTATE_READ_NUMBER;
                if (ch == '.') { lp->number_has_dot = true; }
                addchar = true;
                break;
            }

            if (Is_Operator_Char(ch)) 
            {
                char temp[8];
                size_t len = (size_t)(lp->atom_wordp - lp->atom->data.word);

                if (len == 1 && lp->atom->data.word[0] == '/' && (ch == '/' || ch == '*'))
                {
                    // Comment opener: drop the '/' we already took and consume this char.
                    lexer_clear_atom(lp);
                    lp->state = (ch == '/') ? LEXSTATE_COMMENT_LINE : LEXSTATE_BLOCK_COMMENT;
                }
                else if (len < sizeof(temp) - 1) 
                {
                    memcpy(temp, lp->atom->data.word, len);
                    temp[len] = ch;
                    temp[len+1] = 0;

                    if (Is_Valid_Operator(temp)) 
                    {
                        addchar = true;
                    } 
                    else 
                    {
                        end_atom = true;
                        advance = false;   // reprocess this char
                    }
                }
                else
                {
                    end_atom = true;
                    advance = false;
                }
            }
            else 
            {
                end_atom = true;
                advance = false;
            }
            break;
        }

        // ------------------------------------------------------------
        // LINE COMMENTS    (starts with '//', ends with LF)
        // 
        case LEXSTATE_COMMENT_LINE:
        {
            break;                      // consume character.
        }

        // ------------------------------------------------------------
        // SKIP LINE (error recovery: discard until LF)
        // 
        case LEXSTATE_SKIP_LINE:
        {
            break;                      // consume character.
        }

        // ------------------------------------------------------------
        // BLOCK COMMENTS    (starts with '/*', ends with '*/' )
        // 
        case LEXSTATE_BLOCK_COMMENT:
        {
            if (ch == '*') { lp->state = LEXSTATE_BLOCK_COMMENT_STAR; }
            break;
        }

        case LEXSTATE_BLOCK_COMMENT_STAR:
        {
            if (ch == '/')       { lp->state = LEXSTATE_SKIP_WHITE; }
            else if (ch != '*')  { lp->state = LEXSTATE_BLOCK_COMMENT; }
            break;
        }

        default: { POST_ERROR("BAD STATE!");  break; }
    }

    if (addchar)
    {
        if (left > 1) 
        {
            *bptr++ = ch;
            *bptr = 0;
            lp->atom_wordp = bptr;
            lp->atom_word_left = --left;
        }
        else 
        { 
            // Discard the rest of the line rather than splitting one token into two.
            POST_ERROR("Word too long!");
            lp->state = LEXSTATE_SKIP_LINE;
            end_atom = false;
            advance = true;
        }
    }

    if (end_atom) 
    {
        if (lexer_next_atom(lp))
        {
            lp->state = LEXSTATE_SKIP_WHITE;
        }
        else 
        {
            POST_ERROR("Can't get next atom!");
            lp->state = LEXSTATE_SKIP_LINE;
            advance = (ch != ASCII_LF);     // Don't swallow an LF: the terminator handler needs it.
        }
    }

    if (advance) 
    {
        ++lp->buff;
        --lp->buff_left;
    }

    return false;   // Don't process atom list.
}


#define DUMP_BUFF_SIZE 300

PRIVATE char dump_buff[DUMP_BUFF_SIZE];

PUBLIC void Lexer_Dump(lexer_h lh)
{
    if (lh)
    {
        lexer_t* lp = lh;

        size_t size = sizeof(dump_buff);
        char* buffp = dump_buff;

        dump_buff[0] = 0;

        Print_Atom(lp->first, &buffp, &size, false);       // First atom is at base of heap.

        DBG(0, DEBUG_BLESERVICE, "%s\n---\n", dump_buff);
    }
}


PRIVATE bool lexer_line_pending(const lexer_t* lp)
//
// True if the current line has any atoms not yet handed on.
{
    return lp->atom
        && ( lp->atom_count > 1
          || lp->atom->type != ATOM_TYPE_WORD
          || lp->atom->data.word[0] );
}


PUBLIC bool Lexer_End(lexer_h lh)
//
// Call after the last buffer.  Validates partial tokens and flushes a final
// line that had no trailing LF.  Leaves the lexer reset and ready for reuse.
{
    if (!lh) { return false; }

    lexer_t* lp = lh;

    if (lp->dead && !Lexer_Reset(lp)) { POST_ERROR("Lexer not ready!"); return false; }

    if (!lp->dead) 
    {
        bool ok = lexer_token_complete(lp);

        if (ok && lexer_line_pending(lp) && lexer_next_atom(lp))
        {
            if (lp->last_error_serial != Error_Serial) 
            {
                POST_ERROR("error(s) in line"); 
            }
            else if (lp->process) 
            {
                context_t context = { .heap = lp->heap };
                lp->process(&context, lp->first); 
            }
        }

        if (lp->pending_line_splice)
        {
            POST_ERROR("Backslash at end of file");

            lp->pending_line_splice = false;
        }
    }
    return Lexer_Reset(lp);
}


PUBLIC void Lexer_Process_Buffer(lexer_h lh, char* buff, size_t buff_size)
{
    if (!lh || !buff || !buff_size) { return; }

    lexer_t* lp = lh;

    if (lp->dead && !Lexer_Reset(lp)) { POST_ERROR("Lexer not ready!"); return; }

    lp->buff = buff;
    lp->buff_left = buff_size;

    while (lp->buff_left)
    {
        if (lexer_process_character(lp))
        {
            if (lp->last_error_serial != Error_Serial) 
            {
                POST_ERROR("error(s) in line");
            }
            else if (lp->process) 
            {
                context_t context = { .heap = lp->heap };
                lp->process(&context, lp->first); 
            }

            if (!Lexer_Reset(lp))
            {
                lp->buff_left = 0;          // lp->atom may be NULL: stop now.
                break;
            }
        }
    }
}


// End of file: lexer.c
