// File: Parser.c

#include "common.h"
#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "blob.h"
#include "btstdio.h"
#include "debug.h"
#include "led.h"
#include "matcher.h"
#include "scene.h"


#define CHAR_TIMEOUT 50000

#define MAX_BRIGHTNESS_NUM 1000     // 100.0 %

#define BLOB_PRE_HEADER_SIZE 12      // Number of bytes to read before reading header.

uint64_t read_bytes_retry_counter = 0;     // Number of retries during read_bytes... ever!

#define NUM_RETRIES 5   // Maximum number of consecutive retries for a single call to read_bytes.

#define parser_getchar BlueTooth_TryGetChar


PRIVATE bool read_bytes(int n, uint8_t* buff)
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
            D(DEBUG_PARSER, BTPRINTF("!!! Retry.\n"); )
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

    return n == 0;   // True if all bytes were read.
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


PRIVATE uint32_t Checksum(uint8_t* buff, size_t size)
{
    uint32_t check = 0;

    while (size--)
    {
        check += *buff++;
    }
    return check;
}


PRIVATE void read_blob(void)
{
    uint8_t* base = Get_New_Blob_Base();
    uint8_t* buff = base;

    if (base)
    {
        BLOB_PRE_HEADER* prehead = (void*)buff;
        D(DEBUG_PARSER, PRINTF("\nread_blob: ");)

        buff[0] = 'B'; buff[1] = 'L'; buff[2] = 'O'; buff[3] = 'B';
        buff += 4;

        if (read_bytes(BLOB_PRE_HEADER_SIZE, buff))
        {     
            if (prehead->Version == Version())
            {
                uint32_t blob_size = prehead->Size * sizeof(uint32_t);

                D(DEBUG_PRINTF, PRINTF(" size %d\n", blob_size);)

                if (blob_size < (MAX_BLOB_SIZE - BLOB_PRE_HEADER_SIZE - 4))
                {
                    buff += BLOB_PRE_HEADER_SIZE;

                    if (read_bytes(blob_size, buff))
                    {
                        uint32_t blob_check = Checksum(buff, blob_size);

                        if (prehead->Checksum == blob_check)
                        {
                            // PRINTF("read_blob: base %X %X\n", real_base, base);
                            if (Unpack_Blob_Header(buff))
                            {
                                BTPRINTF("BLOB loaded.\n");
                                return;
                            }
                        }
                        else { BTPRINTF("!!! Bad checksum: expected %X, got %X\n", blob_check, prehead->Checksum); }
                    }
               }
                else { BTPRINTF("!!! Bab blob size (%d) must be less than %d\n", blob_size, MAX_BLOB_SIZE); }
            }
            else 
            { 
                char s1[10], s2[10];
                BTPRINTF("!!! Bad blob version: expected '%s', got '%s'\n", 
                                    version_to_str(s1, Version()), 
                                    version_to_str(s2, prehead->Version)); 
            }
        }
    }
    BTPRINTF("!!! BLOB load load fail.\n\n");
}


#ifdef COMMENT
PRIVATE void read_blob(void)
{
    uint8_t buff[BLOB_PRE_HEADER_SIZE] = {0};
    uint32_t blob_size = 0;

    D(DEBUG_PARSER, PRINTF("\nread_blob: ");)

    if (read_bytes(BLOB_PRE_HEADER_SIZE, buff))
    {     
        uint32_t* ptr = (uint32_t*)buff;

        if (*ptr++ == Version())
        {
            blob_size = *ptr++ * sizeof(uint32_t);
            D(DEBUG_PRINTF, PRINTF(" size %d\n", blob_size);)

            if (blob_size < MAX_BLOB_SIZE)
            {
                uint8_t* base = Get_New_Blob_Base();  //malloc(blob_size);

                if (base)
                {
                    *(uint32_t*)base = blob_size;       // Copy blob_size into header.
                    base += sizeof(uint32_t);           // Move base past the size value.

                    if (read_bytes(blob_size, base))
                    {
                        uint32_t blob_check = Checksum(base, blob_size);
                        uint32_t check = 0;

                        if (read_bytes(sizeof(uint32_t), (uint8_t*)&check))
                        {
                            // PRINTF("check %d, blob_check %d\n", check, blob_check);

                            if (check == blob_check)
                            {
                                // PRINTF("read_blob: base %X %X\n", real_base, base);
                                if (Unpack_Blob_Header(base, check))
                                {
                                    BTPRINTF("BLOB loaded.\n");
                                    return;
                                }
                            }
                            else { BTPRINTF("!!! Bad checksum: expected %X, got %X\n", blob_check, check); }
                        }
                    }
                }
            }
            else { BTPRINTF("!!! Bab blob size (%d) must be less than %d\n", blob_size, MAX_BLOB_SIZE); }
        }
        else 
        { 
            char s1[10], s2[10];
            BTPRINTF("!!! Bad blob version: expected '%s', got '%s'\n", 
                                version_to_str(s1, Version()), 
                                version_to_str(s2, *(ptr-1))); 
        }
    }
    BTPRINTF("!!! BLOB load load fail.\n\n");
}
#endif // COMMENT


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
        else { D(DEBUG_PRINTF, PRINTF("read_num: TIMEOUT\n");) }

        reading = false;
    }
    return val;
}


PRIVATE int read_snum(void)   // Read signed number.
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
            else { break; }

            isfirst = false;
            continue;
        }
        else { D(DEBUG_PRINTF, PRINTF("read_snum: TIMEOUT\n");) }

        reading = false;
    }

    return isneg ? (val ? -val : -1) : val;
}


PRIVATE int read_hnum(void)   // Read hex number.
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
            else { break; }

            continue;
        }
        // else { PRINTF("read_hnum: TIMEOUT\n"); }

        reading = false;
    }

    return val;
}

#define MAX_BUFF_SIZE 1024

PRIVATE void do_export()
{
    if (Blob.Blob_Base)
    {
        static char buff[MAX_BUFF_SIZE];
        uint8_t* ptr = Get_Blob_Base();
        BLOB_PRE_HEADER* pre_header = (BLOB_PRE_HEADER*)ptr;

//        size_t size = Blob.Blob_Size;
        size_t size = pre_header->Size * sizeof(uint32_t);

        char* bufp = buff;
        char* endp = buff + sizeof(buff);

        while (size--) 
        {
            bufp += sprintf(bufp, "%2.2X", *ptr++);

            if (bufp > endp)
            {
                BlueTooth_Send_String(buff);
                bufp = buff; 
                *bufp = 0;
            }
        }
        BlueTooth_Send_String(buff);
    }
    else { BTPRINTF("<<NONE>>\n"); }
}


#ifdef COMMENT
PRIVATE void do_export()
{
    if (Blob.Blob_Base)
    {
        static char buff[MAX_BUFF_SIZE];
        uint8_t*ptr = Blob.Blob_Base;
        size_t size = Blob.Blob_Size;

        // BTPRINTF("Size: %d\n", size);
        char strVersion[10];
        version_to_str(strVersion, Version());

        char* bufp = buff;
        char* endp = buff + MAX_BUFF_SIZE - 8;

        bufp += sprintf(bufp, "BLOB%s", strVersion);

        while (size--) 
        {
            bufp += sprintf(bufp, "%2.2X", *ptr++);

            if (bufp > endp)
            {
                BlueTooth_Send_String(buff);
                bufp = buff; 
            }
        }
        BlueTooth_Send_String(buff);
    }
    else { BTPRINTF("<<NONE>>\n"); }
}
#endif // COMMENT

PRIVATE void scan_for_sync(int ch)
{
    MATCH_CODE code = Is_Match(ch);
    int arg;

    if (code)
    {
        switch (code)
        {
            case MATCH_BLACK:           // BLAC: Set all leds to black.
            {
                Blob_Stop();
                LEDS_All_Black();
                BTPRINTF("BLAC\n");
                break;
            }
            case MATCH_UPDATE:          // UPDA: Updatre and leds that need updating.
            {
                LEDS_Do_Update();
                PRINTF("UPDA\n");
                break;
            }
            case MATCH_SET_BLOB:            // BLOB: Load a new binary object containing program.
            {
                PRINTF("READ\n");
//                BTPRINTF("READ\n");
                read_blob();
                break;
            }
            case MATCH_GET_BLOB:
            {
                do_export();
                break;
            }
            case MATCH_TRIGGER:
            {
                arg = read_num();
                Blob_Trigger(arg);
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
            case MATCH_SPHY:          // Set the current led array to use.
            {
                int phynum = read_snum();

                LEDS_Set_Phynum(phynum);
                BTPRINTF("SPHY %d\n", phynum);
                break;
            }
            case MATCH_SHOW:
            {
                arg = read_num();
                D(DEBUG_PARSER, PRINTF("DO SHOW SCENE %d\n", arg);)
                Set_Scene(arg);
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
                int start = read_hnum();  // Try to read another arg.
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
    Matchers_Init();

    while (true) 
    { 
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
#ifdef COMMENT
            if (ch == 'a')
            {
                BTPRINTF("ABC %d\n", 123);                
            }
            else if (ch == 'b')
            {
                int i;
                for (i=0; i< 1000; ++i)
                    BTPRINTF("ABC %d\n", 123);                
            }
            else if (ch == 'c')
            {
                BTPRINTF("ABCDEFGHIJKLMNOPQRSTUVWXYZ");                
            }
            else if (ch == 'd')
            {
                BlueTooth_Send_String("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");                
            }
            else
            {
                BTPRINTF("BAD.\n");                
            }
        }
#endif
            scan_for_sync(ch); 
            sleep_ms(1); 
        }
    }
}


// EndFile: Parser.c