// File: lexer.h

#ifndef LEXER_H
#define LEXER_H

#include "atom.h"
#include "balloc.h"
#include "evaluator.h"

#define ASCII_CR '\r'
#define ASCII_LF '\n'

#define IS_SPACE(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == ASCII_CR)


typedef void* lexer_h;

typedef void (*process_f)(context_t* context, atom_t* first);    // Called to process lexed atoms.

extern lexer_h Lexer_Initialize(size_t heap_size, process_f process);

//extern bool Lexer_Reset(lexer_h lh);

extern balloc_h Lexer_Heap(lexer_h lh);

extern atom_t* Lexer_Trim_End(atom_t* first);

extern void Lexer_Process_Buffer(lexer_h ph, char* buff, size_t buff_size);

extern bool Is_Punctuation_Char(char c);

extern void Lexer_Dump(lexer_h lh);
extern bool Lexer_End(lexer_h lh);

#endif // LEXER_H

// End of file: lexer.h
