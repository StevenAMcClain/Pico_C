// File: Blob.c

#include "common.h"

#include <hardware/flash.h>
//                                                 #include <pico/btstack_flash_bank.h>
#include "blob.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// //#include <time.h>

#include "btstdio.h"
#include "debug.h"
#include "led.h"

#define MAX_ASCII_BUFF_SIZE 100

PUBLIC void dump_ascii(char* head, uint8_t* abuff, size_t size)
{
    int i;
    uint8_t* ptr = abuff;
    uint8_t buff[MAX_ASCII_BUFF_SIZE] =  {0};
    uint8_t* buffp = buff;
    size_t chars_left = MAX_ASCII_BUFF_SIZE - 1;

    if (head && *head) 
    {
        size_t hsize = strlen(head);

        if (hsize > chars_left) { return; }  // Bail.

        strcpy(buffp, head);
        buffp += hsize;
        chars_left -= hsize;
    }

    if (size > chars_left) { return; }  // Bail.

    for (i = 0; i < size; ++i)
    {
        *buffp++ = (isprint(*ptr) ? *ptr : '?');
        *buffp = 0;
        ++ptr;
    }
    *buffp++ = '\t';
    *buffp = 0;

    printf(buff);
}


PRIVATE void dump_version(void)
{
    BlueTooth_Printf("VERS:%s\n", BLOB_VERSION);
}


PRIVATE void dump_uuid(void)
{
    uint8_t buff[8];
    flash_get_unique_id(buff);
    BlueTooth_Printf("UUID:%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                    buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7]);
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
            char *msg = "PhyStr List:\n%d) %d\n" 
                          + (first ? 0 : 13);
            BlueTooth_Printf(msg, phy_idx + 1, count);
            first = false;
        }
        ++phy_idx;
    }

    if (first) { BlueTooth_Printf("PhyStr List: Empty.\n"); }
}


PRIVATE void dump_checksum(void)
{
    if (Blob_Is_Loaded)
    {
        char *name = Blob.name;
        BlueTooth_Printf("Blob '%s', check %d\n", name, Blob.Blob_Checksum);
    }
    else { BlueTooth_Printf("*** NO BLOB ***\n"); }
}

#define MAX_DUMP_BUFF_SIZE 200

PRIVATE void dump_info(void)
{
    char buff[MAX_DUMP_BUFF_SIZE] = {0};
    char* bptr = buff;
    int chars_left = MAX_DUMP_BUFF_SIZE-1;
    int n;

    uint8_t uuid[8];
    flash_get_unique_id(uuid);
    n = snprintf(bptr, chars_left, "UUID: %02X%02X%02X%02X%02X%02X%02X%02X\nVERS: %s\n", 
                    uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], BLOB_VERSION);
    bptr += n;   chars_left -=n;

    if (Blob_Is_Loaded)
    {
        n = snprintf(bptr, chars_left, "Blob: '%s', Check: %d\n", Blob.name, Blob.Blob_Checksum);
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
        ++phy_idx;   mask <<= 1;
    }

    if (first) { n = snprintf(bptr, chars_left, "Phys: None.\n"); }
    else       { n = snprintf(bptr, chars_left, "\n"); }
    bptr += n;   chars_left -=n;

    n = snprintf(bptr, chars_left, "PhyMask: %X, %X\nDebug: %X\n", 
                                   phy_mask, Current_Phy_Mask, Debug_Mask);
    // bptr += n;   chars_left -=n;

    BlueTooth_Send_String(buff);
    printf("Dump: %s\n", buff);
}


#define STATS_STRING "\
Blob Stats: \n  Blob Size %d\n  Num triggers %d\n  Prog Size %d\n  Num Scenes %d\n  Scene Size %d\n\
  StrindX bytes %d\n  Number of symbols %d\n  VStr_Index_Size %d\n  VStr_Array_Size %d\n"

PRIVATE void dump_blob_stats(void)
{
    if (Blob_Is_Loaded)
    {
        BlueTooth_Printf(STATS_STRING,
                		 Blob.Blob_Size, Blob.Num_Trig, Blob.Num_Prog, Blob.Num_Scenes, Blob.Scene_Size,
                         Blob.StrindX_Size, Blob.SymTab_Size, Blob.VStr_Index_Size, Blob.VStr_Array_Size
                        );
    }
    else { BlueTooth_Printf("NOBLOB\n"); }
}


PRIVATE void dump_blob_scene_index(void)
{
    if (Blob_Is_Loaded)
    {
        BlueTooth_Printf("\nScenes:\n");

        for (int i=0; i < Blob.Num_Scenes; i += 2)
        {
            BlueTooth_Printf("%d) - name %d, val %d\n", 
                    1 + i, Blob.Scene_Index[i], Blob.Scene_Index[i + 1]);
        }
    }
    else { BlueTooth_Printf("NOBLOB\n"); }
}


PRIVATE void dump_blob_triggers(void)
{
    if (Blob_Is_Loaded)
    {
        BlueTooth_Printf("Triggers:\n");

        uint32_t* trigp = Blob.Trigger_Base;

        for (int i=0; i < (Blob.Num_Trig / 2) - 1; ++i, trigp += 2)
        {
            BlueTooth_Printf("  %d - id= %d, prog= %d\n", i, trigp[0], trigp[1]);
        }
    }
    else { BlueTooth_Printf("NOBLOB\n"); }
}



PUBLIC void mem_dump(void* ptr, size_t n)
{
	uint8_t* bptr = ptr;
	uint8_t count = 0;

	while (n--)
	{
		if (!count)
		{
			BlueTooth_Printf("\n%8.8X: ", bptr);
			count = 15;
		}
		else --count;

		BlueTooth_Printf("%2.2X ", *bptr++);
	}
	BlueTooth_Printf("\n");
}


#define MAX_DUMP_BUFF 500

PUBLIC void mem_dump_ints(void* ptr, size_t n)
{
	uint32_t* bptr = ptr;
	uint8_t count = 0;

    uint8_t buff[MAX_DUMP_BUFF] = {0};
    uint8_t* buffp = buff;
    size_t bsize = MAX_DUMP_BUFF - 1;

	while (n-- && bsize)
	{
        int n;

		if (!count)
		{
			n = snprintf(buffp, bsize, "\n%8.8X: ", bptr);
            buffp += n; bsize -= n;
			count = 4;
		}
		else --count;

        n = snprintf(buffp, bsize, "%8.8X ", *bptr++);
        buffp += n; bsize -= n;
	}
    n = snprintf(buffp, bsize, "\n");
    buffp += n; bsize -= n;

    printf(buff);
//    BlueTooth_Printf(buff);
}

#define DEV_STATS_STRING "\
Dev Stats:\nUptime %lld\nRetries %lld\nLost BT Rx Chars %lld\nMiss during read_chunk %lld\n"

extern volatile uint64_t Blob_Time;
extern uint64_t read_bytes_retry_counter;       // Number of retries... ever!
extern uint64_t lost_bluetooth_characters;      // Number of characters missed by bluetooth RX.
extern uint64_t total_miss_count;               // Number of miss recorded by get_chunk.


PRIVATE void dump_dev_stats(void)
{
    BlueTooth_Printf(DEV_STATS_STRING,
                        read_bytes_retry_counter, lost_bluetooth_characters, total_miss_count
                    );
}

extern void dump_stack();


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
  10 - Dev Stats\n\
  11 - Dump Stack\n\
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
        case 10: { dump_dev_stats(); break; }
        case 11: { dump_stack(); break; }
        case 100: { dump_info(); break; }
        case 0: default: { BlueTooth_Printf(HELP_STRING); break; }
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
