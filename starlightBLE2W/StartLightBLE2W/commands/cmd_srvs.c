// File: cmd_srvs.c

#include "common.h"
#include "cmd_srvs.h"

#include "error.h"
#include "io_service.h"


typedef enum
{
    LIST_OPTION_NONE = 0,
    LIST_OPTION_CMDS,
    LIST_OPTION_SRVS,
    LIST_OPTION_VARS,

} LIST_OPTION_CODE;

    //      if (match_word(configp,    "IN", 2)) { type = IOTYPE_INPUT;    }
    // else if (match_word(configp,   "OUT", 3)) { type = IOTYPE_OUTPUT;   }
    // else if (match_word(configp,   "PWM", 3)) { type = IOTYPE_PWM;   }
    // else if (match_word(configp,  "OBUS", 4)) { type = IOTYPE_OUTBUS;  }
    // else if (match_word(configp,   "LED", 3)) { type = IOTYPE_LED;   }
    // else if (match_word(configp, "STRIP", 5)) { type = IOTYPE_STRIP; }

//---------------------------------------------------------------------------------------------------------------------------------------
//
PRIVATE void setup_service(context_t* context, keyword_t* key)   //,    ** msgp, size_t* len, const char* glob, void* arg
{
    UNUSED(context);
    UNUSED(key);
//    return Result_CString(context, "do_Dump_Srvs_List");
}


PRIVATE keyword_t keyword_list[] = 
{ 
    {"IN",    IOTYPE_INPUT,  setup_service, "GPIO Input pin."},
    {"OUT",   IOTYPE_OUTPUT, setup_service, "GPIO Output pin."},
    {"PWM",   IOTYPE_PWM,    setup_service, "PIN with PWM on it."},
    {"OBUS",  IOTYPE_OUTBUS, setup_service, "Output buss."},
    {"LED",   IOTYPE_LED,    setup_service, "LED with up to four PWMS."},
    {"STRIP", IOTYPE_STRIP,  setup_service, "Strip of LEDS"},
};

PRIVATE const keyword_array_t list_keywords = { ARRAY_SIZE(keyword_list), keyword_list };



PRIVATE bool device_init(IOTYPE type, const char* name, uint8_t* port_pins, size_t pin_count, const char* desc)
{
    const ioserv_function_t* device = io_service_get_device(type);

    if (device) { return device->init(name, port_pins, pin_count, desc); }

    return false;
}

#define MAX_PINS 8

PRIVATE bool pin_is_ok(uint8_t pin)
{
    UNUSED(pin);

    return true;
}

//==========================================================================================================================
//
PUBLIC const atom_t* do_srvs_command(context_t* context, const command_t* match, size_t argc, const atom_t** argv)
{
    UNUSED(context);
    UNUSED(match);

    if (!argc) { return POST_ERROR("Must have at least a service name!"); }

    const atom_t* atom = *argv;
    keyword_t* keyword = Match_Argument_Key(&list_keywords, atom);

    if (keyword && keyword->data) 
    {
        uint8_t expect = 0;

        switch (keyword->code)
        {
            case IOTYPE_INPUT:
            case IOTYPE_OUTPUT:
            case IOTYPE_PWM:
            case IOTYPE_STRIP:  { expect= 1; break; }
            case IOTYPE_LED:    { expect= 4; break; }
            case IOTYPE_OUTBUS: { expect= MAX_PINS; break; }
            default:
            {
                if (atom && atom->type == ATOM_TYPE_WORD)
                {
                    const char* name = atom->data.word;
                    return POST_ERROR("Unknown service type '%s'!", name);
                }
                else { return POST_ERROR("Expecting service name!"); }
            }
        }

        uint8_t pins[MAX_PINS];
        size_t pin_count = 0;

        if (expect)
        {
            if (expect > argc - 2)  { expect = argc - 2; }

            while (pin_count < expect)
            {
                FLOAT_TYPE val = 0;
                atom = argv[1 + pin_count];

                if (Atom_To_Number(atom, &val))
                {
                    uint8_t pin = val;
                    if (pin_is_ok(pin)) { pins[pin_count++] = pin; }
                    else { return POST_ERROR("Invalid pin number %d!", pin); }
                }
                else { return POST_ERROR("Expecting a pin number!"); }
            }

            atom = argv[1 + pin_count];

            if (atom && atom->type == ATOM_TYPE_WORD)
            {
                const char* name = atom->data.word;

                if (device_init(keyword->code, name, pins, pin_count, ""))
                {
                    return Result_Printf(context, "Starting '%s' -> '%s'", keyword->name, name);
                }
                else {return POST_ERROR("Cannot start service\n"); }
            }
        }
    }

    return NULL;
}


// End of file: cmd_srvs.c
