// File: command.c

#include "common.h"
#include "command.h"

#include <stdio.h>
#include <strings.h>

#include "cmd_table.h"
#include "error.h"
#include "evaluator.h"
#include "globmatch.h"
#include "print.h"


PUBLIC command_t* Command_Find_Code(command_array_t* commands, command_code_t code)
{
    if (commands == NULL || commands->command == NULL) { return NULL; }

    command_t* cmd = commands->command;
    size_t count = commands->count;

    while (count--)
    {
        if (cmd->code == code) { return cmd; }  // Return the command.

        ++cmd;   // Next record
    }

    return NULL;  // No match found
}


PUBLIC command_t* Command_Find_Name(command_array_t* commands, const char* name)
{
    if (commands == NULL || commands->command == NULL || name == NULL) { return NULL; }
    
    command_t* cmd = commands->command;
    size_t count = commands->count;

    while (count--)
    {
        if (strcmp(name, cmd->name) == 0) { return cmd; }  // Return the matcher_item

        ++cmd;   // Next match record
    }

    return NULL;  // No match found
}


PUBLIC void Dump_Commands(char** buffp, size_t* buffsizep, const char* glob)
{
    BUFF_WRITER_START(buffp, buffsizep);

    BUFF_WRITER_PRINTF("Command_List:\n");

    command_t* cmd = Command_Table.command;
    size_t count = Command_Table.count;
    size_t index = 0;

    if (glob)
    {
        while (count--)
        {
            if (globmatch(glob, cmd->name, 0) == 0) 
            { 
                BUFF_WRITER_PRINTF("%u '%s'\n", (unsigned)++index, cmd->name); 
            }
            ++cmd;   // Next command record
        }
    }
    else
    {
        while (count--)
        {
            BUFF_WRITER_PRINTF("%u '%s'\n", (unsigned)++index, cmd->name);
            ++cmd;   // Next command record
        }
    }

    BUFF_WRITER_END(buffp, buffsizep);
}


PUBLIC const atom_t* Command_Run_Cooked(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    if (cmd && cmd->cooked_func)
    {
        Command_Cooked_Function func = cmd->cooked_func;
        return func(context, cmd, argc, argv);
    }
    return NULL;  // No function to call
}


PUBLIC const atom_t* Command_Run_Raw(context_t* context, const command_t* cmd, size_t argc, const atom_t** argv)
{
    if (cmd && cmd->raw_func)
    {
        Command_Raw_Function func = cmd->raw_func;
        return func(context, cmd, argc, argv);
    }
    return NULL;  // No function to call
}


// ======================================================

PUBLIC bool Match_Argument_Keyword(const atom_t* atom, const char* word)
{
    if (atom && atom->type == ATOM_TYPE_WORD)
    {
        const char *match = atom->data.word;
        return strcmp(match, word) == 0;
    }
    return false;
}


PUBLIC void Parse_Affirmative(const char* str, bool* affirmative)
{
    if (!str || !affirmative) { return; }

    if (strcasecmp(str, "true") == 0 || strcasecmp(str, "yes") == 0 || strcasecmp(str, "on") == 0)
    {
        *affirmative = true;
    }
    else if (strcasecmp(str, "false") == 0 || strcasecmp(str, "no") == 0 || strcasecmp(str, "off") == 0)
    {
        *affirmative = false;
    }
}


PUBLIC void Parse_Arg_Bool(const atom_t* arg, bool* result)
{
    if (!result) { return; }

    *result = false;

    if (!arg) { return; }

    switch (arg->type)
    {
        case ATOM_TYPE_BOOL:
        case ATOM_TYPE_INT:     { *result = arg->data.i != 0; break; }
        case ATOM_TYPE_UINT:    { *result = arg->data.ui != 0; break; }
        case ATOM_TYPE_BYTE:    { *result = arg->data.b != 0; break; }
        case ATOM_TYPE_FLOAT:   { *result = arg->data.f != 0.0f; break; }
        case ATOM_TYPE_WORD:    { if (arg->data.word[0] != '\0') { Parse_Affirmative(arg->data.word, result); } break; }
        case ATOM_TYPE_STRING:  { if (arg->data.ptr != NULL) { Parse_Affirmative((const char*)arg->data.ptr, result); } break; }
        case ATOM_TYPE_RSTRING: { const char* rstr = (const char*)RLINK_PTR(arg, arg->data.rptr);
                                  if (rstr != NULL) { Parse_Affirmative(rstr, result); } break; }
        default: { break; }
    }
}


PUBLIC void Parse_Arg_String(const atom_t* arg, const char** result)
{
    if (!result) { return; }

    *result = NULL;

    if (!arg) { return; }

    switch (arg->type)
    {
        case ATOM_TYPE_WORD:    { if (arg->data.word[0] != '\0') { *result = arg->data.word; } break; }
        case ATOM_TYPE_STRING:  { if (arg->data.ptr != NULL)     { *result = (const char*)arg->data.ptr; } break; }
        case ATOM_TYPE_RSTRING: {                                  *result = (const char*)RLINK_PTR(arg, arg->data.rptr); break; }
        case ATOM_TYPE_BOOL:
        case ATOM_TYPE_INT:
        case ATOM_TYPE_UINT:
        case ATOM_TYPE_BYTE:
        case ATOM_TYPE_FLOAT:   { POST_ERROR("Cannot handle number to string."); break; }
        default: { break; }
    }
}


//-----------------------------------------------------------------------------------------
//
PUBLIC const atom_t* Apply_Output(context_t* context, buff_output_f f, const char* glob, void* arg)
{
    if (!Balloc_Reserve(context->heap))
        return NULL;

    char*  msg  = Balloc_Top(context->heap);
    char*  msgp = msg;
    size_t len  = Balloc_Left(context->heap);

    *msg = 0;

    f(context, &msgp, &len, glob, arg);

    len = strlen(msg) + 1;
    Balloc_Release(context->heap);

    if (len > 1)
    {
        Balloc(context->heap, len);
        DBG(0, DEBUG_COMMAND, "%s\n", msg);
        return Result_CString(context, msg);
    }
    return NULL;
}


//---------------------------------------------------------------------------------------
PUBLIC keyword_t* Keyword_Find(const keyword_array_t* keywords, const char* name)
{
    if (!keywords || !keywords->keyword || !name) { return NULL; }

    keyword_t* kw = keywords->keyword;
    size_t count = keywords->count;

    while (count--)
    {
        if (strcmp(name, kw->name) == 0) { return kw; }  // Return the matcher_item
        ++kw;   // Next match record
    }

    return NULL;  // No match found
}


PUBLIC keyword_t* Match_Argument_Key(const keyword_array_t* keys, const atom_t* atom)
{
    if (atom && atom->type == ATOM_TYPE_WORD)
    {
        const char *match = atom->data.word;
        return Keyword_Find(keys, match);
    }
    return NULL;
}


// EndFile: command.c
