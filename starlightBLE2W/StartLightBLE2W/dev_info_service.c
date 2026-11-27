// File: dev_info_service.c

#include "common.h"

#include <stdint.h>
#include <bluetooth.h>

#include "ble/att_db_util.h"

//--------------------------------------------------
#define UUID_DEVICE_INFORMATION_SERVICE   0x180A

#define UUID_MANUFACTURER_NAME_STRING     0x2A29
#define UUID_MODEL_NUMBER_STRING          0x2A24
#define UUID_SERIAL_NUMBER_STRING         0x2A25
#define UUID_HARDWARE_REVISION_STRING     0x2A27
#define UUID_FIRMWARE_REVISION_STRING     0x2A26
#define UUID_SOFTWARE_REVISION_STRING     0x2A28
#define UUID_SYSTEM_ID                    0x2A23
#define UUID_PNP_ID                       0x2A50


PRIVATE const char manufacturer_name[] = "AbleBit Technology";
PRIVATE const char model_number[] = "PicoW-Demo-01";
PRIVATE const char serial_number[] = "SN-00000001";
PRIVATE const char hw_rev[] = "RevA";
PRIVATE const char fw_rev[] = __DATE__" "__TIME__;   //"FW-1.0.0";
PRIVATE const char sw_rev[] = "SW-1.0.19";
PRIVATE uint8_t system_id[8] = { 0x12,0x34,0x56,0xFF,0xFE,0x9A,0xBC,0xDE };
// PRIVATE uint8_t pnp_id[7] = {
//     0x01,       // Vendor ID Source (1 = Bluetooth SIG)
//     0xFF, 0xFF, // Vendor ID (0xFFFF = test / development)
//     0x01, 0x00, // Product ID
//     0x01, 0x00  // Product Version
// };


PUBLIC void di_service_initialize(void)
{
    // Primary Service: Device Information (0x180A)
    att_db_util_add_service_uuid16(UUID_DEVICE_INFORMATION_SERVICE);
    att_db_util_add_characteristic_uuid16(UUID_MANUFACTURER_NAME_STRING, ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, (uint8_t*)manufacturer_name, sizeof(manufacturer_name) - 1);
    att_db_util_add_characteristic_uuid16(UUID_MODEL_NUMBER_STRING,      ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, (uint8_t*)model_number,      sizeof(model_number) - 1);
    att_db_util_add_characteristic_uuid16(UUID_SERIAL_NUMBER_STRING,     ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, (uint8_t*)serial_number,     sizeof(serial_number) - 1);
    att_db_util_add_characteristic_uuid16(UUID_HARDWARE_REVISION_STRING, ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, (uint8_t*)hw_rev,            sizeof(hw_rev) - 1);
    att_db_util_add_characteristic_uuid16(UUID_FIRMWARE_REVISION_STRING, ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, (uint8_t*)fw_rev,            sizeof(fw_rev) - 1);
    att_db_util_add_characteristic_uuid16(UUID_SOFTWARE_REVISION_STRING, ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, (uint8_t*)sw_rev,            sizeof(sw_rev) - 1);
    att_db_util_add_characteristic_uuid16(UUID_SYSTEM_ID,                ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, system_id,                   sizeof(system_id));
    // att_db_util_add_characteristic_uuid16(UUID_PNP_ID,                   ATT_PROPERTY_READ, ATT_SECURITY_NONE, ATT_SECURITY_NONE, pnp_id,                      sizeof(pnp_id));
}

// End of file: dev_info_service.c
