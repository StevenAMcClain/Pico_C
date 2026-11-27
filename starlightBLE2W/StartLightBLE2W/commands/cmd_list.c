// File: cmd_list.c

#include "common.h"
#include "cmd_list.h"

#include <stdio.h>
#include <string.h>

#include "atom.h"
#include "command.h"
#include "error.h"
#include "io_service.h"
#include "variables.h"


typedef enum
{
    LIST_OPTION_NONE = 0,
    LIST_OPTION_CMDS,
    LIST_OPTION_SRVS,
    LIST_OPTION_VARS,

} LIST_OPTION_CODE;

//---------------------------------------------------------------------------------------------------------------------------------------
//
PRIVATE const atom_t* do_Dump_Srvs_List(context_t* context, char** msgp, size_t* len, const char* glob, void* arg)
{
    UNUSED(arg); UNUSED(glob);
    Dump_Srvs_List(msgp, len);
    return Result_CString(context, "do_Dump_Srvs_List");
}

PRIVATE const atom_t* do_Dump_Commands(context_t* context, char** msgp, size_t* len, const char* glob, void* arg)
{
    UNUSED(arg); UNUSED(glob);
    Dump_Commands(msgp, len, glob);
    return Result_CString(context, "do_Dump_Commands");
}

PRIVATE const atom_t* do_Dump_Variables(context_t* context, char** msgp, size_t* len, const char* glob, void* arg)
{
    UNUSED(arg);
    Dump_Variables(msgp, len, glob);
    return Result_CString(context, "do_Dump_Variables");
}

////Lexer_Dump(lexer_h lh)
///list_all_uuids
///Heap_Dump

PRIVATE keyword_t keyword_list[] = 
{ 
    {"CMDS", LIST_OPTION_CMDS,  do_Dump_Commands, "List of commands."},
    {"ATTR", LIST_OPTION_SRVS, do_Dump_Srvs_List, "List Attr Servers."},
    {"VARS", LIST_OPTION_VARS, do_Dump_Variables, "list of variables"},
};

PRIVATE const keyword_array_t list_keywords = { ARRAY_SIZE(keyword_list), keyword_list };


//---------------------------------------------------------------------------------------------------------------------------------------
//
PUBLIC const atom_t* do_list_command(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    UNUSED(cmd);

    if (argc > 2) { POST_ERROR("I only expect up to two args!"); return NULL; }

    if (argc)
    {
        keyword_t* keyword = Match_Argument_Key(&list_keywords, argv);

        if (keyword && keyword->data) 
        {
            const char* glob = "*";

            if (argc == 2)
            {
                Parse_Arg_String(argv + 1, &glob);
                if (glob) { DBG(0, DEBUG_COMMAND, "LIST GLOB IS '%s'\n", glob); }
            }
            return Apply_Output(context, keyword->data, glob, NULL); 
        }
    }
    return Result_CString(context, HELP_LIST_STR);
}


// End of file: cmd_list.c