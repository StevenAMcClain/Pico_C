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
#include "obled.h"
#include "scene.h"

extern void do_dump(int arg, int arg2);

extern volatile bool is_connected;
extern volatile bool do_pair_flash;


#define CHAR_TIMEOUT 50000

#define MAX_BRIGHTNESS_NUM 1000     // 100.0 %

#define BLOB_PRE_HEADER_SIZE 8      // Number of bytes to read before reading header.

uint64_t read_bytes_retry_counter = 0;     // Number of retries during read_bytes... ever!

#define NUM_RETRIES 5   // Maximum number of consecutive retries for a single call to read_bytes.



PUBLIC uint32_t str_to_uint32(char* str)
{
    return str[3] << 24 | str[2] << 16 | str[1] << 8 | str[0];
}

PUBLIC char* uint32_to_str(char* buff, uint32_t val)
{
    buff[0] = val & 0xFF;
    buff[1] = (val >> 8) & 0xFF;
    buff[2] = (val >> 16) & 0xFF;
    buff[3] = (val >> 24) & 0xFF;
    buff[4] = 0;

    return buff;
}

#define Blobhead() str_to_uint32("BLOB")
#define Version() str_to_uint32(BLOB_VERSION)

// ----------------------------------------  Parser input line buffers - start

#define LINE_SIZE 256
uint8_t line_one[LINE_SIZE] = {0};
uint8_t line_two[LINE_SIZE] = {0};

uint8_t* line_buffer = line_two;
volatile uint8_t* line_pointer = line_two;
size_t chars_ready = 0;


#define PARSER_GETCHAR_EMPTY 1234



PRIVATE int parser_getchar()
{
    int ch = PARSER_GETCHAR_EMPTY;

    if (is_connected && chars_ready)
    {
        ch = *line_pointer++;
        chars_ready--;
    }
    return ch;
}


PRIVATE void parser_setup_readline()
{
    line_pointer = line_buffer = (line_buffer == line_two) ? line_one : line_two;
    chars_ready = 0;
    Bt_Make_Request(line_buffer, LINE_SIZE);
}


PRIVATE void parser_wait_readline()
{
    while (is_connected && !chars_ready)
    {
        sleep_ms(1); 

        chars_ready = Bt_ChecK_Request();

        if (chars_ready)
        {
            line_pointer = line_buffer;

            printf("btchars ready %d\n", chars_ready);
            // dump_ascii((uint8_t*)line_pointer, chars_ready);
        }
    }
}

// ----------------------------------------  Parser input line buffers - end


PRIVATE bool read_bytes(uint8_t* buff, size_t size)
{
    while (size)
    {
        int ch = parser_getchar();

        if (ch != PARSER_GETCHAR_EMPTY)
        {
            *buff++ = ch;
            --size;
        }
        else
        {
            BlueTooth_Printf("!!! Blob read TIMEOUT expecting %d more bytes.\n", size); 
            break;
        }
    }

    return size == 0;   // True if all bytes were read.
}

#define MAX_MISS_COUNT 10

PUBLIC uint64_t total_miss_count = 0;

PRIVATE size_t read_chunk(uint8_t* buff, size_t size)
{
    uint64_t end_time = time_us_64() + 1000000;
    int miss_count = MAX_MISS_COUNT;
    size_t last_ready = 0;

    Bt_Make_Request(buff, size);
    chars_ready = 0;

    while (miss_count && chars_ready != size && time_us_64() < end_time)
    {
        sleep_ms(100); 

        chars_ready = Bt_ChecK_Request();

        if (chars_ready == last_ready) { --miss_count; ++total_miss_count; }
        else                             { miss_count = MAX_MISS_COUNT; }
    }

    last_ready = chars_ready;

    return last_ready;
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

//         if (ch != PARSER_GETCHAR_EMPTY)
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
//             printf("read_scene: TIMEOUT\n"); 
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


PRIVATE void load_blob(void)
{
    uint8_t* base = Get_New_Blob_Base();

    size_t size = read_chunk(base, MAX_BLOB_SIZE);

    if (size > BLOB_PRE_HEADER_SIZE)
    {
        uint32_t *aptr = (uint32_t*)base;

// size_t old_chars_ready = chars_ready;
// sleep_ms(1000); 
// chars_ready = Bt_ChecK_Request();
// if (old_chars_ready != chars_ready)
// {
//     printf("@@@@@@@@@ After delay chars ready %d -> %d @@@@@@@\n", old_chars_ready, chars_ready);
// }
// else
// {
//     printf("@ nada @\n");
// }

        if (*aptr++ == Blobhead())
        {
            if (*aptr++ == Version())
            {
                size_t blob_size = *aptr++ * sizeof(uint32_t);

                D(DEBUG_PRINTF, printf(" size %d\n", blob_size);)

                if (blob_size < MAX_BLOB_SIZE)
                {
                    uint32_t* bptr = aptr;

                    uint32_t blob_check = Checksum((uint8_t*)bptr, blob_size);
                    uint32_t *check = (uint32_t*)((uint8_t*)bptr + blob_size);

                    if (*check == blob_check)
                    {
                        // printf("read_blob: base %X %X\n", real_base, base);
                        if (Blob_Load((uint8_t*)bptr, blob_size, *check))
                        {
                            Set_New_Blob_Base();
                            BlueTooth_Printf("BLOB loaded.\n");
                            parser_setup_readline();
                            return;
                        }
                    // printf("So far ... so good\n");

                    // *(uint32_t*)base = blob_size;       // Copy blob_size into header.
                    // base += sizeof(uint32_t);           // Move base past the size value.
                    // printf("...read blob... start.\n");
                    // {
                    //     uint32_t blob_check = Checksum(base, blob_size);
                    //     uint32_t *check = (uint32_t*)(base + blob_size);

                    //     printf("...read blob... done.\n");
                    //     if (*check == blob_check)
                    //     {
                    }
                    else { BlueTooth_Printf("!!! Bad checksum: expected %X, got %X\n", blob_check, *check); }
                }
                else { BlueTooth_Printf("!!! Bab blob size (%d) must be less than %d\n", blob_size, MAX_BLOB_SIZE); }
            }
            else 
            { 
                char s1[10], s2[10];
                BlueTooth_Printf("!!! Bad blob version: expected '%s', got '%s'\n", 
                                    uint32_to_str(s1, Version()), 
                                    uint32_to_str(s2, *(aptr-1))); 
            }
        }
        else 
        { 
            char s1[10], s2[10];
            BlueTooth_Printf("!!! Bad blob: expected '%s', got '%s'\n", 
                                uint32_to_str(s1, Blobhead()), 
                                uint32_to_str(s2, *(aptr-1))); 
        }
        parser_setup_readline();
    }

    // blob_size + sizeof(uint32_t)

    // uint8_t buff[BLOB_PRE_HEADER_SIZE] = {0};
    // uint32_t blob_size = 0;

    // D(DEBUG_PARSER, printf("\nload_blob: ");)

    // if (read_bytes(buff, BLOB_PRE_HEADER_SIZE))
    // {     
    //     uint32_t* ptr = (uint32_t*)buff;

    // }
    BlueTooth_Printf("!!! BLOB load load fail.\n\n");

    parser_setup_readline();
}



// PRIVATE void load_blob(void)
// {
//     uint8_t buff[BLOB_PRE_HEADER_SIZE] = {0};
//     uint32_t blob_size = 0;

//     D(DEBUG_PARSER, printf("\nload_blob: ");)

//     if (read_bytes(buff, BLOB_PRE_HEADER_SIZE))
//     {     
//         uint32_t* ptr = (uint32_t*)buff;

//         if (*ptr++ == Version())
//         {
//             blob_size = *ptr++ * sizeof(uint32_t);
//             D(DEBUG_PRINTF, printf(" size %d\n", blob_size);)

//             uint8_t* base = Get_New_Blob_Base();  //malloc(blob_size);

//             if (base && blob_size < MAX_BLOB_SIZE)
//             {
//                 *(uint32_t*)base = blob_size;       // Copy blob_size into header.
//                 base += sizeof(uint32_t);           // Move base past the size value.
//                 printf("...read blob... start.\n");
//                 if (read_chunk(base, blob_size + sizeof(uint32_t)))
//                 {
//                     uint32_t blob_check = Checksum(base, blob_size);
//                     uint32_t *check = (uint32_t*)(base + blob_size);

//                     printf("...read blob... done.\n");
//                     if (*check == blob_check)
//                     {
//                         // printf("read_blob: base %X %X\n", real_base, base);
//                         if (Unpack_Blob_Header(base, *check))
//                         {
//                             Set_New_Blob_Base();
//                             BlueTooth_Printf("BLOB loaded.\n");
//                             return;
//                         }
//                     }
//                     else { BlueTooth_Printf("!!! Bad checksum: expected %X, got %X\n", blob_check, *check); }
//                 }
//             }
//             else { BlueTooth_Printf("!!! Bab blob size (%d) must be less than %d\n", blob_size, MAX_BLOB_SIZE); }
//         }
//         else 
//         { 
//             char s1[10], s2[10];
//             BlueTooth_Printf("!!! Bad blob version: expected '%s', got '%s'\n", 
//                                 version_to_str(s1, Version()), 
//                                 version_to_str(s2, *(ptr-1))); 
//         }
//     }
//     BlueTooth_Printf("!!! BLOB load load fail.\n\n");
// }


PRIVATE int read_num(void)
{
    int val = 0;

    while (true)
    {
        int ch = parser_getchar();

        if (ch == PARSER_GETCHAR_EMPTY) { break; }

        if (isdigit(ch))
        {
            val = (val * 10) + (ch - '0');
            continue;
        }
         break;
    }
    return val;
}


PRIVATE int read_snum(void)   // Read signed number.
{
    int val = 0;
    bool isfirst = true;
    bool isneg = false;

    while (true)
    {
        int ch = parser_getchar();

        if (ch == PARSER_GETCHAR_EMPTY) { break; }

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

    return isneg ? (val ? -val : -1) : val;
}


PRIVATE int read_hnum(void)   // Read hex number.
{
    int val = 0;

    while (true)
    {
        int ch = parser_getchar();

        if (ch == PARSER_GETCHAR_EMPTY) { break; }

        if (isxdigit(ch))
        {
            ch = tolower(ch);
            ch = (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10 : ch - '0';
            val = (val << 4) + ch;
            continue;
        }
        break;
    }

    return val;
}

#define MAX_BUFF_SIZE 1000

PRIVATE void do_export()
{
    if (Blob.Blob_Base)
    {
        uint8_t*ptr = Blob.Blob_Base;
        size_t size = Blob.Blob_Size;

        // BlueTooth_Printf("Size: %d\n", size);
        char s1[10];
        uint32_to_str(s1, Version());

        char buff[MAX_BUFF_SIZE];
        char* bufp = buff;
        char* endp = buff + MAX_BUFF_SIZE - 8;

        bufp += sprintf(bufp, "BLOB%s", s1);

        while (size--) 
        {
            bufp += sprintf(bufp, "%2.2X", *ptr++);

            if (bufp > endp)
            {
                *bufp = 0;
                BlueTooth_Send_String(buff);
                bufp = buff; 
            }
        }
        *bufp = 0;
        BlueTooth_Send_String(buff);
    }
    else { BlueTooth_Printf("<<NONE>>\n"); }
}


PRIVATE void parse_and_execute_commands(int ch)
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
                BlueTooth_Printf("BLAC\n");
                break;
            }
            case MATCH_UPDATE:          // UPDA: Updatre and leds that need updating.
            {
                LEDS_Do_Update();
                printf("UPDA\n");
                break;
            }
            case MATCH_SET_BLOB:            // BLOB: Load a new binary object containing program.
            {
                load_blob();
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
                BlueTooth_Printf("TRIG %d\n", arg);
                break; 
            }
            case MATCH_BRIGHTNESS:
            {
                arg = read_num();
                D(DEBUG_PRINTF, { printf("BRIG %d \r", arg); fflush(stdout); })

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
                BlueTooth_Printf("SPHY %d\n", phynum);
                break;
            }
            case MATCH_SHOW:
            {
                arg = read_num();
                D(DEBUG_PARSER, printf("DO SHOW SCENE %d\n", arg);)
                Set_Scene(arg);
                LEDS_Do_Update();
                break;
            }
            // case MATCH_QUEUE:
            // {
            //     arg = read_num();
            //     printf("QUEU %d\n", arg);         // SAME as trig (for now).
            //     Blob_Queue_Next(arg);
            //     break; 
            // }
            // case MATCH_SCENE:   
            // {
            //     printf("SCEN\n");
            //     read_scene();
            //     break; 
            // }
            case MATCH_DUMP:
            {
                arg = read_num();
                int start = read_hnum();  // Try to read another arg.
                // printf("Dump %d, start %X\n", arg, start);
                do_dump(arg, start);
                break; 
            }
            case MATCH_DEBUG:
            {
                arg = read_num();
                printf("Set Debug Flag %d(0x%X)\n", arg, arg);
                Debug_Mask = arg;
                break;
            }
        }
        Matchers_Reset();
    }
}


PUBLIC void BtParser()
{
    bool is_connected_flag = false;
    // bool is_online = false;

    // is_connected
    Matchers_Init();

    while (true) 
    {
        // if (!is_connected)
        // {
        //     // if (is_online)     // Go offline.
        //         is_online = false;
        // }
        // else
        // {
        //     if (!is_online)
        //     {
        //         is_online = true;
                
        //     }
        // }

        if (do_pair_flash)
        {
            for (int i = 3; i; --i)
            {
#               define PAIR_FLASH_DELAY 500
                ObLED_On();  sleep_ms(PAIR_FLASH_DELAY);
                ObLED_Off(); sleep_ms(PAIR_FLASH_DELAY);
            }
            do_pair_flash = false;
        }

        if (is_connected)
        {
            if (!is_connected_flag)
            {
                is_connected_flag = true;
                printf("Parser: Connected\n.");
            }
            parser_setup_readline();
            parser_wait_readline();

            while (true)
            {
                int ch = parser_getchar();

                if (ch == PARSER_GETCHAR_EMPTY)
                    break;
                
                parse_and_execute_commands(ch); 
            }
        }
        else
        {
            if (is_connected_flag)
            {
                is_connected_flag = false;
                printf("Parser: Not connected\n.");
            }
        }
    }
}


// EndFile: Parser.c