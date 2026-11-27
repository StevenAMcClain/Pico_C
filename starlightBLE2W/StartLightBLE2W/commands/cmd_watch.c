// File: cmd_watch.c

#include "common.h"
#include "cmd_watch.h"

#include "error.h"
#include "variables.h"


// num_params == 0 passes params == NULL. Every command function must guard for this.

//---------------------------------------------------------------------------------------------------------------------------------------
//
PUBLIC const atom_t* do_watch_command(context_t* context, const command_t* cmd, size_t argc, const atom_t** argv)
{
    UNUSED(context);  UNUSED(cmd);

    if (argc > 2) {return POST_ERROR("Command has too many args!"); }

    if (argc >= 1)
    {
        const atom_t* atom = *argv;

        if (atom)
        {
            if (atom->type == ATOM_TYPE_VARIABLE)
            {
                variable_t* var = (variable_t*)atom->data.ptr;

                if (var)
                {
                    bool do_watch = true;

                    if (argc == 2)
                    { 
                        const atom_t* arg2 = *(argv + 1);
                        Parse_Arg_Bool(arg2, &do_watch);
                        printf("WATCH: %s, arg2 type %d\n", var->name, arg2->type);
                    }

                    if (do_watch)
                    {
                        var->flags |= VAR_FLAG_WATCHED;
                        return Result_Printf(context, "Watching %s", var->name);
                    }
                    else
                    {
                        var->flags &= ~VAR_FLAG_WATCHED;
                        return Result_Printf(context, "Unwatching %s", var->name);
                    }
                }
                else { POST_ERROR("Command requires a variable parameter!"); }
            }
            else { POST_ERROR("Command requires a variable parameter!"); }
        }
        else { POST_ERROR("Arg is not an atom!"); }
    }
    return Result_CString(context, "Expect: WATCH <var>");
}


// End of file: cmd_watch.c
