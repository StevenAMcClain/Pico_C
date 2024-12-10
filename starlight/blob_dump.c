// File: Blob.c

#include "Common.h"
#include "Blob.h"

#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// //#include <time.h>

#include "bluetooth_stdio.h"

// #include "obled.h"
// #include "Queue.h"
// #include "Led.h"
// #include "Morph.h"
// #include "Scene.h"

// PRIVATE volatile uint64_t Blob_Time = 0;

// PUBLIC const int Tick_Speed = 
// //#ifdef DEBUG
// //1000000;
// //#else
// 1000;
// //#endif

// PRIVATE LED LED_Morph_Data[MAX_NUM_LEDS];

// #define PROG_QUEUE_SIZE 10   // Maximum number of pending programs.

// // #define RUNNING_QUEUE_SIZE 10   // Maximum number of pending programs.


// #define All_Program_Stop(msg, n) { printf(msg, n); Blob_Stop(); }

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

// typedef enum
// {
// 	STATE_IDLE,             // Nothing running.
// 	STATE_WAITING,          // In wait.
// 	STATE_COMMAND,          // Running commands.
// 	STATE_TRANSITION,       // In transition.

// } STATE;


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


PRIVATE void dump_blob_stats(void)
{
    BlueTooth_Printf("Blob Stats: \nNum triggers %d\n Prog Size %d\nNum Scenes %d\n Scene Size\n",
		Blob.Num_Trig, Blob.Num_Prog, Blob.Num_Scenes, Blob.Scene_Size);
}


PRIVATE void dump_blob_triggers(void)
{
    int i;
    uint32_t* Trig = Blob.Trigger_Base;

    printf("Triggers:\n");
    BlueTooth_Printf("Triggers:\n");

    for (i=0; i < (Blob.Num_Trig / 2) - 1; ++i, Trig += 2)
    {
        printf("  %d - id= %d, prog= %d\n", i, *Trig, *Trig+1);
        BlueTooth_Printf("  %d - id= %d, prog= %d\n", i, *Trig, *Trig+1);
    }
}


PRIVATE void dump_blob_scene_index(void)
{
    int i;

    printf("\nScenes:\n");
    BlueTooth_Printf("\nScenes\n");

    for (i=0; i < Blob.Num_Scenes; ++i)
    {
        printf("   %d - %d\n", i, Blob.Scene_Index[i]);
        BlueTooth_Printf("   %d - %d\n", i, Blob.Scene_Index[i]);
    }
}


PUBLIC void do_dump(int arg)
{
    switch (arg)
    {
        case 0:
        {
            printf("Dump:\n1 - Stats\n2 - Triggers.\n3 - Scene Index.");
            BlueTooth_Printf("Dump:\n1 - Stats\n2 - Triggers.\n3 - Scene Index.");
            break;
        }
        case 1:
        {
            dump_blob_stats();
            break;
        }
        case 2:
        {
            dump_blob_triggers();
            break;
        }
        case 3:
        {
            dump_blob_scene_index();
            break;
        }
    }
}


// EndFile: Blob.c

#ifdef DEBUG
PRIVATE void mem_dump(void* ptr, size_t n)
{
	uint8_t* bptr = ptr;
	uint8_t count = 0;

	while (n--)
	{
		if (!count)
		{
			printf("\n%8.8X: ", bptr);
			count = 16;
		}
		else --count;

		printf("%2.2X ", *bptr++);
	}
	printf("\n");
}
#endif

