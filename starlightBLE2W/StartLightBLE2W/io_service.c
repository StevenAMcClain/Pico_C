// File: io_service.c

#include "common.h"
#include "io_service.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <bluetooth.h>
#include <ble/att_db_util.h>

#include "config.h"
//#include "debug_pin.h"
#include "error.h"
#include "hashtab.h"
#include "picopins.h"
#include "picopwm.h"

#include "io_service_bus.h"
#include "io_service_gpio.h"
#include "io_service_led.h"
#include "io_service_pwm.h"
#include "io_service_strip.h"


#define ATT_DESC_CHARACTERISTIC_USER_DESCRIPTION     0x2901
#define ATT_DESC_CLIENT_CHARACTERISTIC_CONFIGURATION 0x2902
#define ATT_DESC_CHARACTERISTIC_PRESENTATION_FORMAT  0x2904

#define MAX_ATT_VALUES 30

PRIVATE ATT_VALUE Att_Values[MAX_ATT_VALUES] = {0};
PRIVATE ATT_VALUE* Att_Values_LastPtr = Att_Values;

PRIVATE hash_table_t* hashtab = NULL;     // Use a hash table to map names to ATT_VALUE pointers for quick lookup.


PRIVATE ATT_VALUE* Att_Values_Find(handle_t handle)
{
    ATT_VALUE* ptr = Att_Values;

    while (ptr < Att_Values_LastPtr)
    {
        if ( (handle == ptr->handle)
          || (handle == (ptr->handle + 1)) )
        {
            return ptr;
        }
        ptr++;
    }
    return NULL;
}


PUBLIC ATT_VALUE* Att_Values_Find_Name(const char* name)
{
    NAME hash = Hash__Lookup(hashtab, name, false);

    if (hash)
    {
        return Hash__Get_Value(hashtab, hash);
    }
    return NULL;
}


PUBLIC ATT_VALUE* Att_Values_Add(handle_t handle, const char* name, IOTYPE type, uintptr_t iohandle, uint16_t value_len, uintptr_t value, const char* desc)
{
    ATT_VALUE* ptr = NULL;

    if (name && *name && Att_Values_LastPtr < Att_Values + MAX_ATT_VALUES)
    {
        ptr = Att_Values_LastPtr++;

        ptr->handle = handle;

        strncpy(ptr->name, name, sizeof(ptr->name)-1);
        ptr->name[sizeof(ptr->name)-1] = 0;

        ptr->type = type;
        ptr->iohandle = iohandle;
        ptr->value_len = value_len;
        ptr->value = value;

        if (desc) 
        {
            strncpy(ptr->desc, desc, sizeof(ptr->desc)-1); 
            ptr->desc[sizeof(ptr->desc)-1] = 0;
        }

        NAME hash = Hash__Lookup(hashtab, name, true);
        if (hash) { Hash__Set_Value(hashtab, hash, ptr); }
    }
    return ptr;
}


PUBLIC const char* Att_Value_Type_String(ATT_VALUE* att_val)
{
    if (att_val)
    {
        IOTYPE type = att_val->type;

        if (type <= IOTYPE_MAX)
        {
            static const char* type_string[] = { "NONE", "INPUT", "OUTPUT", "PWM", "OUTBUS", "LED", "STRIP", "MAX" };
            return type_string[type];
        }
    }
    return "<?>";
}


PUBLIC void Att_Value_Pin_List(char** buffp, size_t* buffsizep, uint8_t* pin_list)
{
    char* buff =  *buffp;
    size_t buffsize = *buffsizep;
    // int count = MAX_BUS_PINS;

    while (*pin_list != BAD_PIN_NUMBER && buffsize)
    {
        snprintf(buff, buffsize, "%d%s", *pin_list, *(pin_list + 1) == BAD_PIN_NUMBER ? "" : ", ");
        size_t len = strlen(buff); buff += len; buffsize -= len;

        ++pin_list;
    }

    *buffp = buff; *buffsizep = buffsize;
}


PUBLIC void Dump_Srvs_List(char** buffp, size_t* buffsizep)
{
    if (buffp && buffsizep)
    {
        char* buff = *buffp;
        size_t buffsize = *buffsizep;
        
        if (buff && buffsize > 1)
        {
            *buff = 0; --buffsize;

            ATT_VALUE* ptr = Att_Values;

            snprintf(buff, buffsize, "\nStart.\n");
            size_t len = strlen(buff); buff += len;  buffsize -= len;

            while (ptr < Att_Values_LastPtr)
            {
                IOTYPE type = ptr->type;
                const char* type_name = Att_Value_Type_String(ptr);

                snprintf(buff, buffsize, "%s\t%s\th= %2.2X, ", ptr->name, type_name, ptr->handle);
                len = strlen(buff); buff += len;  buffsize -= len;

                switch (type)
                {
                    case IOTYPE_INPUT:
                    case IOTYPE_OUTPUT:
                    case IOTYPE_PWM:
                    {
                        snprintf(buff, buffsize, "pin(%d)", ptr->iohandle);
                        break; 
                    }
                    case IOTYPE_OUTBUS:
                    {
                        uint8_t *bus_pins = (uint8_t*)ptr->iohandle;
                        char pbuff[100] = {0};
                        char* xbuff = pbuff;
                        size_t xbuffsize = sizeof(pbuff);

                        Att_Value_Pin_List(&xbuff, &xbuffsize, bus_pins);
                        snprintf(buff, buffsize, "pins(%s)", pbuff); 
                        break;
                    }
                    case IOTYPE_LED:
                    {
                        uint32_t val = ptr->iohandle;
                        uint8_t   red_val =  val        & 0xFF;
                        uint8_t green_val = (val >> 8)  & 0xFF;
                        uint8_t  blue_val = (val >> 16) & 0xFF;
                        uint8_t white_val = (val >> 24) & 0xFF;

                        snprintf(buff, buffsize, "r %d g %d b %d w %d", 
                                             red_val, green_val, blue_val, white_val);
                        break;
                    }
                    case IOTYPE_STRIP:
                    {
                        snprintf(buff, buffsize, "pin(%d)", ptr->iohandle);
                        break; 
                    }
                    default:
                    {
                        POST_ERROR("Unknown type %d!", type);
                        break;
                    }
                }
                len = strlen(buff); buff += len;  buffsize -= len;

                if (ptr->cccd)
                {
                    snprintf(buff, buffsize, ", notify"); 
                    len = strlen(buff); buff += len;  buffsize -= len;
                }

                if (*ptr->desc)
                {
                    snprintf(buff, buffsize, ", desc(%s)", ptr->desc); 
                    len = strlen(buff); buff += len;  buffsize -= len;
                }

                *buff++ = '\n';  --buffsize;
                *buff = 0; 
                // snprintf(buff, buffsize, "\n");
                // len = strlen(buff); buff += len;  buffsize -= len;

                ptr++;
            }

            len = strlen(buff); buff += len;  buffsize -= len;
        }
        *buffp = buff;
        *buffsizep = buffsize;
    }
}


PUBLIC const ioserv_function_t* io_service_get_device( IOTYPE type )
{
    static const ioserv_function_t* device_list[] =    // must stay in one to one with IOTYPE_TYPE enum!
    {
        NULL,                 // IOTYPE_NONE,   // 0
        &IOServ_GPIO_Input,   // IOTYPE_INPUT,  // 1
        &IOServ_GPIO_Output,  // IOTYPE_OUTPUT, // 2
        &IOServ_PWM,          // IOTYPE_PWM,    // 3
        &IOServ_Output_Bus,   // IOTYPE_BUS,    // 4
        &IOServ_Led,          // IOTYPE_LED,    // 5
        &IOServ_Strip,        // IOTYPE_STRIP,  // 6
    };

    return (type > IOTYPE_NONE && type < IOTYPE_MAX) ? device_list[type] : NULL;
}


PRIVATE uint16_t device_read(IOTYPE type, ATT_VALUE* att_value, uint8_t* val, size_t val_len)
{
    const ioserv_function_t* device = io_service_get_device(type);

    if (device && device->read) { return device->read(att_value, val, val_len); }

    return 0;
}


PRIVATE bool device_write(IOTYPE type, ATT_VALUE* att_value, uint32_t val)
{
    const ioserv_function_t* device = io_service_get_device(type);

    if (device && device->write) { return device->write(att_value, val); }

    return false;
}


PUBLIC bool Att_Apply_Write(handle_t handle, const uint8_t* buffer, uint16_t buffer_size)
{
    bool got_one = false;
    ATT_VALUE* att_value = Att_Values_Find(handle);

    if (att_value)
    {
        if (handle == att_value->handle)
        {
            uint32_t val;

                 if (buffer_size == 1) { val = BUILD_VAL(buffer); }
            else if (buffer_size == 2) { val = BUILD_2_VAL(buffer); }
            else if (buffer_size == 4) { val = BUILD_4_VAL(buffer); }
            else { return POST_ERROR("invalid len expect 1,2,4 got %d", buffer_size); }

            got_one = device_write(att_value->type, att_value, val);

            if (!got_one) { POST_ERROR("can't handle iotype %d!\n", att_value->type); }
        }
        else  // Ignore: CCCD Writes. 
        {
            DBG(0, DEBUG_BLESERVICE, "CCCD write (%x)!\n", handle);
            att_value->cccd = *buffer;
            got_one = true; 
        }
    }
    else { POST_ERROR("no att_value!"); }

    return got_one;
}


PUBLIC uint16_t Att_Apply_Read(handle_t handle, uint8_t* buffer, uint16_t buffer_size)
{
    uint16_t rval = 0;
    ATT_VALUE* att_value = Att_Values_Find(handle);

    if (att_value)
    {
        if (handle == att_value->handle)
        {
            rval = device_read(att_value->type, att_value, buffer, buffer_size);

            if (!rval) { POST_ERROR("can't handle iotype %d!\n", att_value->type); }
        }
        else  // Ignore: CCCD Writes. 
        {
            DBG(0, DEBUG_BLESERVICE, "CCCD read (%x).\n", handle);

            if (buffer && buffer_size >= 2)
            {
                *buffer = 0;  *(buffer + 1) = att_value->cccd ? 1 : 0;
            }
            
            rval = 2;
        }
    }
    else { POST_ERROR("no att_value!"); }

    return rval;
}


// -----------------------------------------------------------------------------
// Buffers and handles
// -----------------------------------------------------------------------------

static uint8_t  _uuid[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x42, 0x4C, 0x45, 0x42, 0x49, 0x54, 0x54, 0x45, 0x43, 0x48 };



PRIVATE uint8_t* next_uuid(void)
{
    ++_uuid[3];
    return _uuid;
}


PUBLIC handle_t New_Characteristic(const char* name, bool read_only, gatt_unit_t unit, gatt_format_t format, gatt_exponent_t exponent, IOTYPE type, uint8_t unit_id)
{
    uint8_t* uuid = next_uuid();
    
    handle_t handle = att_db_util_add_characteristic_uuid128( uuid,
        (read_only ? 0 : (ATT_PROPERTY_WRITE | ATT_PROPERTY_WRITE_WITHOUT_RESPONSE)) | ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY | ATT_PROPERTY_DYNAMIC,
        ATT_SECURITY_NONE, ATT_SECURITY_NONE, NULL, 0);

    if (name && *name)
    {
        att_db_util_add_descriptor_uuid16(
            ATT_DESC_CHARACTERISTIC_USER_DESCRIPTION,   // UUID 0x2901
            ATT_PROPERTY_READ,
            ATT_SECURITY_NONE, ATT_SECURITY_NONE,
            (uint8_t*)name, strlen(name) );
    }

    uint8_t presentation[] = 
    {
        format,                        // format
        exponent,                      // exponent
        GATT_UNIT(unit),               // unit
        GATT_NAMESPACE_BLUETOOTH_SIG,  // namespace
        type, unit_id                  // description
    };

    att_db_util_add_descriptor_uuid16(
        ATT_DESC_CHARACTERISTIC_PRESENTATION_FORMAT,  // UUID 0x2904
        ATT_PROPERTY_READ,
        ATT_SECURITY_NONE, ATT_SECURITY_NONE,
        (uint8_t*)presentation, sizeof(presentation));

    return handle;
}


PUBLIC bool IO_Service_Handle_Match(uint16_t handle)
{
    return (bool)Att_Values_Find(handle);
}


PUBLIC int IO_Service_Write(handle_t handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    UNUSED(transaction_mode);  // Not used in this callback, but required by the function signature.
    UNUSED(offset);

    return Att_Apply_Write(handle, buffer, buffer_size) ? ATT_ERROR_SUCCESS : ATT_ERROR_ATTRIBUTE_NOT_FOUND;
}


PUBLIC uint16_t IO_Service_Read(handle_t handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{
    UNUSED(offset);  // Not used in this callback, but required by the function signature.

    return Att_Apply_Read(handle, buffer, buffer_size);
}


PUBLIC void IO_Service_Initialize()
{
    att_db_util_add_service_uuid128(next_uuid());    // Add io service
    hashtab = Hash__Initialize(MAX_ATT_VALUES);  // Initialize the name hash table.
}


// End of file: io_service.c
