// File: Parser.c

#include "common.h"
#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blob.h"
#include "beng.h"
#include "flashblob.h"
#include "btstdio.h"
#include "debug.h"
#include "led.h"
#include "matcher.h"
#include "scene.h"
#include "spi.h"


#define MAX_VARNAME_SIZE 40

#define CHAR_TIMEOUT 50000

#define MAX_BRIGHTNESS_NUM 1000 // 100.0 %

uint64_t read_bytes_retry_counter = 0; // Number of retries during read_bytes... ever!

#define NUM_RETRIES 5 // Maximum number of consecutive retries for a single call to read_bytes.

#define parser_getchar BlueTooth_TryGetChar

volatile int parser_engine_idx = 0;


PRIVATE bool read_bytes(int n, uint8_t *buff)
{
    int retries = NUM_RETRIES;

    while (n)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            *buff++ = ch;
            --n;
            retries = NUM_RETRIES;
        }
        else if (retries--)
        {
            read_bytes_retry_counter++;
            sleep_ms(3);
            continue;
        }
        else
        {
            BTPRINTF("!!! Blob read TIMEOUT expecting %d more bytes, total retries %lld.\n",
                     n, read_bytes_retry_counter);
            break;
        }
    }

    return n == 0; // True if all bytes were read.
}


PRIVATE bool blob_loader(void)
{
    uint8_t *base = Blob_Base_Get_New();

    if (base)
    {
        D(DEBUG_PARSER, PRINTF("\nread_blob: ");)

        BLOB_RAW *blob_raw = (BLOB_RAW *)base;

        blob_raw->Cookie = BLOB_COOKIE;

        uint8_t *buff = base + BLOB_COOKIE_SIZE;

        if (read_bytes(BLOB_PRE_HEADER_SIZE, buff))
        {
            if (blob_raw->Version == Version())
            {
                uint32_t blob_size = blob_raw->Size * sizeof(uint32_t);

                D(DEBUG_PRINTF, PRINTF(" size %d\n", blob_size);)

                if (blob_size < (MAX_BLOB_SIZE - BLOB_PRE_HEADER_SIZE - 4))
                {
                    buff += BLOB_PRE_HEADER_SIZE;

                    if (read_bytes(blob_size, buff))
                    {
                        uint32_t blob_check = Checksum(buff, blob_size);

                        if (blob_raw->Checksum == blob_check)
                        {
                            return true;
                        }
                        else { BTPRINTF("!!! Bad checksum: expected %X, got %X\n", blob_check, blob_raw->Checksum); }
                    }
                    else { BTPRINTF("!!! Can't read (%d) blob bytes!\n", blob_size); }
                }
                else { BTPRINTF("!!! Bab blob size (%d) must be less than %d\n", blob_size, MAX_BLOB_SIZE); }
            }
            else
            {
                char s1[10], s2[10];
                BTPRINTF("!!! Bad blob version: expected '%s', got '%s'\n",
                         version_to_str(s1, Version()),
                         version_to_str(s2, blob_raw->Version));
            }
        }
    }
    BTPRINTF("!!! BLOB load load fail.\n\n");

    return false;
}


PRIVATE void skip_white(void)
{
    while (true)
    {
        uint8_t val;

        if (BlueTooth_TryGetPeek(&val) && isblank(val))
        {
            (void)parser_getchar();   // Consume whitespace.
        }
        else break;
    }
}


PRIVATE int read_var_name(char* buff, size_t buff_size)
{
    skip_white();

    int count = 0;

    bool reading = true;
    bool is_first = true;

    if (buff_size) --buff_size;   // always leave room for final null.

    while (reading && count < buff_size)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            if ( ch == '_' || (is_first && isalpha(ch)) || (!is_first && isalnum(ch)) )
            {
                is_first = false;
                *buff++ = ch;
                *buff = 0;
                ++count;
                continue;
            }
        }
        else { D(DEBUG_PRINTF, PRINTF("read_var_name: TIMEOUT\n");) }

        reading = false;
    }
    return count;
}


PRIVATE int read_var_value(char* buff, size_t buff_size)
{
    skip_white();

    int count = 0;

    bool reading = true;
    bool first = true;

    if (buff_size) --buff_size;   // always leave room for final null.

    while (reading && count < buff_size)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            if (  (first && (ch == '+' || ch == '-'))
                 || isalnum(ch) || ch == '.')
            {
                *buff++ = ch;
                *buff = 0;
                ++count;
                continue;
            }
        }
        else { D(DEBUG_PRINTF, PRINTF("read_var_value: TIMEOUT\n");) }

        reading = false;
    }
    return count;
}


PRIVATE int read_num(void)
{
    skip_white();

    int val = 0;

    bool reading = true;

    while (reading)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            if (isdigit(ch))
            {
                val = (val * 10) + (ch - '0');
                continue;
            }
        }
        else { D(DEBUG_PRINTF, PRINTF("read_num: TIMEOUT\n");) }

        reading = false;
    }
    return val;
}


PRIVATE int read_snum(void) // Read signed number.
{
    skip_white();

    bool isfirst = true;
    bool isneg = false;
    int val = 0;

    bool reading = true;

    while (reading)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            if (isfirst && ch == '-')
            {
                isneg = true;
            }
            else if (isdigit(ch))
            {
                val = (val * 10) + (ch - '0');
            }
            else { break; }

            isfirst = false;
            continue;
        }
        else { D(DEBUG_PRINTF, PRINTF("read_snum: TIMEOUT\n");) }

        reading = false;
    }
    return isneg ? (val ? -val : -1) : val;
}


PRIVATE int read_hnum(void) // Read hex number.
{
    skip_white();

    int val = 0;

    bool reading = true;

    while (reading)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            if (isxdigit(ch))
            {
                ch = tolower(ch);
                ch = (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10 : ch - '0';
                val = (val << 4) + ch;
            }
            else { break; }

            continue;
        }
        // else { PRINTF("read_hnum: TIMEOUT\n"); }

        reading = false;
    }
    return val;
}


typedef enum parse_arg
{
    PARSE_ARG_NONE,
    PARSE_ARG_ANY,
    PARSE_ARG_INT,
    PARSE_ARG_FLOAT,
    PARSE_ARG_PROG,
    PARSE_ARG_VARNAME

} PARSE_ARG;


typedef struct parser_command
{
    MATCH_CODE code;
    PARSE_ARG one;
    PARSE_ARG two;

} PARSER_COMMAND;


PARSER_COMMAND parse_args[] =   // This table must have excatly the same entries as MATCH_CODE table.
{
    { MATCH_NONE,          PARSE_ARG_NONE, PARSE_ARG_NONE },         // Must be first.

    { MATCH_RESET,         PARSE_ARG_NONE, PARSE_ARG_NONE },        // Stop all engines and set to no blob loaded.
    { MATCH_BLACK,         PARSE_ARG_NONE, PARSE_ARG_NONE },        // All leds black (stop running blob and clears queue)

// - Variables.

    { MATCH_GETV,          PARSE_ARG_VARNAME, PARSE_ARG_NONE },         // Get variable.
    { MATCH_SETV,          PARSE_ARG_VARNAME, PARSE_ARG_NONE },         // Set variable.
    { MATCH_SETQ,          PARSE_ARG_VARNAME, PARSE_ARG_NONE },         // Set variable (quiet).

// - Blobs.

    { MATCH_LOAD_BLOB,     PARSE_ARG_NONE, PARSE_ARG_NONE },     // Loads a new blob.  Start running.
    { MATCH_STAGE_BLOB,    PARSE_ARG_NONE, PARSE_ARG_NONE },    // Loads a new blob.  Waits for activate to start running.
    { MATCH_ACTIVATE_BLOB, PARSE_ARG_NONE, PARSE_ARG_NONE }, // Start running new blob.
    { MATCH_EXPORT_BLOB,   PARSE_ARG_NONE, PARSE_ARG_NONE },   // Export current blob.

    { MATCH_LOAD,          PARSE_ARG_INT, PARSE_ARG_NONE },          // Load flash blob to RAM.
    { MATCH_LOAD_WAIT,     PARSE_ARG_INT, PARSE_ARG_NONE },     // Load flash blob to RAM.  Wait for activate to start running.
    { MATCH_SAVE,          PARSE_ARG_INT, PARSE_ARG_NONE },          // Save RAM blob to flash.
    { MATCH_ERASE,         PARSE_ARG_INT, PARSE_ARG_NONE },         // Erase flash page.

// Scenes.

    { MATCH_SCENE,         PARSE_ARG_NONE, PARSE_ARG_NONE },         // Set leds to scene from terminal.
    { MATCH_SHOW,          PARSE_ARG_NONE, PARSE_ARG_NONE },          // Set leds to scene from loaded blob.
    { MATCH_UPDATE,        PARSE_ARG_NONE, PARSE_ARG_NONE },        // Update leds that need updaing.

// Events.

    { MATCH_TRIGGER,      PARSE_ARG_NONE, PARSE_ARG_NONE },       // Trigger a program to start running immediately.
    { MATCH_INTERRUPT,    PARSE_ARG_NONE, PARSE_ARG_NONE },     // Halt running program, run new program, then resume halted program.
    { MATCH_QUEUE,        PARSE_ARG_NONE, PARSE_ARG_NONE },         // Queue a program to start running after current program completes.
 
// Instrumentation.

    { MATCH_DUMP,         PARSE_ARG_INT, PARSE_ARG_NONE },          // Dump debug info.

// Dregs.

    { MATCH_SPHY,         PARSE_ARG_NONE, PARSE_ARG_NONE },          // Set the current led array to use.

    { MATCH_LAST,         PARSE_ARG_NONE, PARSE_ARG_NONE }           // Must be last.
};



// ---------------------------------------------------------------------------------------------------------------------

#define MAX_EXPORT_BUFF_SIZE 512   //1024
#define PREAMBLE_STR "BLOB" BLOB_VERSION
#define PREAMBLE_SIZE 8


PRIVATE void do_export()
{
    if (Blob.Blob_BASE)
    {
        BLOB_RAW *blob_raw = Blob_Base_Current();
        uint8_t *ptr = (uint8_t *)blob_raw;

        size_t size = (BLOB_COOKIE_SIZE + blob_raw->Size) * sizeof(uint32_t);

        while (size)
        {
            size_t to_send =  (size < MAX_EXPORT_BUFF_SIZE) ? size : MAX_EXPORT_BUFF_SIZE;

            BlueTooth_Send_Buffer(ptr, to_send);
            ptr += to_send;
            size -= to_send;
        }
    }
    else { BTPRINTF("<<NONE>>\n"); }
}


PUBLIC bool BPage_Load_Blob(int bpage)
{
    if (BPAGE_VALID(bpage))
    {
        if (!BPage_Is_Blank(bpage))
        {
            if (BPage_Verify_Checksum(bpage))
            {
                uint8_t *base = (uint8_t *)Blob_Base_Get_New();

                BPage_Read_Blob(bpage, base);

                if (Blob_Verify_Checksum(base))
                {
                    return true;
                }
                else { BTPRINTF("LOAD: Bad checksum after load.\n"); }
            }
            else { BTPRINTF("LOAD: Bad checksum on page %d\n", bpage); }
        }
        else { BTPRINTF("LOAD: Page %d is blank!\n", bpage); }
    }
    else { BTPRINTF("LOAD: Bad page number (%d) must be (0->15)\n", bpage); }

    return false;
}


PRIVATE bool do_BPage_Save(int bpage)
{
    if (BPAGE_VALID(bpage))
    {
        if (!BPage_Is_Blank(bpage)) { BPage_Erase_Page(bpage); }

        BPage_Write_Blob(bpage, (uint8_t *)Blob_Base_Current());
        BTPRINTF("SAVED to page %d\n", bpage);
        return true;
    }
    else { BTPRINTF("SAVE: Bad page number must be (0->15)\n"); }

    return false;
}


PRIVATE void do_set_scene(SCENE_ID scene_id)
//
// Send a set scene command to current engine.
{
    BENG_STATE *bs = Get_Beng_State(parser_engine_idx);

    if (bs) { Set_Scene_mask(bs->phy_idx, scene_id); }
}


PRIVATE void parser(int ch)
{
    MATCH_CODE code = Is_Match(ch);

    if (code)
    {
        char var_name[MAX_VARNAME_SIZE];
        int arg_int = 0;

        if (code < MATCH_LAST)
        {
            PARSER_COMMAND* args = parse_args + code;

            switch (args->one)
            {
                case PARSE_ARG_INT:
                {
                    arg_int = read_num();
                    break;
                }
                case PARSE_ARG_VARNAME:
                {
                    arg_int = read_var_name(var_name, sizeof(var_name));
                    break;
                }
            }
        }   

        switch (code)
        {
        case MATCH_RESET: // Stop all engines, Black leds, and set to no blob.
        {
            LEDS_All_Black();
            Blob_Unload();
            BTPRINTF("RESE\n");
        }
        case MATCH_BLACK: // BLAC: Set all leds to black.
        {
            Beng_All_Stop(); // Stop all Blob engines.
            LEDS_All_Black();
            BTPRINTF("BLAC\n");
            break;
        }
//------
        case MATCH_LOAD_BLOB: // BLOB: Load a new binary object containing program and start running.
        {
            blob_loader();
            Blob_Activate(parser_engine_idx); 
            break;
        }
        case MATCH_STAGE_BLOB: // BLST: Load a new binary object containing program.  Must call activate to start running.
        {
            blob_loader();
            break;
        }
        case MATCH_ACTIVATE_BLOB:  
        {
            Blob_Activate(parser_engine_idx); 
            break;
        }
        case MATCH_EXPORT_BLOB:
        {
            do_export();
            break;
        }
//------
        case MATCH_SCENE:
        {
            D(DEBUG_PARSER, PRINTF("parser: MATCH_SCENE is not implemented\n");)
        //     read_scene();
            break;
        }
        case MATCH_SHOW:
        {
            arg_int = read_num();
            D(DEBUG_PARSER, PRINTF("DO SHOW SCENE %d\n", arg_int);)
            do_set_scene(arg_int);
            LEDS_Do_Update();
            break;
        }
        case MATCH_UPDATE: // UPDA: Update and leds that need updating.
        {
            LEDS_Do_Update();
            PRINTF("UPDA\n");
            break;
        }
//------
        case MATCH_TRIGGER:
        {
            arg_int = read_num();
            BTPRINTF("TRIG %d\n", arg_int);
            Blob_Trigger(parser_engine_idx, arg_int);
            break;
        }
        case MATCH_INTERRUPT:
        {
            D(DEBUG_PARSER, PRINTF("parser: MATCH_INTERRUPT is not implemented\n");)
            break;
        }
        case MATCH_QUEUE:
        {
            D(DEBUG_PARSER, PRINTF("parser: MATCH_QUEUE is not implemented\n");)
        //     arg = read_num();
        //     PRINTF("QUEU %d\n", arg);         // SAME as trig (for now).
        //     Blob_Queue_Next(arg);
            break;
        }
//------
        case MATCH_SETV:
        {
            if (arg_int)
            {
                BENG_VAR* var = BVar_Find_By_Name(NULL, var_name);

                D(DEBUG_PARSER, PRINTF("parser: MATCH_SETV '%s' [%X]\n", var_name, var);)

                if (var)
                {
                    arg_int = read_var_value(var_name, sizeof(var_name));

                    if (arg_int)
                    {
                        bool b = BVar_From_String(var, var_name);

                        if (b) { BTPRINTF("%s = %s\n", var->name, var_name); }
                        else   { BTPRINTF("%s = %s (fail)\n", var->name, var_name); }
                    }
                    else
                    {
                        char buff[20];

                        BVar_To_String(var, buff, sizeof(buff));
                        BTPRINTF("%s unchanged (%s)\n", var_name, buff);
                    }
                }
            }
            break;
        }
        case MATCH_SETQ:
        {
            if (arg_int)
            {
                BENG_VAR* var = BVar_Find_By_Name(NULL, var_name);

                if (var)
                {
                    arg_int = read_var_value(var_name, sizeof(var_name));

                    if (arg_int) { BVar_From_String(var, var_name); }
                }
            }
            break;
        }
        case MATCH_GETV:
        {
            if (arg_int)
            {
                BENG_VAR* var = BVar_Find_By_Name(NULL, var_name);

                D(DEBUG_PARSER, PRINTF("parser: MATCH_GETV '%s' [%X]\n", var_name, var);)

                if (var)
                {
                    char buff[20];
                    BVar_To_String(var, buff, sizeof(buff));
                    BTPRINTF("%s = %s\n", var_name, buff);
                }
            }
            break;
        }
//------
        case MATCH_LOAD: // BLOB: Load a new binary object from flash page and start running.
        {
//            int num = read_num();

            if (BPAGE_VALID(arg_int))
            {
                if (BPage_Load_Blob(arg_int)) 
                {
                    Blob_Activate(parser_engine_idx);
                    BTPRINTF("Page %d loaded and running.\n", arg_int);
                }
                else { BTPRINTF("Page %d can't load!\n", arg_int); }
            }
            else { BTPRINTF("LOAD: Bad page number must be (0->15)\n"); }
            break;
        }
        case MATCH_LOAD_WAIT: // BLOB: Load a new binary object containing program and wait for activate.
        {
//            int num = read_num();

            if (BPAGE_VALID(arg_int))
            {
                if (BPage_Load_Blob(arg_int)) { BTPRINTF("Page %d loaded and ready.\n", arg_int); }
                else                          { BTPRINTF("Page %d can't load!\n", arg_int); }
            }
            else { BTPRINTF("LDWT: Bad page number must be (0->15)\n"); }
            break;
        }
        case MATCH_SAVE:
        {
//            int num = read_num();

            if (BPAGE_VALID(arg_int))
            {
                if (do_BPage_Save(arg_int)) { BTPRINTF("Blob save to page %d of flash.\n", arg_int); }
                else                        { BTPRINTF("Can't save blob to page %d of flash!\n", arg_int); }
            }
            else { BTPRINTF("SAVE: Bad page number must be (0->15)\n"); }
            break;
        }
        case MATCH_ERASE:
        {
//            int bpage = read_num();

            if (BPAGE_VALID(arg_int))
            {
                if (BPage_Erase_Page(arg_int)) { BTPRINTF("Flash page %d erased.\n", arg_int); }
                else                           { BTPRINTF("Can't erase flash page %d!\n", arg_int); }
            }
            else { BTPRINTF("ERAS: Bad page number must be (0->15)\n"); }
            break;
        }
//------
        case MATCH_DUMP:
        {
            int start = read_hnum(); // Try to read another arg.
            // PRINTF("Dump %d, start %X\n", arg, start);
            extern void do_dump(int arg, int arg2);
            do_dump(arg_int, start);
            break;
        }
//------
        case MATCH_SPHY: // Set the current led array to use.
        {
            int phy_mask = read_snum();

            Beng_Set_Phy(parser_engine_idx, phy_mask);
            BTPRINTF("SPHY %d\n", phy_mask);
            break;
        }
//------
        case MATCH_NONE:
        default:
        {
            break;
        }
        }

        Matchers_Reset();
    }
}


PUBLIC void Start_Parser()
{
    Matchers_Init();

    while (true)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT) {    parser(ch); }
        else if (Spi_Data_Ready())    { Spi_Process(); }
        else                          {   sleep_ms(1); }
    }
}


// EndFile: Parser.c