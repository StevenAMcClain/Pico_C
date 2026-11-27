// File: cmd_misc.c

#include "common.h"
#include "command.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <hardware/watchdog.h>

#include "atom.h"
#include "config.h"
#include "error.h"
#include "io_service.h"
#include "parser.h"
#include "shell.h"
#include "print.h"
#include "variables.h"

#include "cmd_table.h"
#include "cmd_list.h"


PUBLIC CMD_COOKED_PROTO(do_check_command);
PUBLIC CMD_COOKED_PROTO(do_config_command);
PUBLIC CMD_COOKED_PROTO(do_help_command);
PUBLIC CMD_COOKED_PROTO(do_load_command);
PUBLIC CMD_RAW_PROTO(do_parms_command);
//PUBLIC CMD_RAW_PROTO(do_watch_command);

// num_params == 0 passes params == NULL. Every command function must guard for this.

//==========================================================================================================================

// PRIVATE int calc_checksum(const char* buff, size_t blen)
// {
//     int check = 0;

//     for (size_t i = 0; i < blen; i++) { check += buff[i]; }

//     return check;
// }

// typedef void read_buff_t;

// PRIVATE void check_buff_read_done(read_buff_t* buff)
// {
//     UNUSED(buff);

    // printf("%s: bigbuff: '%s'\n", __func__, bigbuff);

    // const char* bptr = skip_first_line(bigbuff);
    // int len = strlen(bptr);
    // int checksum = calc_checksum(bptr, len);

    // bool result = Process_Config(bigbuff, false);

    // if (result) { snprintf(bigbuff, sizeof(bigbuff), "Success.\n"); }
    // else        { snprintf(bigbuff, sizeof(bigbuff), "Fail: %d, Size: %d, Check: %d\n", result, len, checksum); }

    // Post_Output(bigbuff, 0);
// }


PUBLIC const atom_t* do_check_command(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    UNUSED(context);
    UNUSED(cmd);
    UNUSED(argc);
    UNUSED(argv);
//     if (  (num_params >= 2)
//        && (params[0].type == PARM_TYPE_BYTE || params[0].type == PARM_TYPE_UINT)
//        && (params[1].type == PARM_TYPE_BYTE || params[1].type == PARM_TYPE_UINT) )
//     {
//         size_t blen = params[0].data.ui;   // requested payload size
        
//         if (blen > 0 && blen < (sizeof(bigbuff) - 1))
//         {
//             char* bptr = bigbuff;

//             // Write the header using the buffer's real capacity, not the
//             // caller-requested payload size — otherwise a small `blen` truncates
//             // the "ABLE ..." header itself.
//             snprintf(bigbuff, sizeof(bigbuff), "ABLE %zu %lu\n", blen, params[1].data.ui);

//             size_t slen = strlen(bigbuff);

//             if (slen < sizeof(bigbuff))
//             {
//                 size_t max_len = sizeof(bigbuff) - slen - 1;   // space left for payload + NUL
//                 bptr += slen;

//                 if (blen > max_len) { blen = max_len; }
//                 if (blen > 0)
//                 {
//                     printf("%s: Start to read buffer.  Size %zu\n", __func__, blen);
// //                    Parser_Start_Read_Buffer(bptr, blen + 1, check_buff_read_done);
//                 }
//             }
//             else { POST_ERROR("slen too long!"); }
//         }
//     }
//     else
//     {
//         const char* p = "unknown error\n";

//              if (  params[0].type == PARM_TYPE_UNDEFINED)   { p = "Expecting a size!\n";         }
//         else if (  params[0].type != PARM_TYPE_BYTE 
//                 && params[0].type != PARM_TYPE_UINT)        { p = "Size must be integer!\n";     }
//         else if (  params[1].type == PARM_TYPE_UNDEFINED)   { p = "Expecting a checksum!\n";     }
//         else if (  params[1].type != PARM_TYPE_BYTE 
//                 && params[1].type != PARM_TYPE_UINT)        { p = "Checksum must be integer!\n"; }

//         Post_Output(p, 0);
//     }

//    return NULL;

    return Result_UInt(context, 123456789);   // indicate success
}


// PRIVATE void load_buff_read_done(read_buff_t* buff)
// {
//     UNUSED(buff);

//     printf("%s: bigbuff: '%s'\n", __func__, bigbuff);

//     bool result = Process_Config(bigbuff, false);

//     if (result)
//     {
//         if (Save_Config(bigbuff) == PICO_OK)                // Write config to flash.
//         {
//             watchdog_reboot(0, 0, 0);   // !!!! REBOOT NOW !!!!
//         }
//         else { POST_ERROR("CANT SAVE CONFIG!\n"); }
//     }

//     if (result) { snprintf(bigbuff, sizeof(bigbuff), "Success.\n"); }
//     else        
//     {
//         const char* bptr = skip_first_line(bigbuff);
//         size_t len = strlen(bptr);
//         int checksum = calc_checksum(bptr, len);

//          snprintf(bigbuff, sizeof(bigbuff), "Fail: %d, Size: %zu, Check: %d\n", result, len, checksum); 
//     }
//     Post_Output(bigbuff, 0);
// }


PUBLIC const atom_t* do_load_command(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    UNUSED(cmd);
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(context);  

//     if (  (num_params >= 1)
//        && (params[0].type == PARM_TYPE_BYTE || params[0].type == PARM_TYPE_UINT) )
//     {
//         size_t blen = params[0].data.ui;
// //        char* bptr = bigbuff;

//         if (blen > 0 && blen < (sizeof(bigbuff) - 1))
//         {
//             printf("%s: Start to read buffer.  Size %zu\n", __func__, blen);
// //            Parser_Start_Read_Buffer(bptr, blen + 1, load_buff_read_done);
//         }
//         else { if (blen == 0) { POST_ERROR("len is zero!"); } else { POST_ERROR("len too long!"); } }
//     }
//     else
//     {
//         const char* p = "unknown error\n";

//              if (  params[0].type == PARM_TYPE_UNDEFINED)   { p = "Expecting a size!\n";         }
//         else if (  params[0].type != PARM_TYPE_BYTE 
//                 && params[0].type != PARM_TYPE_UINT)        { p = "Size must be integer!\n";     }

//         Post_Output(p, 0);
//     }
    return NULL;
}


//---------------------------------------------------------------------------------------------------------------------------------------
//
// PUBLIC atom_t* do_parms_command(eval_context_t* context, command_t* cmd, size_t argc, const expression_t** argv)
// {
//     UNUSED(context);
//     UNUSED(cmd);

//     char bigbuff[1024];
//     char *bufp = bigbuff;
//     size_t size = sizeof(bigbuff);

//     int len = snprintf(bufp, size, "PARMS: %zu\n", argc);

//     if (len > 0)
//     {
//         bufp += len;
//         size -= (size_t)len;
//     }

//     for (size_t i = 0; i < argc && size > 1; ++i)
//     {
//         expression_dump(argv[i]);

//         len = snprintf(bufp, size, "arg[%zu]\n", i);

//         if (len > 0)
//         {
//             bufp += len;
//             size -= (size_t)len;
//         }
//     }

//     Post_Output(bigbuff, 0);

//     return NULL;
// }


//---------------------------------------------------------------------------------------------------------------------------------------
//
// PUBLIC atom_t* do_pin_command(eval_context_t* context, command_t* cmd, size_t argc, const expression_t** argv)
// {
//     UNUSED(context);
//     UNUSED(cmd);

//     char bigbuff[1024];
//     char *bufp = bigbuff;
//     size_t size = sizeof(bigbuff);

//     int len = snprintf(bufp, size, "PIN: %zu\n", argc);

//     if (len > 0)
//     {
//         bufp += len;
//         size -= (size_t)len;
//     }

//     for (size_t i = 0; i < argc && size > 1; ++i)
//     {
//         expression_dump(argv[i]);

//         len = snprintf(bufp, size, "arg[%zu]\n", i);

//         if (len > 0)
//         {
//             bufp += len;
//             size -= (size_t)len;
//         }
//     }

//     Post_Output(bigbuff, 0);

//     return NULL;
// }


//---------------------------------------------------------------------------------------------------------------------------------------
//
PUBLIC const atom_t* do_config_command(context_t* context, const command_t* cmd, size_t argc, const atom_t* argv)
{
    UNUSED(context); UNUSED(cmd); UNUSED(argc); UNUSED(argv);
// atom 48, expr 24, exprl 8, cmd 32, var 72
    printf("\n\natom %d, cmd %d, var %d\n\n", 
        sizeof(atom_t), sizeof(command_t), sizeof(variable_t) );

    // char* config = Config_File();

    // if (*config != 0xFF)
    // {
    //     snprintf(bigbuff, sizeof(bigbuff), "CONFIG:\n");
    //     size_t len = strlen(bigbuff);
    //     Post_Output(bigbuff, 0);

    //     size_t config_len = strlen(config);

    //     // NOTE: config lives in its own (flash) buffer, not in bigbuff, so
    //     // checking it against sizeof(bigbuff) doesn't actually bound it.
    //     // Replace CONFIG_FILE_MAX_LEN with whatever real limit Config_File()
    //     // guarantees (flash page size, etc.) so this guard means something.
    //     if (config_len > 0 && config_len < CONFIG_FILE_MAX_LEN)
    //     {
    //         Post_Output(config, 0);
    //     }
    //     else
    //     {
    //         size_t elen = snprintf(bigbuff + len, sizeof(bigbuff) - len, "CONFIG FILE TOO LONG!\n");
    //         Post_Output(bigbuff + len, elen);
    //     }

    //     Post_Output("\n", 1);        // Print trailing carriage return.
    // }
    // else Post_Output("No Config.\n", 11);
    return NULL;
}


// End of file: commands.c

    // {"RESE", 0, COMMAND_MATCH_RESET}, 
    // {"BLAC", 0, MATCH_BLACK}, 
    // {"BLOB", 0, MATCH_LOAD_BLOB}, 
    // {"BLST", 0, MATCH_STAGE_BLOB}, 
    // {"BLBA", 0, MATCH_ACTIVATE_BLOB}, 
    // {"XBLB", 0, MATCH_EXPORT_BLOB},

    // {"SCEN", 0, MATCH_SCENE}, 
    // {"SHOW", 0, MATCH_SHOW}, 
    // {"UPDA", 0, MATCH_UPDATE}, 

    // {"TRIG", 0, MATCH_TRIGGER}, 
    // {"INTR", 0, MATCH_INTERRUPT},
    // {"QUEU", 0, MATCH_QUEUE}, 

    // {"GETV", 0, MATCH_GETV}, 
    // {"SETV", 0, MATCH_SETV}, 
    // {"SETQ", 0, MATCH_SETQ}, 

    // {"LOAD", 0, MATCH_LOAD},
    // {"LDWT", 0, MATCH_LOAD_WAIT},
    // {"SAVE", 0, MATCH_SAVE},
    // {"ERAS", 0, MATCH_ERASE},

    // {"DUMP", 0, MATCH_DUMP},

    // {"SPHY", 0, MATCH_SPHY}, 

//    {"SENG", 0, MATCH_SENG}, 
//    {"PHYS", 0, MATCH_PHYS}, 
//    {"BRIG", 0, MATCH_BRIGHTNESS}, 
//    {"DEBU", 0, MATCH_DEBUG},


//    MATCH_RESET,        // Stop all engines and set to no blob loaded.
//     MATCH_BLACK,        // All leds black (stop running blob and clears queue)

// // - Variables.

//     MATCH_GETV,         // Get variable.
//     MATCH_SETV,         // Set variable.
//     MATCH_SETQ,         // Set variable (quiet).

// // - Blobs.

//     MATCH_LOAD_BLOB,     // Loads a new blob.  Start running.
//     MATCH_STAGE_BLOB,    // Loads a new blob.  Waits for activate to start running.
//     MATCH_ACTIVATE_BLOB, // Start running new blob.
//     MATCH_EXPORT_BLOB,   // Export current blob.

//     MATCH_LOAD,         // Load flash blob to RAM.
//     MATCH_LOAD_WAIT,    // Load flash blob to RAM.  Wait for activate to start running.
//     MATCH_SAVE,         // Save RAM blob to flash.
//     MATCH_ERASE,        // Erase flash page.

// // Scenes.

//     MATCH_SCENE,        // Set leds to scene from terminal.
//     MATCH_SHOW,         // Set leds to scene from loaded blob.
//     MATCH_UPDATE,       // Update leds that need updaing.

// // Events.

//     MATCH_TRIGGER,      // Trigger a program to start running immediately.
//     MATCH_INTERRUPT,    // Halt running program, run new program, then resume halted program.
//     MATCH_QUEUE,        // Queue a program to start running after current program completes.

// // Instrumentation.

//     MATCH_DUMP,         // Dump debug info.

// // Dregs.

//     MATCH_SPHY,         // Set the current led array to use.
// //    MATCH_BRIGHTNESS,   // Set led brighness (0.0 -> 1.0)
// //    MATCH_SENG,         // Set the current engine to use.
// //    MATCH_DEBUG,        // Set debug mask.
//PRIVATE void parm_to_string(atom_t* parm)
// {
//     switch (parm->type)
//     {
//         case PARM_TYPE_UNDEFINED:
//         {
//             printf("???");
//             break;
//         }
//         case PARM_TYPE_BYTE:      // unsigned 8-bit integer.
//         {
//             uint8_t val = parm->data.b;

//             if (parm->flag == PARM_FLAG_IS_BINARY)
//             {
//                 char bin[33];
//                 printf("0b%s", to_binary_trim(val, bin));
//             }
//             else if (parm->flag == PARM_FLAG_IS_CHARACTER )
//             {
//                 printf("'%c'", val);
//             }
//             else if (parm->flag == PARM_FLAG_IS_HEXIDECIMAL )
//             {
//                 printf("0x%X", val);
//             }
//             else
//             {
//                 printf("%d", val);
//             }
//             break;
//         }
//         case PARM_TYPE_INT:       // signed 32-bit integer.
//         {
//             printf("%d", parm->data.i);
//             break;
//         }
//         case PARM_TYPE_UINT:      // unsigned 32-bit integer.
//         {
//             uint32_t val = parm->data.ui;

//             if (parm->flag == PARM_FLAG_IS_BINARY)
//             {
//                 char bin[33];
//                 printf("0b%s", to_binary_trim(val, bin));
//             }
//             else if (parm->flag == PARM_FLAG_IS_HEXIDECIMAL )
//             {
//                 printf("0x%X", val);
//             }
//             else
//             {
//                 printf("%u", val);
//             }
//             break;
//         }
//         case PARM_TYPE_FLOAT:     // Floating point.
//         {
//             printf("%f", parm->data.f);
//             break;
//         }
//         case PARM_TYPE_WORD:
//         {
//             printf("\"%s\"", parm->data.word);
//             break;
//         }
//         default:
//         {
//             printf("?%d?\n", parm->type);
//             break;
//         }
//     }
// }
//---------------------------------------------------------------------------------------------------------------------------------------
//
// void comp_buff_read_done(read_buff_t* buff)
// {
//     printf("%s: bigbuff: '%s'\n", __func__, bigbuff);
//     //int len = strlen(bigbuff);

//     const char* bptr = skip_first_line(bigbuff);
//     int len1 = strlen(bptr);

//     int checksum = calc_checksum(bptr, len1);

//     const char* cptr = skip_first_line(config_buff);
//     int len2 = strlen(cptr);

//     if (len1 != len2) { printf("%s: lengths do not match!  len %d, len %d\n", __func__, len1, len2); }

//     int match = strncmp(bptr, cptr, len2);

//     printf("%s: match %d\n", __func__, match);

//     snprintf(bigbuff, sizeof(bigbuff), "Result: %d, Size: %d, Check: %d\n", match, len1, checksum);
//     post_output(bigbuff, strlen(bigbuff));
// }


// PRIVATE void do_comp_command(matcher_item_t*match, size_t num_params, atom_t* params)
// {
//     if (  (num_params >= 2)
//        && (params[0].type == PARM_TYPE_BYTE || params[0].type == PARM_TYPE_UINT)
//        && (params[1].type == PARM_TYPE_BYTE || params[1].type == PARM_TYPE_UINT) )
//     {
//         size_t len = params[0].data.ui;
//         uint8_t* ptr = bigbuff;
//         size_t buff_size = params[0].data.ui;

//         if (buff_size > sizeof(bigbuff))
//             buff_size = sizeof(bigbuff);
//         if (buff_size == 0)
//             buff_size = 1;   // ensure snprintf writes a null

//         snprintf(bigbuff, buff_size, "ABLE %d %d\n", len, params[1].data.ui);

//         size_t slen = strlen(bigbuff);

//         ptr += slen;

//         if (slen >= sizeof(bigbuff) || buff_size > sizeof(bigbuff) - slen - 1)
//         {
//             buff_size = sizeof(bigbuff) - slen - 1;
//         }

//         printf("%s: Start to read buffer.  Size %d\n", __func__, buff_size);

//         Parser_Set_Read_Buffer(ptr, buff_size, comp_buff_read_done);
//     }
//     else
//     {
//         const char* p = NULL;

//              if (params[0].type == PARM_TYPE_UNDEFINED)                                { p = "Expecting a size!\n";         }
//         else if (params[0].type != PARM_TYPE_BYTE && params[0].type != PARM_TYPE_UINT) { p = "Size must be integer!\n";     }
//         else if (params[1].type == PARM_TYPE_UNDEFINED)                                { p = "Expecting a checksum!\n";     }
//         else if (params[1].type != PARM_TYPE_BYTE && params[1].type != PARM_TYPE_UINT) { p = "Checksum must be integer!\n"; }
//         else                                                                           { p = "unknown error\n";             }

//         post_output(p, strlen(p));
//     }
// }

/*
// PRIVATE const uint8_t* config_buff = "\
// // ABLE 76 5286\n\
// // ADV ABLEBIT111\n\
// // PIN LED 13 12 11 0 led1 My second led\n\
// // PIN IN 16 in1 input one";
*/

// //---------------------------------------------------------------------------------------------------------------------------------------
// //
// PRIVATE void test_buff(const char* buff)
// {
//     printf("%s: buff: '%s'\n", __func__, buff);

//     const char* ptr = skip_first_line(buff);
    
//     int len = strlen(ptr);
//     int i = calc_checksum(ptr, len);

//     bool result = Process_Config(buff, false);

//     snprintf(bigbuff, sizeof(bigbuff), "Result: %d, Size: %d, Check: %d\n", result, len, i);
//     post_output(bigbuff, strlen(bigbuff));
// }


// //---------------------------------------------------------------------------------------------------------------------------------------
// //
// PRIVATE void do_bcheck_command(matcher_item_t*match, size_t num_params, atom_t* params)
// {
//     test_buff(config_buff);
// }


    // {false, "bcheck", 0, ALT(MATCH_CODE_BCHECK), do_bcheck_command, NIL},
    // {false, "comp",   0, ALT(MATCH_CODE_COMP  ), do_comp_command,   NIL},
    // {false, "test",   0, ALT(MATCH_CODE_TEST  ), do_test_command,   NIL},
