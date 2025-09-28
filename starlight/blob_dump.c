// File: Blob.c

#include "common.h"

//#include <hardware/flash.h>
//                                                 #include <pico/btstack_flash_bank.h>
#include "blob.h"

#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
// //#include <time.h>

#include "btstdio.h"
#include "debug.h"
#include "led.h"
#include "flashblob.h"


extern uint8_t uuid[8];


PRIVATE void dump_version(void)
{
    char s1[10];
    version_to_str(s1, Version());
    BTPRINTF("VERS:%s\n", s1);
}


PRIVATE void dump_uuid(void)
{
    BTPRINTF("UUID:%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                    uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7]);
}


PRIVATE void dump_phystr(void)
{
    bool first = true;
    size_t phy_idx = 0;

    while (phy_idx < MAX_PHY)
    {
        size_t count = PHY_Get_LED_Count(phy_idx);

        if (count)
        {
            char *msg = "PhyStr List:\n%d) %d\n";
            msg += (first ? 0 : 13);
            BTPRINTF(msg, phy_idx + 1, count);
            first = false;
        }
        ++phy_idx;
    }

    if (first) { BTPRINTF("PhyStr List: Empty.\n"); }
}


PRIVATE void dump_checksum(void)
{
    if (Blob_Is_Loaded)
    {
        BLOB_RAW* blob_raw = Blob_Base_Current();

        BTPRINTF("Blob '%s', check %d\n", Blob.name, blob_raw->Checksum);
    }
    else { BTPRINTF("NOBLOB\n"); }
}


#define MAX_DUMP_BUFF_SIZE 200

PRIVATE void dump_info(void)
{
    char buff[MAX_DUMP_BUFF_SIZE] = {0};
    char* bptr = buff;
    int chars_left = MAX_DUMP_BUFF_SIZE-1;
    int n;

    char version_str[10];
    version_to_str(version_str, Version());

    n = snprintf(bptr, chars_left, "UUID: %02X%02X%02X%02X%02X%02X%02X%02X\nVERS: %s\n", 
                    uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], version_str);
    bptr += n;   chars_left -=n;

    if (Blob_Is_Loaded)
    {
        BLOB_RAW* blob_raw = Blob_Base_Current();

        n = snprintf(bptr, chars_left, "Blob: '%s', Check: %d\n", Blob.name, blob_raw->Checksum);
    }
    else { n = snprintf(bptr, chars_left, "NOBLOB\n"); }

    bptr += n;   chars_left -=n;

    // Dump Phys
    bool first = true;
    size_t phy_idx = 0;
    uint32_t mask = 1;
    uint32_t phy_mask = 0;

    while (phy_idx < MAX_PHY)
    {
        size_t count = PHY_Get_LED_Count(phy_idx);

        if (count)
        {
            char *msg = first ? "Phys: %d" : ", %d";
            n = snprintf(bptr, chars_left, msg, count);
            bptr += n;   chars_left -=n;
            first = false;
            phy_mask |= mask;
        }
        else {  PRINTF("Phy: %d has zero leds!", phy_idx); }

        ++phy_idx;   mask <<= 1;
    }

    if (first) { n = snprintf(bptr, chars_left, "Phys: None.\n"); }
    else       { n = snprintf(bptr, chars_left, "\n"); }
    bptr += n;   chars_left -=n;

    int Current_Phy_Mask = 3;     // Should be something else (or removed!)

    n = snprintf(bptr, chars_left, "PhyMask: %X, %X\nDebug: %X\n", 
                                   phy_mask, Current_Phy_Mask, Debug_Mask);
    // bptr += n;   chars_left -=n;

    BlueTooth_Send_String(buff);
   // printf("Dump: %s\n", buff);
}


#define STATS_STRING "\
Blob Stats: \n  Blob Size %d\n  Num triggers %d\n  Prog Size %d\n  Num Scenes %d\n  Scene Size %d\n\
  StrindX bytes %d\n  Number of symbols %d\n  VStr_Index_Size %d\n  VStr_Array_Size %d\nRetries %lld\n"

extern uint64_t read_bytes_retry_counter;     // Number of retries... ever!

PRIVATE void dump_blob_stats(void)
{
    if (Blob_Is_Loaded)
    {
        BTPRINTF(STATS_STRING,
                		 Blob.Blob_BASE->Size, Blob.Num_Trig, Blob.Num_Prog, Blob.Num_Scenes, Blob.Scene_Size,
                         Blob.StrindX_Size, Blob.SymTab_Size, Blob.VStr_Index_Size, Blob.VStr_Array_Size,
                         read_bytes_retry_counter);
    }
    else { BTPRINTF("NOBLOB\n"); }
}


PRIVATE void dump_blob_scene_index(void)
{
    if (Blob_Is_Loaded)
    {
        BTPRINTF("\nScenes:\n");

        for (int i=0; i < Blob.Num_Scenes; i += 2)
        {
            BTPRINTF("%d) - name %d, val %d\n", 
                    1 + i, Blob.Scene_Index[i], Blob.Scene_Index[i + 1]);
        }
    }
    else { BTPRINTF("NOBLOB\n"); }
}


PRIVATE void dump_blob_triggers(void)
{
    if (Blob_Is_Loaded)
    {
        BTPRINTF("Triggers:\n");

        uint32_t* trigp = Blob.Trigger_Base;

        for (int i=0; i < (Blob.Num_Trig / 2) - 1; ++i, trigp += 2)
        {
            BTPRINTF("  %d - id= %d, prog= %d\n", i, trigp[0], trigp[1]);
        }
    }
    else { BTPRINTF("NOBLOB\n"); }
}


PRIVATE void dump_engines_state(void)
{
    BTPRINTF("NOT DONE\n");
}


PRIVATE void dump_blank_flash(void)
{
    uint16_t blanks = BPage_Blank_Pages();
    BTPRINTF("Blanks = %04.4X\n", blanks);
}


PRIVATE void dump_variable_table(void)
{
    if (Blob_Is_Loaded)
    {
        BLOB_VAR* varp = Blob.VarTab_Base;
        uint32_t n = Blob.Num_VarRecs;
        uint8_t* strs = Blob.StrindX;
        bool first = true;

        BTPRINTF("Variables:\n");

        while (n--)
        {
            if (varp->b >= 100)
            {
                if (first)
                {
                    BTPRINTF("  Global:\n");
                    first = false;
                }
                BTPRINTF("%d) '%s' %d %d\n", varp->b, strs + varp->a, varp->c, varp->d);
            }
            ++varp;
        }

        first = true;
        varp = Blob.VarTab_Base;
        n = Blob.Num_VarRecs;

        while (n--)
        {
            if (varp->b < 100)
            {
                if (first)
                {
                    BTPRINTF("  Local:\n");
                    first = false;
                }
                BTPRINTF("%d) '%s' %d %d\n", varp->b, strs + varp->a, varp->c, varp->d);
            }
            ++varp;
        }

//        uint32_t* varp = Blob.;

        // for (int i=0; i < (Blob.Num_Trig / 2) - 1; ++i, trigp += 2)
        // {
        //     BTPRINTF("  %d - id= %d, prog= %d\n", i, trigp[0], trigp[1]);
        // }
    }
    else { BTPRINTF("NOBLOB\n"); }

//     if (blob)
//     {
//         uint32_t* ptr = blob->

//         BTPRINTF("VarTab: Start %d, Size %d.\n", vtab, vtab_size);
//     }
//     else { BTPRINTF("No Blob Loaded.\n"); }
}


PUBLIC void mem_dump_p(void (*p)(char*, ...), void* ptr, size_t n)
{
	uint8_t* bptr = ptr;
	uint8_t count = 0;

	while (n--)
	{
		if (!count)
		{
			p("\n%8.8X: ", bptr);
			count = 15;
		}
		else --count;

		p("%2.2X ", *bptr++);
	}
	p("\n");
}

//extern void mem_dump_p(void (*p)(char*, ...), void* ptr, size_t n);
#define mem_dump_printf(ptr, size) mem_dump_p(PRINTF, ptr, size);
#define mem_dump_bluetooth(ptr, size) mem_dump_p(BTPRINTF, ptr, size);

PUBLIC void mem_dump(void* ptr, size_t n)
{
	uint8_t* bptr = ptr;
	uint8_t count = 0;

	while (n--)
	{
		if (!count)
		{
			BTPRINTF("\n%8.8X: ", bptr);
			count = 15;
		}
		else --count;

		BTPRINTF("%2.2X ", *bptr++);
	}
	BTPRINTF("\n");
}


PUBLIC void mem_dump_ints(void* ptr, size_t n)
{
	uint32_t* bptr = ptr;
	uint8_t count = 0;

	while (n--)
	{
		if (!count)
		{
			PRINTF("\n%8.8X: ", bptr);
			count = 4;
		}
		else --count;

		PRINTF("%8.8X ", *bptr++);
	}
	PRINTF("\n");
}

// PUBLIC void dump_heap_stats(void)
// {
//     PRINTF("Stack limit         : %p\r\n",&__StackLimit);
//     PRINTF("Stack One Top       : %p\r\n",&__StackOneTop);
//     PRINTF("Stack Top           : %p\r\n",&__StackTop);
//     PRINTF("Stack One Bottom    : %p\r\n",&__StackOneBottom);
//     PRINTF("Stack Bottom        : %p\r\n",&__StackBottom);
//     PRINTF("Heap top            : %p\r\n",(&__StackLimit - (10 * 1024)));
//     PRINTF("Heap limit          : %p\r\n",&__HeapLimit);
//     PRINTF("Heap size           : %u\r\n", ((&__StackLimit - (10 * 1024)) - &__HeapLimit));

//#include "pico/malloc.h"
    // struct heap_stats stats;
    // heap_get_stats(&stats);
    // printf("Total allocated bytes: %u\n", stats.total_allocated_bytes);
    // printf("Total free bytes: %u\n", stats.total_free_bytes);
// }


#define HELP_STRING "\
Dump:\n\
  1 - Version\n\
  2 - Device UUID.\n\
  3 - Phy Strings.\n\
  4 - Blob Checksum.\n\
  5 - Blob Stats\n\
  6 - Scene Index.\n\
  7 - Triggers.\n\
  8 - Dump bytes. (start)\n\
  9 - Dump ints. (start)\n\
  10 - Engine States.\n\
  11 - Blank Flash pages.\n\
  12 - Dump Variables.\n\
  \n"


PUBLIC void do_dump(int arg, int arg2)
{
    switch (arg)
    {
        case 1: { dump_version(); break; }
        case 2: { dump_uuid(); break; }
        case 3: { dump_phystr(); break; }
        case 4: { dump_checksum(); break; }
        case 5: { dump_blob_stats(); break; }
        case 6: { dump_blob_scene_index(); break; }
        case 7: { dump_blob_triggers(); break; }
        case 8: { mem_dump( (void*)arg2, 64); break; }
        case 9: { mem_dump_ints( (void*)arg2, 20); break; }
        case 10: { dump_engines_state(); break; }
        case 11: { dump_blank_flash(); break; }
        case 12: { dump_variable_table(); break; }
        case 100: { dump_info(); break; }
        case 0: default: { BTPRINTF(HELP_STRING); break; }
    }
}


// EndFile: Blob.c


// typedef enum Command
// {
//     COMMAND_END       = 0,    // end                      -- end of program, stop running.
//     COMMAND_UPDATE    = 1,    // upda                     -- update all leds that need updating.
//     COMMAND_YIELD     = 2,    // yiel                     -- wait until next tick.
//     COMMAND_WAIT      = 3,    // wait (n)                 -- wait for (n) milli seconds.
//     COMMAND_JUMP      = 4,    // jump (n)                 -- jump to a new line in routine.
//     COMMAND_CALL      = 5,    // call (n)                 -- call a sub routine.
//     COMMAND_REPEAT    = 6,    // repe (n)                 -- repeat a sequence (n) times.
//     COMMAND_SPHY      = 7,    // sphy (s)                 -- set current led string (1->8 is physical string), 0 is current, <1 all.
//     COMMAND_SCENE     = 8,    // scen (n)                 -- paint scene (n).
//     COMMAND_SHIFT     = 9,    // shif (+/-n)              -- shift led color values (values that are shifted off the end are lost).
//     COMMAND_ROTATE    = 10,    // rota (+/-n)              -- rotate led value. (end wraps).
//     COMMAND_MORPH     = 11,   // morp (t) [scene] (n)     -- morph current scene into new scene (n) over (t) seconds.
//     COMMAND_INTERRUPT = 12,   // intr (n)                 -- interrupt current routine.
//     COMMAND_QUEUE     = 13,   // queu (n)                 -- add routine to queue.
//     COMMAND_TRIGGER   = 14,   // trig (n) [routine] (n)   -- bind a trigger to a routine.
//     COMMAND_PHYS      = 15,   // phys (s) (n)             -- set number of leds for specific string.

//     COMMAND_LAST             // This must always be last!
// } COMMAND;

// #define CMD_MASK (0xFFFF)


// typedef struct Blob_State
// {
// 	STATE State;				// Start in IDLE state.

// 	uint32_t wait_counter;		// Ticks left to wait until next step.

// 	uint32_t repeat;			// Number of times left to repeat.
// 	PROG* repeat_start;			// First command in program sequence.
// 	PROG* prog;				    // Points to next program command.

// 	// XITI* Xiti;				// Transition currently playing.
// //	uint32_t* Xiti_Times;		// Store for transition step time counts.

// 	QUEUE* program_queue;		// Pending program stack.

// } BLOB_STATE;


// PUBLIC BLOB Blob = {0};
// PRIVATE BLOB_STATE Blob_State = {0};


// PRIVATE PROG* Prog_Ptr(PROG_ID id)
// //
// // Returns program pointer give program id.
// {
// 	return Blob.Program_Base + (id - 1);
// }

// PRIVATE PROG_ID Prog_Id(PROG *cmdp)
// //
// // Returns program id given program pointer.
// {
// 	return cmdp - Blob.Program_Base + 1;
// }


// PRIVATE const char* Command_Name(COMMAND cmd)
// //
// // Returns string given command code.
// {
//     const char* command_name[] = {
//             "end",  // COMMAND_END      -- end of program, stop running.
//             "upda", // COMMAND_UPDATE   -- update all leds that need updaing.
//             "yiel", // COMMAND_YIELD    -- wait until next tick.
//             "wait", // COMMAND_WAIT     -- wait for (n) seconds.
//             "jump", // COMMAND_JUMP     -- jump to a new line in routine.
//             "call", // COMMAND_CALL     -- call a sub routine.
//             "repe", // COMMAND_REPEAT   -- repeat a sequence (n) times.
//             "sphy", // COMMAND_SPHY     -- set current led string (1->8 is physical string), 0 is current, <1 all.
//             "scen", // COMMAND_SCENE    -- paint scene (n).
//             "shif", // COMMAND_SHIFT    -- shift led color values (values that are shifted off the end are lost).
//             "rota", // COMMAND_ROTATE   -- rotate led value. (end wraps).
//             "morp", // COMMAND_MORPH    -- morph current scene into new scene (n) over (t) seconds.
//             "intr", // COMMAND_INTRRUPT -- interrupt current routine.
//             "queu", // COMMAND_QUEUE    -- add routine to queue.
//             "trig", // COMMAND_TRIGGER  -- bind a trigger to a routine.
//             "phys", // COMMAND_PHYS     -- set number of leds for specific string.
//         };

//     cmd &= CMD_MASK;

//     if (cmd >= COMMAND_END && cmd < COMMAND_LAST)
//         return command_name[cmd];

//     return "";
// }
