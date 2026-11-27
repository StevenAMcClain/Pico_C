// File: shell.c

#include "common.h"
#include "shell.h"

#include <stdio.h>
#include <string.h>

#include <hardware/sync.h>
#include <pico/flash.h>
#include <pico/stdlib.h>

#include "BLECore.h"   // For: BLE_Set_Node_Name()
#include "command.h"
#include "error.h"
#include "evaluator.h"
#include "lexer.h"
#include "print.h"
#include "spsc_queue.h"
#include "variables.h"
#include "var_list.h"
#include "watcher.h"

#define RESULT_BUFF_SIZE 250

#define LEXER_HEAP_SIZE (16 * 1024)           // Maximum room for atom list and expression tree.
PRIVATE lexer_h lexer = NIL;


// -----------------------------------------------------------------------------
#define TICK_INTERVAL_MS 50                 // Interval for periodic tasks in milliseconds
PRIVATE repeating_timer_t tick_timer;       // Timer for periodic tasks
PUBLIC volatile uint32_t now_ms = 0;        // Current time in milliseconds.

// -----------------------------------------------------------------------------
PRIVATE bool tick_timer_cb(repeating_timer_t *rt)    // This runs every interval
{
    UNUSED(rt);  // Not used in this callback, but required by the function signature.

    now_ms = to_ms_since_boot(get_absolute_time());

    // Here you can add any periodic tasks that need to run every TICK_INTERVAL_MS milliseconds.

    // Parse_Timer_Tick();   // Decrement the parse timer if it's running

    return true;    // return true to keep repeating
}


//-----------------------------------------------------------------------------------------
//
PRIVATE void post_results(context_t* context, const atom_t* result)
{
    if (!Balloc_Reserve(context->heap)) { POST_ERROR("Can't reserve heap for result"); return; }

    char*  msg  = Balloc_Top(context->heap);
    char*  msgp = msg;
    size_t len  = Balloc_Left(context->heap);

    if (!msg || len < 2)
    {
        Balloc_Release(context->heap);
        POST_ERROR("No room for result");
        return;
    }

    *msg = 0;                                   // Print_Atom may write nothing at all

    Print_Atom(result, &msgp, &len, true);

    if (len > 1) { --len; *msgp++ = '\n'; *msgp = 0; }

    size_t used = (size_t)((uint8_t*)Balloc_Next(context->heap) - (uint8_t*)Balloc_Base(context->heap));

    BUFF_WRITER_WRITE(&msgp, &len, "[%u]\n\n", (unsigned)used);

    #ifdef USE_DEBUG
        if ((DEBUG_RESULTS & Debug_Mask) == DEBUG_RESULTS)
        {
            printf("Result: \"%s\"\n[%u]\n", msg, (unsigned)used);
        }
    #endif

    if (*msg) { Post_Output(msg, 0); }

    Balloc_Release(context->heap);              // Release after posting: msg lives in the reserved area
}


PRIVATE void lexer_process(context_t* context, atom_t* first)
//
// Hand the finished atom list on, then start a fresh line.
{
    atom_t* atom = Lexer_Trim_End(first);

    if (atom)
    {
        size_t old_error_serial = Error_Serial;   // Remember last error serial for this line.

        Atom_Promote_List(atom);          // First atom is at base of heap.

        D(LEXER_DEBUG_DUMP, Lexer_Dump(lexer));

        atom_t *list = Expr_List_Build(context, atom);

        if (Error_Serial != old_error_serial) { POST_ERROR("error(s) in line"); return; }

        if (!list)  { return; }

        const atom_t* result = Eval_Expr_List(context, list);

        if (result) { post_results(context, result); }

        else { POST_ERROR("Expr return no result!"); }
    }
}


PRIVATE void shell_initialize()
{
    lexer = Lexer_Initialize(LEXER_HEAP_SIZE, lexer_process);

    if (lexer)
    {
        flash_safe_execute_core_init();  // Initialize flash access for multicore operation

        add_repeating_timer_ms(TICK_INTERVAL_MS, tick_timer_cb, NULL, &tick_timer);                   // Start the periodic timer tick
    }
    else { POST_ERROR("Lexer_Initialize failed"); }
}


PRIVATE void process_buffer(char* buff, size_t buff_size)
{
    Lexer_Process_Buffer(lexer, buff, buff_size);
}


// -----------------------------------------------------------------------------
PUBLIC void Shell(void)
//
// Shell main loop entry point.  This runs on core1 and never returns.
{
    Var_Initialize();

    shell_initialize();

    add_vars(lexer);

    while (1)
    {
        consume(process_buffer);

        if ( Error_Posted() )   { Error_Flush("Errors:\n"); }

        if ( Watcher_Posted() ) { Watcher_Flush(); }

        tight_loop_contents();

        __wfe();  // Wait for event to reduce CPU usage
    }
}

// End of file: shell.c