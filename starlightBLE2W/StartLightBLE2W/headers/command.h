// File: command.h

#ifndef COMMAND_H
#define COMMAND_H

#include "atom.h"
#include "balloc.h"
#include "evaluator.h"


// Important: command functions must report failures
// Every command handler must use POST_ERROR() when an operation fails.

typedef enum Command_Code
{
    MATCH_CODE_FAIL = -1,
    MATCH_CODE_NONE = 0,
    //
    // Positive match codes are reserved for valid commands.

} command_code_t;


struct command;   // Foreward.

typedef const atom_t* (*Command_Cooked_Function)
(
    context_t* context,
    const struct command* cmd,
    size_t argc,
    const atom_t* argv
);

typedef const atom_t* (*Command_Raw_Function)
(
    context_t* context,
    const struct command* cmd,
    size_t argc,
    const atom_t** argv
);

#define CMD_COOKED_PROTO(cmd) const atom_t* cmd(context_t *E, const struct command* cmdp, size_t argc, const atom_t* argv)
#define CMD_RAW_PROTO(cmd)    const atom_t* cmd(context_t *E, const struct command* cmdp, size_t argc, const atom_t** argv)


typedef enum
{
    COMMAND_ARGS_COOKED = 0,   // existing behavior
    COMMAND_ARGS_RAW    = 1    // receive expression trees

} command_arg_mode_t;

#define COMMAND_ARITY_VARIADIC UINT8_MAX

typedef struct command
{
    const char* name;                       // Full match string.
    command_code_t code;                    // Match code when matched.

    uint8_t min_arity;
    uint8_t max_arity;   // UINT8_MAX == variadic

    Command_Cooked_Function cooked_func;    // function pointer to call when matched (optional).
    Command_Raw_Function    raw_func;       // With raw unevaluated args.

    void* data;                             // user data associated with this match item (optional).

    command_arg_mode_t arg_mode; 

    const char* signature;                  // Signature string for this keyword (optional).
    const char* help;                       // Help string for this keyword (optional).

} command_t;


typedef struct command_array
{
    size_t     count;      // Number of commands in the array.
    command_t* command;    // Pointer to the first command.

} command_array_t;


extern command_t* Command_Find_Code(command_array_t* commands, command_code_t code);

extern command_t* Command_Find_Name(command_array_t* commands, const char* name);

extern const atom_t* Command_Run_Cooked(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv);
extern const atom_t* Command_Run_Raw   (context_t* context, const command_t* cmd, size_t argc, const atom_t** argv);


//--------------------------------------------------------------------------------
extern bool Match_Argument_Keyword(const atom_t* atom, const char* word);

extern void Parse_Affirmative(const char* str, bool* affirmative);

extern void Dump_Commands(char** buffp, size_t* buffsizep, const char* glob);


//--------------------------------------------------------------------------------
// Apply_Output
//
// Runs an output callback against the heap's scratch buffer and returns the text it
// produced as a constant string atom.
//
// The callback is given the free space at the top of the heap. It writes its text there,
// advancing *msgp and decrementing *len as it goes. The result is then committed to the
// heap, so the string persists after the call returns.
//
// Parameters:
//   context  Evaluation context; its heap supplies the scratch buffer.
//   f        Output callback, called as f(&msgp, &len, glob, arg). Must null-terminate
//            its output and must not write more than *len bytes.
//   glob     Optional glob pattern passed through to the callback. May be NULL.
//   arg      Optional user data passed through to the callback. May be NULL.
//
//            Callbacks that don't need glob or arg must ignore them.
//
// Returns:
//   Pointer to a string atom holding the callback's output, or NULL if
//     - the heap scratch buffer could not be reserved, or
//     - the callback produced no output (empty string).
//
// Notes:
//   Callers must treat NULL as "no result", not necessarily as an error.
//
typedef atom_t* (*buff_output_f)(context_t* context, char** msgp, size_t* len, const char* glob, void* arg);

extern const atom_t* Apply_Output(context_t* context, buff_output_f f, const char* glob, void* arg);

//--------------------------------------------------------------------------------
extern void Parse_Arg_Bool(const atom_t* arg, bool* result);
extern void Parse_Arg_String(const atom_t* arg, const char** result);

//--------------------------------------------------------------------------------
typedef struct keyword
{
    const char* name;           // Full match string.
    uint32_t code;              // Match code when matched.

    void* data;                 // user data associated with this match item (optional).

    const char* help;           // Help string for this keyword (optional).

} keyword_t;


typedef struct keyword_array
{
    size_t     count;      // Number of keywords in the array.
    keyword_t* keyword;    // Pointer to the first keyword.

} keyword_array_t;

extern keyword_t* Keyword_Find(const keyword_array_t* keywords, const char* name);
extern keyword_t* Match_Argument_Key(const keyword_array_t* keys, const atom_t* atom);


#endif // COMMAND_H

// End of file: command.h
