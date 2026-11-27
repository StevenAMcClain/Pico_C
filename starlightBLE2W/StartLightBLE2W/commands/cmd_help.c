// File: cmd_help.c

#include "common.h"
#include "cmd_help.h"

//==========================================================================================================================
//
PUBLIC const atom_t* do_help_command(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    UNUSED(context);
    UNUSED(cmd);
    UNUSED(argc);
    UNUSED(argv);
    static const char help_string[] = "This is the help file\nCommands:\nHELP - Show this file.\n\n";

    return Result_CString(context, help_string);

//     Post_Output(help_string, 0);

//     atom_t* result = Balloc(context->heap, sizeof(atom_t));

//     if (result)
//     {
//         Atom_Zap(result);
//         result->type = ATOM_TYPE_INT;
//         result->data.i = 99;   // indicate success
// //        result->data.i = 0;   // indicate success
//     } 

//     return result;
}


// End of file: cmd_help.c
