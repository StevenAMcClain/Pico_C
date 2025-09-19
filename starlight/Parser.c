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

#define CHAR_TIMEOUT 50000

#define MAX_BRIGHTNESS_NUM 1000 // 100.0 %

uint64_t read_bytes_retry_counter = 0; // Number of retries during read_bytes... ever!

#define NUM_RETRIES 5 // Maximum number of consecutive retries for a single call to read_bytes.

#define parser_getchar BlueTooth_TryGetChar


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
//            D(DEBUG_PARSER, BTPRINTF("!!! Retry.\n");)
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


// PRIVATE void read_scene(void)
// {
//     int scene_led_num = 0;
//     int scene_led_idx = 0;
//     uint8_t scene_led_val[LED_SIZE] = {0};

//     bool reading = true;

//     while (reading)
//     {
//         int ch = parser_getchar();

//         if (ch != PICO_ERROR_TIMEOUT)
//         {
//             scene_led_val[scene_led_idx++] = ch;

//             if (scene_led_idx == LED_SIZE)
//             {
//                 scene_led_idx = 0;
// // fix /////////////                // LED_Set_LED(scene_led_num, scene_led_val);

// //                if (++scene_led_num == Num_LEDS)
//                 {
// // fix /////////////                    led_send();
//                     reading = false;
//                 }
//             }
//         }
//         else
//         {
//             PRINTF("read_scene: TIMEOUT\n");
//             reading = false;
//         }
//     }
// }


PRIVATE bool read_blob(void)
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
                            Blob_Base_Switch();

                            // PRINTF("read_blob: base %X %X\n", real_base, base);
                            if (Unpack_Blob_Header(base))
                            {
                                BTPRINTF("BLOB loaded.\n");
                                return true;
                            }
                        }
                        else { BTPRINTF("!!! Bad checksum: expected %X, got %X\n", blob_check, blob_raw->Checksum); }
                    }
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


PRIVATE int read_num(void)
{
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
        else
        {
            D(DEBUG_PRINTF, PRINTF("read_num: TIMEOUT\n");)
        }

        reading = false;
    }
    return val;
}


PRIVATE int read_snum(void) // Read signed number.
{
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
            else
            {
                break;
            }

            isfirst = false;
            continue;
        }
        else
        {
            D(DEBUG_PRINTF, PRINTF("read_snum: TIMEOUT\n");)
        }

        reading = false;
    }

    return isneg ? (val ? -val : -1) : val;
}


PRIVATE int read_hnum(void) // Read hex number.
{
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
            else
            {
                break;
            }

            continue;
        }
        // else { PRINTF("read_hnum: TIMEOUT\n"); }

        reading = false;
    }

    return val;
}


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
            if (size < MAX_EXPORT_BUFF_SIZE)
            {
                BlueTooth_Send_Buffer(ptr, size);
                size = 0;
            }
            else
            {
                BlueTooth_Send_Buffer(ptr, MAX_EXPORT_BUFF_SIZE);
                ptr += MAX_EXPORT_BUFF_SIZE;
                size -= MAX_EXPORT_BUFF_SIZE;
            }
        }
    }
    else
    {
        BTPRINTF("<<NONE>>\n");
    }
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
                    Blob_Base_Switch();

                    if (Unpack_Blob_Header(base))
                    {
                        //                        BTPRINTF("LOAD: Success.\n");
                        // ---------------------------------------------------------        Start running blob.
                        return true;
                    }
                    else { BTPRINTF("LOAD: Can't unpack.\n"); }
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


PRIVATE bool BPage_Save_Blob(int bpage)
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


PRIVATE void Set_Scene_engines(int beng_mask, SCENE_ID scene_id)
//
// Shift (or rotate) led array on one phy.
{
    int beng_idx = 0;
    uint32_t mask = 1;

    while (beng_mask && beng_idx < MAX_BENG)
    {
        if (mask & beng_mask)
        {
            BENG_STATE *bs = Get_Beng_State(beng_idx);
            Set_Scene(bs->phy_mask, scene_id);
            beng_mask &= ~mask;
        }
        mask <<= 1;
        ++beng_idx;
    }
}


PRIVATE void parser(int ch)
{
    static int engine_mask = 1;
    MATCH_CODE code = Is_Match(ch);
    int arg;

    if (code)
    {
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
        case MATCH_UPDATE: // UPDA: Update and leds that need updating.
        {
            LEDS_Do_Update();
            PRINTF("UPDA\n");
            break;
        }
        case MATCH_SET_BLOB: // BLOB: Load a new binary object containing program.
        {
            PRINTF("BLOB\n");

            if (read_blob())
            {
                //=== Start...
                Blob_Run_mask(engine_mask, 1);
            }
            break;
        }
        case MATCH_EXPORT_BLOB:
        {
            do_export();
            break;
        }
        case MATCH_SAVE:
        {
            BPage_Save_Blob(read_num());
            break;
        }
        case MATCH_LOAD:
        {
            if (BPage_Load_Blob(read_num()))
            {
                //=== Start...
                Blob_Run_mask(engine_mask, 1);
            }
            break;
        }
        case MATCH_ERASE:
        {
            int bpage = read_num();

            if (BPAGE_VALID(bpage))
            {
                BTPRINTF("Page %d erased.\n", arg);
                BPage_Erase_Page(bpage);
            }
            else
            {
                BTPRINTF("ERAS: Bad page number must be (0->15)\n");
            }
            break;
        }
        case MATCH_TRIGGER:
        {
            arg = read_num();
            BTPRINTF("TRIG %d\n", arg);
            Blob_Trigger_mask(engine_mask, arg);
            BTPRINTF("TRIG %d\n", arg);
            break;
        }
        case MATCH_BRIGHTNESS:
        {
            arg = read_num();
            D(DEBUG_PRINTF, { PRINTF("BRIG %d \r", arg); fflush(stdout); })

            if (arg >= 0 && arg <= MAX_BRIGHTNESS_NUM)
            {
                double newval = arg / (double)MAX_BRIGHTNESS_NUM;

                if (LED_Brightness != newval)
                {
                    LED_Brightness = newval;
                    LED_Needs_Update(ALL_PHYS);
                    LEDS_Do_Update();
                }
            }
            break;
        }
        case MATCH_SENG: // Set the active engine mask.
        {
            arg = read_snum();

            if (arg >= 0 && arg < 128)
            {
                engine_mask = arg;
                BTPRINTF("SENG %d\n", arg);
            }
            else
            {
                BTPRINTF("SENG: engine mask must be (0->127)\n");
            }
            break;
        }
        case MATCH_SPHY: // Set the current led array to use.
        {
            int phy_mask = read_snum();

            Beng_Set_Phy_mask(engine_mask, phy_mask);
            BTPRINTF("SPHY %d\n", phy_mask);
            break;
        }
        case MATCH_SHOW:
        {
            arg = read_num();
            D(DEBUG_PARSER, PRINTF("DO SHOW SCENE %d\n", arg);)
            Set_Scene_engines(engine_mask, arg);
            LEDS_Do_Update();
            break;
        }
        // case MATCH_QUEUE:
        // {
        //     arg = read_num();
        //     PRINTF("QUEU %d\n", arg);         // SAME as trig (for now).
        //     Blob_Queue_Next(arg);
        //     break;
        // }
        // case MATCH_SCENE:
        // {
        //     PRINTF("SCEN\n");
        //     read_scene();
        //     break;
        // }
        case MATCH_DUMP:
        {
            arg = read_num();
            int start = read_hnum(); // Try to read another arg.
            // PRINTF("Dump %d, start %X\n", arg, start);
            extern void do_dump(int arg, int arg2);
            do_dump(arg, start);
            break;
        }
        case MATCH_DEBUG:
        {
            arg = read_num();
            PRINTF("Set Debug Flag %d(0x%X)\n", arg, arg);
            Debug_Mask = arg;
            break;
        }
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
    // This should be controlled by port pin.
    // if (BPage_Load_Blob(0))
    // {
    //     Blob_Run_mask(1, 1);        //=== Start...
    // }

    Matchers_Init();

    while (true)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT) { parser(ch); }
        else                          { sleep_ms(1);    }
    }
}


// EndFile: Parser.c