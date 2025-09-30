// File: blob.c

#include "common.h"
#include "blob.h"

#include "pico/stdlib.h"

#include <string.h>

#include "beng.h"
#include "debug.h"
#include "flashblob.h"
#include "led.h"
#include "parser.h"

#define AUTO_START_PIN 2

PUBLIC volatile bool Blob_Is_Loaded = false;
PUBLIC BLOB Blob = {0};

PRIVATE uint8_t Blob_Buff[2][MAX_BLOB_SIZE] = {0};
PRIVATE int Current_Blob_Buffer = 0;


PUBLIC uint32_t Version()
{
    uint8_t* ver = (uint8_t*)BLOB_VERSION;
    return ver[3] << 24 | ver[2] << 16 | ver[1] << 8 | ver[0];
}

PUBLIC char* version_to_str(char* buff, uint32_t val)
{
    buff[0] = val & 0xFF;
    buff[1] = (val >> 8) & 0xFF;
    buff[2] = (val >> 16) & 0xFF;
    buff[3] = (val >> 24) & 0xFF;
    buff[4] = 0;

    return buff;
}


PUBLIC uint8_t* Blob_Base_Get_New()
{
    int i = (Current_Blob_Buffer == 0) ? 1 : 0;
    uint8_t* base = &Blob_Buff[i][0];
    memset(base, 0, MAX_BLOB_SIZE);
    return base;
}


PUBLIC void Blob_Base_Switch()
{
    int i = (Current_Blob_Buffer == 0) ? 1 : 0;
    Current_Blob_Buffer = i;
}


PUBLIC BLOB_RAW* Blob_Base_Current()
{
    return (BLOB_RAW*)&Blob_Buff[Current_Blob_Buffer][0];
}

PUBLIC PROG* Prog_Ptr(PROG_ID id)
//
// Returns program pointer give program id.
{
	return Blob.Program_Base + (id - 1);
}

PUBLIC PROG_ID Prog_Id(PROG *cmdp)
//
// Returns program id given program pointer.
{
	return cmdp - Blob.Program_Base + 1;
}


PUBLIC PROG_ID Get_Trigger_Prog(TRIG_ID id)
{
	if (Blob.Num_Trig > 0)
	{
		int i = Blob.Num_Trig;
		uint32_t *trigp = Blob.Trigger_Base;

		while (--i)
		{
			if (id == *trigp++)		// ID Match?
				return *trigp;		// Return Program ID.
			++trigp;                // Skip Program ID
		}
	}
	return 0;
}

PUBLIC uint32_t Checksum(uint8_t* buff, size_t size)
{
    uint32_t check = 0;

    while (size--)
    {
        check += *buff++;
    }
    return check;
}


PUBLIC bool Blob_Verify_Checksum(uint8_t* base)
{
    BLOB_RAW* blob_raw = (BLOB_RAW*)base;

    if (blob_raw->Cookie == BLOB_COOKIE)
    {
        uint32_t blob_size = blob_raw->Size * sizeof(uint32_t);
        uint32_t check = Checksum(base + 16, blob_size);

        //printf("Blob_Verify_Checksum: expect %u got %u\n", blob_raw->Checksum, check);

        return blob_raw->Checksum == check;
    }
    return false;
}


PUBLIC bool Blob_Verify_Checksum_Loaded(void)
{
    return Blob_Verify_Checksum((uint8_t*)Blob_Base_Current());
}




PUBLIC void Blob_Unload(void)
//
// Stop all Engines and clear blob_base memory.
{
	if (Blob.Blob_BASE)
	{
		Beng_All_Stop();

        Blob_Is_Loaded = false;

		void* base = Blob.Blob_BASE;

        Blob.Blob_BASE = NULL;

		D(DEBUG_BLOB, PRINTF("Blob_Unload: %X\n", base);)
	}
}


PUBLIC bool Unpack_Blob_Header(uint8_t* blob_base)
//
// Load a new blob_base.
{
	if (blob_base)
	{
		Blob_Unload();

	    BLOB_RAW* blob_raw = (BLOB_RAW*)blob_base;
    
		D(DEBUG_BLOB, PRINTF("trig_size=%d, prog_size=%d, scen_count=%d, scen_size=%d\n",
				(blob_raw->trig_size / 2) - 1, blob_raw->prog_size, blob_raw->scen_count, blob_raw->scen_size);)

		Blob.Blob_BASE = blob_raw;

	    uint32_t* bptr = (uint32_t*)(blob_base + (4 * sizeof(uint32_t)));

        Blob.StrindX = (uint8_t*)(bptr + blob_raw->strindx_start);     // Point to base of string table.

        Blob.name = Blob.StrindX + blob_raw->blob_name - 1;

        Blob.SymTab = bptr + blob_raw->symtab_start;       // Pointer to start of symbol table.

        Blob.VStr_Index = bptr + blob_raw->vstr_index;     // Pointer to base of virtual string index.
        Blob.VStr_Array = bptr + blob_raw->vstr_array;     // Pointer to base of virtual string array.

        Blob.Scene_Index = (SCENE_ID*)(bptr + blob_raw->scen_index);	// Scene index start here.
        Blob.Scene_Array = (SCENE*)   (bptr + blob_raw->scen_array);	// Scene array start here.

        Blob.Trigger_Base = (PROG_ID*)(bptr + blob_raw->trig_start);   // Start of the trigger table.
        Blob.Program_Base = (PROG*)   (bptr + blob_raw->prog_start);	// Blob Programs start here.

        Blob.VarTab_Base = (BLOB_VAR*)(bptr + blob_raw->vartab_start);	// Vartable starts here.
        Blob.Num_VarRecs = blob_raw->vartab_size / BLOB_VAR_SIZE;

        Blob.StrindX_Size = blob_raw->strindx_size;
        Blob.SymTab_Size = blob_raw->symtab_size / 2;

        Blob.VStr_Index_Size = blob_raw->vstr_count;
        Blob.VStr_Array_Size = blob_raw->vstr_size;
            
        Blob.Num_Scenes = blob_raw->scen_count / 2;    // Number of scenes defined.
        Blob.Scene_Size = blob_raw->scen_size;	        // Sizeof the scene array.
        Blob.Num_Trig = blob_raw->trig_size / 2;   	// Number of trigger records.
        Blob.Num_Prog = blob_raw->prog_size;		    // Number of PROG records.

        if (blob_raw->phystr_size)
        {
            uint32_t* phystr = (bptr + blob_raw->phystr_start + 1);     // Point to base of phy string table.
            size_t num_phys = blob_raw->phystr_size - 1;            // Get phystring size.
            int phyidx = 0;

            while (num_phys--) { PHY_Set_led_count(phyidx++, *phystr++); }
        }

        Blob_Is_Loaded = true;

        return true;
	}
    return false;
}


PUBLIC void Blob_Init(void)
//
// Prepare BLOB for use.  Call once at startup (after everything is setup).
{
    gpio_init(AUTO_START_PIN);
    gpio_set_dir(AUTO_START_PIN, GPIO_IN);
    gpio_pull_up(AUTO_START_PIN);

    sleep_ms(1);   // Wait for everything to settle.

    bool autostart = gpio_get(AUTO_START_PIN);

    if (autostart) { BPage_Load_Blob(0); }
}


// EndFile: Blob.c
