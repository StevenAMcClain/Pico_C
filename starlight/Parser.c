// File: Parser.c

#include "common.h"
#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "bluetooth_stdio.h"

#include "blob.h"
#include "debug.h"
#include "led.h"
#include "matcher.h"
#include "scene.h"


#define CHAR_TIMEOUT 50000

#define MAX_BRIGHTNESS_NUM 100

#define MAX_BLOB_SIZE (16 * 1024)    // Fairly arbitrary max size

#define BLOB_PRE_HEADER_SIZE 8      // Number of bytes to read before reading header.



PRIVATE int parser_getchar(void)
{
    return BlueTooth_Check_Receive() ? BlueTooth_GetChar() : PICO_ERROR_TIMEOUT;
}

uint64_t retry_counter = 0;     // Number of retries... ever!

#define NUM_RETRIES 5

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
            // BlueTooth_Printf("!!! Retry.\n"); 
            retry_counter++;
            sleep_ms(3);
            continue;
        }
        else
        {
            //printf("!!! Blob read TIMEOUT expecting %d more bytes.\n", n); 
            BlueTooth_Printf("!!! Blob read TIMEOUT expecting %d more bytes.\n", n); 
            break;
        }
    }

    return n == 0;   // True if all bytes were read.
}


PRIVATE void read_scene(void)
{
    int scene_led_num = 0;
    int scene_led_idx = 0;
    uint8_t scene_led_val[LED_SIZE] = {0};

    bool reading = true;

    while (reading)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            scene_led_val[scene_led_idx++] = ch;

            if (scene_led_idx == LED_SIZE)
            {
                scene_led_idx = 0;
// fix /////////////                // LED_Set_LED(scene_led_num, scene_led_val);

//                if (++scene_led_num == Num_LEDS)
                {
// fix /////////////                    led_send();
                    reading = false; 
                }
            }
        }    
        else 
        {
            printf("read_scene: TIMEOUT\n"); 
            reading = false; 
        }
    }
}

PRIVATE uint32_t Version()
{
    uint8_t* ver = BLOB_VERSION;
    return ver[3] << 24 | ver[2] << 16 | ver[1] << 8 | ver[0];
}

PRIVATE char* version_to_str(char* buff, uint32_t val)
{
    buff[0] = val & 0xFF;
    buff[1] = (val >> 8) & 0xFF;
    buff[2] = (val >> 16) & 0xFF;
    buff[3] = (val >> 24) & 0xFF;
    buff[4] = 0;

    return buff;
}


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
    uint8_t buff[BLOB_PRE_HEADER_SIZE];
    uint32_t blob_size = 0;

    memset(buff, -1, sizeof(buff));

    printf("\nread_blob: ");

    if (read_bytes(BLOB_PRE_HEADER_SIZE, buff))
    {
        uint32_t* ptr = (uint32_t*)buff;

        if (*ptr++ == Version())
        {
            blob_size = *ptr++ * sizeof(uint32_t);
            printf(" size %d\n", blob_size);

            if (blob_size < MAX_BLOB_SIZE)
            {
                uint8_t* base = malloc(blob_size);

                if (base)
                {
                    memset(base, 0, blob_size);
                    *(uint32_t*)base = blob_size;       // Copy blob_size into header.
                    base += sizeof(uint32_t);

                    if (read_bytes(blob_size, base))
                    {
                        uint32_t blob_check = Checksum(base, blob_size);
                        uint32_t check = 0;

                        if (read_bytes(sizeof(uint32_t), (uint8_t*)&check))
                        {
                            // printf("check %d, blob_check %d\n", check, blob_check);

                            if (check == blob_check)
                            {
                                // printf("read_blob: base %X %X\n", real_base, base);
                                Blob_Load(base);
                                BlueTooth_Printf("BLOB loaded.\n");
                                return;
                            }
                            else { BlueTooth_Printf("!!! Bad checksum: expected %X, got %X\n", blob_check, check); }
                        }
                    }
                }
            }
            else { BlueTooth_Printf("!!! Bab blob size (%d) must be less than %d\n", blob_size, MAX_BLOB_SIZE); }
        }
        else 
        { 
            char s1[10], s2[10];
            BlueTooth_Printf("!!! Bad blob version: expected '%s', got '%s'\n", 
                                version_to_str(s1, Version()), 
                                version_to_str(s2, *(ptr-1))); 
        }
    }
    BlueTooth_Printf("!!! BLOB load load fail.\n\n");
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
        else { printf("read_num: TIMEOUT\n"); }

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
        else { printf("read_snum: TIMEOUT\n"); }

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
        else { printf("read_hnum: TIMEOUT\n"); }

        reading = false;
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
        version_to_str(s1, Version());

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


PRIVATE void scan_for_sync(void)
{
    int ch = parser_getchar();

    if (ch != PICO_ERROR_TIMEOUT)
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
                    read_blob();
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
                    printf("BRIG %d    \r", arg);
                    fflush(stdout);

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
                case MATCH_PHYS:          // Define the number of LEDs in system.
                {
                    int phynum = read_snum();
                    arg = read_num();

                    PHY_Set_led_count(phynum, arg);
                    BlueTooth_Printf("PHYS %d has %d LEDs.\n", phynum, arg);
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
                case MATCH_QUEUE:
                {
                    arg = read_num();
                    printf("QUEU %d\n", arg);         // SAME as trig (for now).
                    Blob_Queue_Next(arg);
                    break; 
                }
                case MATCH_SCENE:   
                {
                    printf("SCEN\n");
                    read_scene();
                    break; 
                }
                case MATCH_DUMP:
                {
                    arg = read_num();
                    int start = read_hnum();
                    printf("Dump %d, start %X\n", arg, start);
                    extern void do_dump(int arg, int arg2);
                    do_dump(arg, start);
                    break; 
                }
                case MATCH_DEBUG:
                {
                    arg = read_num();
                    printf("Set Debug Flag %d(%X)\n", arg, arg);
                    Debug_Mask = arg;
                    break;
                }
                case MATCH_VERSION:
                {
                    char s1[10];
                    version_to_str(s1, Version());
                    BlueTooth_Printf("VERS:%s\n", s1);
                    break;
                }
                case MATCH_GET_BLOB:
                {
                    do_export();
                    break;
                }
            }
            Matchers_Reset();
        }
    }
}


PUBLIC void Start_Parser()
{
    Matchers_Init();

    while (true) 
    { 
        scan_for_sync(); 
        sleep_ms(1); 
    }
}


// EndFile: Parser.c