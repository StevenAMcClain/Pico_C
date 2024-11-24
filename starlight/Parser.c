// File: Parser.c

#include "Common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "Blob.h"
#include "Led.h"
#include "Matcher.h"

#include "bluetooth_stdio.h"


#define BLOB_VERSION "0.1a"

#define CHAR_TIMEOUT 50000

#define READ_INTR   16
#define READ_QUEUE  20
#define READ_TRIG   22
#define READ_BRIGHTNESS 94
#define READ_CONFIG 99

#define MAX_BRIGHTNESS_NUM 100

int char_counter = 0;


PRIVATE int parser_getchar(void)
{
    int val = PICO_ERROR_TIMEOUT;
    if (BlueTooth_Check_Receive())
    {
        val = BlueTooth_GetChar();
    }
    return val;
}


PRIVATE bool read_bytes(int n, uint8_t* buff)
{
    while (n--)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            ++char_counter;
            *buff++ = ch;
        }
        else
        {
            printf("read_bytes: TIMEOUT expecting %d more bytes.\n", n); 
            break;
        }

    }

    return n == (-1);
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
            ++char_counter;

            scene_led_val[scene_led_idx++] = ch;

            if (scene_led_idx == LED_SIZE)
            {
                scene_led_idx = 0;
// fix /////////////                // LED_Set_LED(scene_led_num, scene_led_val);

                if (++scene_led_num == Num_LEDS)
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

#define BLOB_PRE_HEADER_SIZE 12         // Number of bytes to read before reading header.

PRIVATE uint32_t Version()
{
    uint8_t* ver = BLOB_VERSION;
    return ver[3] << 24 | ver[2] << 16 | ver[1] << 8 | ver[0];
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
    uint8_t buff[BLOB_PRE_HEADER_SIZE + 4];
    uint32_t blob_size = 0;

    printf("\nread_blob: ");

    if (read_bytes(BLOB_PRE_HEADER_SIZE, buff))
    {
        uint32_t* ptr = (uint32_t*)buff;

        if (*ptr++ == Version())
        {
            if (*ptr++ == Num_LEDS)
            {
                blob_size = *ptr++ * sizeof(uint32_t);
                // printf(" size %d\n", blob_size);

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
                                printf("\n* * * * * * BLOB LOADED * * * * * *\n");
                                Blob_Load(base);
                                BlueTooth_Printf("BLOB loaded.\n");
                            }
                            else { BlueTooth_Printf("!!! Bad checksum: expected %X, got %X\n", blob_check, check); }
                        }
                    }
                }
            }
            else { BlueTooth_Printf("!!! Wrong number of LEDS: expected %d, got %d\n", Num_LEDS, *(ptr-1)); }
        }
        else { BlueTooth_Printf("!!! Bad blob version: expected %X, got %X\n", Version(), *(ptr-1)); }
    }
    BlueTooth_Printf("!!! BLOB load load fail.\n\n");
}


PRIVATE void read_num(int mode)
{
    int val = 0;

    bool reading = true;

    while (reading)
    {
        int ch = parser_getchar();

        if (ch != PICO_ERROR_TIMEOUT)
        {
            ++char_counter;

            if (isdigit(ch))
            {
                val *= 10;
                val += ch - '0';
            }
            else if (ch == ' ')
            {
                if (mode == READ_INTR)
                {
                    printf("DO INTER %d\n", val);         // SAME as trig (for now).
//                    Blob_Queue_Next(trigger_num);
                }
                else if (mode == READ_QUEUE)
                {
                    printf("DO NEXT %d\n", val);         // SAME as trig (for now).
                    Blob_Queue_Next(val);
                }
                else if (mode == READ_TRIG)
                {
                    printf("DO TRIGGER %d\n", val);
                    Blob_Trigger(val);
                }
                else if (mode == READ_CONFIG)
                {
                    printf("DO READ CONFIG %d\n", val);

                    if (val > 0 && val < MAX_NUM_LEDS)
                    {
                        Num_LEDS = val;
                        // Blob must match Num_LEDS.
                    }
                }
                else if (mode == READ_BRIGHTNESS)
                {
                    printf("BRIG %d\n", val);

                    if (val >= 0 && val <= MAX_BRIGHTNESS_NUM)
                    {
                        LED_Brightness = val / (double)MAX_BRIGHTNESS_NUM;
                        LED_Update();
                    }
                }
                reading = false;
            }
            else { reading = false; }
        }
        else 
        {
            printf("read_num: TIMEOUT\n"); 
            reading = false;
        }
    }
}


PRIVATE void scan_for_sync()
{
    int ch = parser_getchar();

    if (ch != PICO_ERROR_TIMEOUT)
    {
        ++char_counter;

        MATCH_CODE code = Is_Match(ch);

        if (code)
        {
            switch (code)
            {
                case MATCH_BLACK:           // BLAC: Set all leds to black.
                {
                    printf("BLAC\n");
                    Blob_Stop();
                    LEDS_All_Black();
                    BlueTooth_Printf("BLAC\n");
                    break;
                }
                case MATCH_BLOB:            // BLOB: Load a new binary object containing program.
                {
                    printf("BLOB\n");
                    read_blob();
                    break;
                }
                case MATCH_TRIGGER:
                {
                    printf("TRIG\n");
                    read_num(READ_TRIG);
                    BlueTooth_Printf("TRIG\n");
                    break; 
                }
                case MATCH_BRIGHTNESS:
                {
//                    printf("BRIG\n");
                    read_num(READ_BRIGHTNESS);
                    break;
                }
                case MATCH_CONFIG:          // Define the number of LEDs in system.
                {
                    printf("CONF\n");
                    read_num(READ_CONFIG);
                    break;
                }
                case MATCH_QUEUE:
                {
                    printf("QUEU\n");
                    read_num(READ_QUEUE);
                    break; 
                }
                case MATCH_SCENE:   
                {
                    printf("SCEN\n");
                    read_scene();
                    break; 
                }
            }
            Matchers_Reset();
        }
    }
//    else { printf("scan_for_sync: TIMEOUT\n"); }
}


PUBLIC void Start_Parser()
{
    Matchers_Init();

    while (1) { scan_for_sync(); }
}


// EndFile: Parser.c