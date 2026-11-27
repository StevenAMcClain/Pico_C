// File: config.c

#include "common.h"
#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "BLECore.h"
#include "debug.h"
#include "error.h"
#include "io_service.h"
#include "picoflash.h"
#include "picopins.h"

#define CONFIG_COOKIE 0x454C4241   //'ABLE'

PUBLIC size_t Script_Sector = BAD_FLASH_SECTOR; 
PUBLIC char* Script_Start = "";


PUBLIC int Save_Config(const char* str)
{
    int result = PICO_ERROR_GENERIC;

    if (str)
    {
        if (Flash_Erase_All_Sectors() == PICO_OK)
        {
            result = Flash_Write_Buff(0, (uint8_t*)str, strlen(str) + 1);

            if (result == PICO_OK)
            {
                size_t len = strlen(str);
                Script_Sector = (len / FLASH_SECTOR_SIZE) + 1;
                Script_Start = (char*)Flash_Sector_Number_To_Address(Script_Sector);
            }
            }
            else { POST_ERROR("Flash erase failed!"); }
    }
    return result;
}


PUBLIC char* Config_File(void)
{
    return (char*)Flash_Sector_Number_To_Address(0);
}


PUBLIC bool Load_Config(void)
{
    bool result = false;

    // Config always starts at sector zero of the flash.
    //
    uint32_t* wptr = (uint32_t*)Config_File();

    if (*wptr == CONFIG_COOKIE)
    {
        char* ptr = (char*)wptr;

        result = Process_Config(ptr, false);

        if (result)
        {
            result = Process_Config(ptr, true);

            if (result)
            {
                size_t len = strlen(ptr);
                Script_Sector = (len / FLASH_SECTOR_SIZE) + 1;
                Script_Start = (char*)Flash_Sector_Number_To_Address(Script_Sector);
            }
            else { POST_ERROR("Real Process failed!"); }
        }
        else { POST_ERROR("Test Process failed!"); }
    }
    else 
    {
        const char* msg = (*wptr == 0xFFFFFFFF) ? "No config!" : "Bad cookie!";
        POST_ERROR("%s", msg); 
    }

    return result;
}


#define SKIP_PIN 0xFF

PUBLIC picopins_t Pico_Pins;        // Global pin state.


PRIVATE bool device_init(IOTYPE type, char* name, uint8_t* port_pins, size_t pin_count, const char* desc)
{
    const ioserv_function_t* device = io_service_get_device(type);

    if (device) { return device->init(name, port_pins, pin_count, desc); }

    return false;
}


PRIVATE void calc_checks(const char* configp, uint32_t* countp, uint32_t* checkp)
{
    *countp = *checkp = 0;

    while (*configp)
    {
        ++(*countp);
        (*checkp) += *configp;
        ++configp;
    }

    DBG(DEBUG_CONFIG, "calculated count %d, check %d\n", *countp, *checkp);
}


PRIVATE bool checks_match(const char* configp, uint32_t read_count, uint32_t read_check)
{
    uint32_t count, check;

    calc_checks(configp, &count, &check);

    bool pass = (read_count == count && read_check == check);

    if (!pass) { POST_ERROR("pass %d, count %d %d, check %d %d", pass, read_count, count, read_check, check); }

    return pass;
}


PRIVATE bool space_test(char ch)
{
    return ch && ch != '\n' && isspace(ch);
}


PRIVATE const char* skip_space(const char* configp)
{
    char ch = *configp;

    while (space_test(ch)) { ch = *(++configp); }

    return configp;
}


PRIVATE bool match_word(const char** configp, const char* word, size_t word_size)
{
    bool result = false;
    const char* p = *configp;

    if (strncasecmp(p, word, word_size) == 0)
    {
        p += word_size;                       // Skip word.

        if (space_test(*p))
        {
            *configp = skip_space(p);         // Skip space.
            result = true;
        }
    }
    return result;
}


PRIVATE bool read_number(const char** configp, uint32_t* num)
{
    int num_count = 6;    // Max digits in number.
    bool result = false;
    const char* p = *configp;

    *num = 0;

    while (num_count && *p && isdigit(*p))
    {
        *num *= 10; *num += *p++ - '0';
        --num_count;
        result = true;
    }

    *configp = skip_space(p);

    //DBG(DEBUG_CONFIG,"last ch %d\n", **configp);

    return result;
}


PRIVATE bool read_name(const char** configp, char* name, size_t size)
{
    bool result = false;

    int count = size - 1;
    char* namep = name;

    bool isfirst = true;
    const char* p = *configp;

    *namep = 0;

    while (*p && count)
    {
        if ((*p == '_') || (isfirst ? isalpha(*p) : isalnum(*p)))
        {
            isfirst = false;
            *namep++ = *p++;
            --count;
            result = true;
        }
        else { break; }
    }

    *namep = 0;
    *configp = skip_space(p);

    if (!count)   // Description too long?
    {
        POST_ERROR("name '%s' too long! must be less than %d characters!", name, size);
        while (*p && *p != '\n') { ++p; }
    }

    return result;
}


PRIVATE bool read_description(const char** configp, char* desc, size_t size)
{
    bool result = false;

    int count = size - 1;
    char* descp= desc;
    const char* p = *configp;

    *descp = 0;

    while (*p && count)
    {
        if (*p != '\n')
        {
            *descp++ = *p++;
            --count;
        }
        else { break; }
    }

    *descp = 0;
    *configp = p;

    if (!count)   // Description too long?
    {
        POST_ERROR("'%s' too long! must be less than %d characters!", desc, size);
        while (*p && *p != '\n') { ++p; }
    }
    else { result = true; }

    return result;
}


// typedef enum
// {
//     PIN_TYPE_NONE,          // 0 No type (yet).
//     PIN_TYPE_IN,            // 1 GPIO Input.
//     PIN_TYPE_OUT,           // 2 GPIO Output.
//     PIN_TYPE_PWM,           // 3 Single PWM pin.
//     PIN_TYPE_OBUS,          // 4 Multiple GPIO pins.
//     PIN_TYPE_LED,           // 5 Mutlple PWM channels.
//     PIN_TYPE_STRIP,         // 6 Strip of LEDs.

// } PIN_TYPE;


PRIVATE IOTYPE parse_pin_type(const char** configp)
{
    IOTYPE type = IOTYPE_NONE;

         if (match_word(configp,    "IN", 2)) { type = IOTYPE_INPUT;    }
    else if (match_word(configp,   "OUT", 3)) { type = IOTYPE_OUTPUT;   }
    else if (match_word(configp,   "PWM", 3)) { type = IOTYPE_PWM;   }
    else if (match_word(configp,  "OBUS", 4)) { type = IOTYPE_OUTBUS;  }
    else if (match_word(configp,   "LED", 3)) { type = IOTYPE_LED;   }
    else if (match_word(configp, "STRIP", 5)) { type = IOTYPE_STRIP; }

    return type;
}

#define ALLOW_SKIP true
#define NO_SKIP false

PRIVATE bool read_pin_number(const char** configp, uint8_t* pin_number, uint8_t* pin_count, size_t max_pins, bool allow_skip)
{
    const char* p = *configp;
    bool result = false;
    *pin_count = 0;

    while (*p && max_pins)
    {
        uint32_t pin_num;
        
        if (allow_skip && *p == '.')
        {
            p = skip_space(++p);                      // skip past '.'
            *pin_number++ = SKIP_PIN;
            (*pin_count)++;                           // one more pin defined.
            --max_pins;
        }
        else if (read_number(&p, &pin_num))          // Read pin number.
        {
            if (pin_num <= PICOPIN_MAX || pin_num == BAD_PIN_NUMBER)
            {
                DBG(DEBUG_CONFIG, "%d) pin_num %d\n", *pin_count, pin_num);

                *pin_number++ = pin_num;
                (*pin_count)++;
                --max_pins;
                result = true;
            }
            else { POST_ERROR("invalid pin number %u!", pin_num); result = false; break; }
        }
        else { break; }
//        else { POST_ERROR("can't read number '%d'", *p); break; }
    }

    *configp = p;

//    DBG(DEBUG_CONFIG, "result %d, pin count %d, rest of line '%s'\n", result, *pin_count, p);

    return result;
}


PRIVATE bool parse_name_desc(const char** configpp, char* name, char* desc)
{
    const char* configp = *configpp;
    bool result = false;

    configp = skip_space(configp);              // Skip space.

    if (read_name(&configp, name, MAX_NODE_NAME_SIZE))
    {
        if (read_description(&configp, desc, MAX_DESCRIPTOR_SIZE))
        {
            if (!*configp || *configp == '\n')
            {
                if (*configp == '\n') { ++configp; }   // Skip EOL.

                result = true;
            }
            else { POST_ERROR("expected newline1!"); }
        }
        else { POST_ERROR("expected description!"); }
    }
    else { POST_ERROR("expected name!"); }

    *configpp = configp;
    return result;
}

#define MAX_PIN_NUMBERS 32

PRIVATE bool parse_pins(const char* configp, bool apply_config)
{
    uint32_t old_pins;

    if (apply_config) { PicoPin_Initialize(&Pico_Pins); }           // Start with fresh pin map.
    else              { old_pins = PicoPin_Reset(&Pico_Pins); }     // Or save current pin map.

    bool parsing = true;
    bool result = false;

    while (parsing && *configp)
    {
        configp = skip_space(configp);              // Skip space.

        if (match_word(&configp, "PIN", 3))
        {
            IOTYPE type = parse_pin_type(&configp);

            if (type == IOTYPE_STRIP)
            {
                uint8_t pin_num[1] = { [0] = 0xFF };
                uint8_t pin_count;

                if (read_pin_number(&configp, pin_num, &pin_count, 1, false))         // Read pin number.
                {
                    uint32_t num_leds = 0;

                    if (read_number(&configp, &num_leds))          // Read num_leds.
                    {
                        if (*configp == '0' || *configp == '1') 
                        {
                            bool is_rgbw = *configp == '1' ? true : false;
                            ++configp;                                  // Skip rgbw flag.
                            configp = skip_space(configp);              // Skip space.

                            char name[MAX_NODE_NAME_SIZE] = {0};
                            char desc[MAX_DESCRIPTOR_SIZE] = {0};

                            if (parse_name_desc(&configp, name, desc))
                            {
                                DBG(DEBUG_CONFIG, "pin %u, type %d, num_leds %d, is_rgbw %d, name '%s', desc '%s'\n", 
                                                   pin_num[0], type, num_leds, is_rgbw, name, desc);

                                if (apply_config)
                                {
                                    const ioserv_function_t* device = io_service_get_device(type);

                                    if (device)
                                    {
                                        void* fnv = device->init;
                                        bool (*fn)(char*name, uint8_t port_pin, uint num_leds, bool is_rgbw, char* user_desc) = fnv;

                                        if (!fn(name, *pin_num, num_leds, is_rgbw, desc))
                                        {
                                            parsing = false;
                                        }
                                    }
                                }
                                result = true;
                            }
                        }
                        else { POST_ERROR("expected '0' or '1' for has_rgbw!"); result = false; break; }
                    }
                    else { POST_ERROR("expected num_leds!"); result = false; break; }
                }
                else { POST_ERROR("expected pin_number!"); result = false; break; }
            }
            else if (type != IOTYPE_NONE)
            {
                uint8_t pin_num[MAX_PIN_NUMBERS] = {[0 ... (MAX_PIN_NUMBERS-1)] = BAD_PIN_NUMBER};
                uint8_t pin_count;
                size_t max_pins = (type == IOTYPE_OUTBUS || type == IOTYPE_LED) ? MAX_PIN_NUMBERS : 1;
                bool allow_skip = type == IOTYPE_LED ? ALLOW_SKIP : NO_SKIP;

                if (read_pin_number(&configp, pin_num, &pin_count, max_pins, allow_skip))         // Read pin number(s).
                {
                    char name[MAX_NODE_NAME_SIZE] = {0};
                    char desc[MAX_DESCRIPTOR_SIZE] = {0};

                    if (parse_name_desc(&configp, name, desc))
                    {
                        DBG(DEBUG_CONFIG, "pin %u, type %d, name '%s', desc '%s'\n", pin_num[0], type, name, desc);

                        if (apply_config)
                        {
                            if (!device_init(type, name, pin_num, pin_count, desc))
                            {
                                parsing = false;
                            }
                        }

                        result = true;
                    }
                }
                else { POST_ERROR("expected pin_number!"); result = false; break; }
            }
            else { POST_ERROR("expected pin type!"); result = false; break; }
        }
        else { POST_ERROR("expected 'PIN'!"); result = false; break; }
    }

    if (!apply_config)
    {
        PicoPin_Set(&Pico_Pins, old_pins);          // Restore old pin settings.
    }

    return result;
}


PUBLIC bool Process_Config(const char* configp, bool apply_config)
{
    bool result = false;

    DBG(DEBUG_CONFIG, "start.\n");

    if (configp && *configp)
    {
        if (match_word(&configp, "ABLE", 4))
        {
            uint32_t count, check;
            
            if (read_number(&configp, &count))          // Read count.
            {
                if (read_number(&configp, &check))      // Read check.
                {
                    DBG(DEBUG_CONFIG, "read count %u, check %u\n", count, check);

                    configp = skip_space(configp);      // Skip space.

                    if (*configp == '\n')
                    {
                        ++configp;                      // Skip EOL.

                        if (checks_match(configp, count, check))
                        {
                            configp = skip_space(configp);

                            if (match_word(&configp, "ADV", 3))
                            {
                                char name[MAX_NODE_NAME_SIZE] = {0};

                                if (read_name(&configp, name, sizeof(name)))
                                {
                                    DBG(DEBUG_CONFIG, "name is '%s'\n", name);

                                    if (apply_config) { BLE_Set_Node_Name(name); }

                                    configp = skip_space(configp);

                                    if (*configp == '\n')
                                    {
                                        ++configp;
                                        DBG(DEBUG_CONFIG, "ready to parse pins, etc.\n");
                                        result = parse_pins(configp, apply_config);
                                    }
                                    else { POST_ERROR("expected newline2!"); }
                                }
                                else { POST_ERROR("expected name!"); }
                            }
                            else { POST_ERROR("expected 'ADV'!"); }
                        }
                        else 
                        {
                            uint32_t gcount, gcheck;
                            calc_checks(configp, &gcount, &gcheck);
                            POST_ERROR("checksums failed got (%u %u), expected (%u %u)!", gcount, gcheck, count, check); 
                        }
                    }
                    else { POST_ERROR("expected newline1!"); }
                }
                else { POST_ERROR("expected checksum!"); }
            }
            else { POST_ERROR("expected count!"); }
        }
        else { POST_ERROR("expected 'ABLE'!"); }
    }
    else { POST_ERROR("missing config!"); }

    if (!result && configp) { POST_ERROR("next uint8_t %d %d %d %d\n", *configp, *(configp+1), *(configp+2), *(configp+3)); }

//    DBG(DEBUG_CONFIG, "done......\n\n");

    return result;
}


// End of file: config.c


//                                     switch (type)
//                                     {
//                                         case PIN_TYPE_IN:
//                                         {
//                                             if ( !IOServ_GPIO_Input.init(name, pin_num, pin_count, desc) )
//                                             {
//                                                 parsing = false;
//                                             }
//                                             break;
//                                         }
//                                         case PIN_TYPE_OUT:
//                                         {
//                                             if ( !IOServ_GPIO_Output.init(name, pin_num, pin_count, desc) )
//                                             {
//                                                 parsing = false;
//                                             }
//                                             break;
//                                         }
//                                         case PIN_TYPE_PWM:
//                                         {
//                                             if ( !IOServ_PWM.init(name, pin_num, pin_count, desc) )
// //                                            if ( !io_device_init_pwm(name, pin_num[0], desc) )
//                                             {
//                                                 parsing = false;
//                                             }
//                                             break;
//                                         }
//                                         case PIN_TYPE_BUS:
//                                         {
//                                             if ( !IOServ_Bus.init(name, pin_num, pin_count, desc) )
//                                             {
//                                                 parsing = false;
//                                             }
//                                             // if (pin_count)
//                                             // {
//                                             //     bool is_first = true;
//                                             //     printf("%s: BUS[", __func__);
//                                             //     for (int i = 0; i < pin_count; ++i)
//                                             //     {
//                                             //         printf("%s%d", is_first ? "" : ", ", pin_num[i]);
//                                             //         is_first = false;
//                                             //     }
//                                             //     printf("]\n");

//                                             //     if ( !io_device_init_bus(name, pin_num, pin_count, desc) )
//                                             //     {
//                                             //         parsing = false;
//                                             //     }
//                                             // }
//                                             // else  { POST_ERROR("BUS must have at least one pin!\n"); result = false; break; }
//                                             break;
//                                         }
//                                         case PIN_TYPE_LED:
//                                         {
//                                             if ( !IOServ_Led.init(name, pin_num, pin_count, desc) )
//                                             {
//                                                 parsing = false;
//                                             }
//                                             // if (pin_count)
//                                             // {
//                                             //     bool is_first = true;
//                                             //     printf("%s: LED[", __func__);
//                                             //     for (int i = 0; i < pin_count; ++i)
//                                             //     {
//                                             //         printf("%s%d", is_first ? "" : ", ", pin_num[i]);
//                                             //         is_first = false;
//                                             //     }
//                                             //     printf("]\n");

//                                             //     if ( !io_device_init_led(name, pin_num, desc) )
//                                             //     {
//                                             //         parsing = false;
//                                             //     }
//                                             // }
//                                             // else  { POST_ERROR("LED must have at least one pin!"); result = false; break; }
//                                             break;
//                                         }
//                                     }
