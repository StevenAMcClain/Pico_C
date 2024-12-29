// File: Blob.c

#include "common.h"
#include "blob.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth_stdio.h"

#include "debug.h"
#include "led.h"
#include "morph.h"
#include "obled.h"
#include "scene.h"
#include "stack.h"


PRIVATE volatile uint64_t Blob_Time = 0;

PUBLIC const int Tick_Speed = 
//#ifdef DEBUG
//1000000;
//#else
1000;
//#endif

#define CONTEXT_ITEMS 5         // Number of uint32_t that are saved.
#define CONTEXT_DEPTH 5         // Maximum number of saves.

#define PROG_STACK_SIZE (CONTEXT_ITEMS * CONTEXT_DEPTH)      // Maximum number of pending programs.


// #define RUNNING_QUEUE_SIZE 10   // Maximum number of pending programs.
// PRIVATE LED LED_Morph_Data[MAX_NUM_LEDS];
// #define All_Program_Stop(msg, n) { printf(msg, n); Blob_Stop(); }

typedef enum Command
{
    COMMAND_END       = 0,    // end                      -- end of program, stop running.
    COMMAND_UPDATE    = 1,    // upda                     -- update all leds that need updating.
    COMMAND_YIELD     = 2,    // yiel                     -- wait until next tick.
    COMMAND_WAIT      = 3,    // wait (n)                 -- wait for (n) milli seconds.
    COMMAND_JUMP      = 4,    // jump (n)                 -- jump to a new line in routine.
    COMMAND_CALL      = 5,    // call (n)                 -- call a sub routine.
    COMMAND_REPEAT    = 6,    // repe (n)                 -- repeat a sequence (n) times.
    COMMAND_SPHY      = 7,    // sphy (s)                 -- set current led string (1->8 is physical string), 0 is current, <1 all.
    COMMAND_SCENE     = 8,    // scen (n)                 -- paint scene (n).
    COMMAND_RENDER    = 9,    // rend (n)                 -- paint scene (n), no update.
    COMMAND_SHIFT     = 10,   // shif (+/-n)              -- shift led color values (values that are shifted off the end are lost).
    COMMAND_ROTATE    = 11,   // rota (+/-n)              -- rotate led value. (end wraps).
    COMMAND_MORPH     = 12,   // morp (t) [scene] (n)     -- morph current scene into new scene (n) over (t) seconds.
    COMMAND_INTERRUPT = 13,   // intr (n)                 -- interrupt current routine.
    COMMAND_QUEUE     = 14,   // queu (n)                 -- add routine to queue.
    COMMAND_TRIGGER   = 15,   // trig (n) [routine] (n)   -- bind a trigger to a routine.
    COMMAND_PHYS      = 16,   // phys (s) (n)             -- set number of leds for specific string.

    COMMAND_LAST              // This must always be last!
} COMMAND;

#define CMD_MASK (0xFFFF)

typedef enum
{
	STATE_IDLE,             // Nothing running.
	STATE_WAITING,          // In wait.
	STATE_COMMAND,          // Running commands.
	STATE_TRANSITION,       // In transition.

} STATE;


typedef struct Blob_State
{
	STATE State;				// Start in IDLE state.

	uint32_t wait_counter;		// Ticks left to wait until next step.

	uint32_t repeat;			// Number of times left to repeat.
	PROG* repeat_start;			// First command in program sequence.
	PROG* prog;				    // Points to next program command.

	// XITI* Xiti;				// Transition currently playing.
//	uint32_t* Xiti_Times;		// Store for transition step time counts.

	STACK* program_stack;		// Pending program stack. 

} BLOB_STATE;


PUBLIC BLOB Blob = {0};
PRIVATE BLOB_STATE Blob_State = {0};


PRIVATE PROG* Prog_Ptr(PROG_ID id)
//
// Returns program pointer give program id.
{
	return Blob.Program_Base + (id - 1);
}

PRIVATE PROG_ID Prog_Id(PROG *cmdp)
//
// Returns program id given program pointer.
{
	return cmdp - Blob.Program_Base + 1;
}


PRIVATE const char* Command_Name(COMMAND cmd)
//
// Returns string given command code.
{
    const char* command_name[] = {
            "end",  // COMMAND_END      -- end of program, stop running.
            "upda", // COMMAND_UPDATE   -- update all leds that need updaing.
            "yiel", // COMMAND_YIELD    -- wait until next tick.
            "wait", // COMMAND_WAIT     -- wait for (n) seconds.
            "jump", // COMMAND_JUMP     -- jump to a new line in routine.
            "call", // COMMAND_CALL     -- call a sub routine.
            "repe", // COMMAND_REPEAT   -- repeat a sequence (n) times.
            "sphy", // COMMAND_SPHY     -- set current led string (1->8 is physical string), 0 is current, <1 all.
            "scen", // COMMAND_SCENE    -- paint scene (n).
            "rend", // COMMAND_RENDER   -- paint scene (n), no update.
            "shif", // COMMAND_SHIFT    -- shift led color values (values that are shifted off the end are lost).
            "rota", // COMMAND_ROTATE   -- rotate led value. (end wraps).
            "morp", // COMMAND_MORPH    -- morph current scene into new scene (n) over (t) seconds.
            "intr", // COMMAND_INTRRUPT -- interrupt current routine.
            "queu", // COMMAND_QUEUE    -- add routine to queue.
            "trig", // COMMAND_TRIGGER  -- bind a trigger to a routine.
            "phys", // COMMAND_PHYS     -- set number of leds for specific string.
        };

    cmd &= CMD_MASK;

    if (cmd >= COMMAND_END && cmd < COMMAND_LAST)
        return command_name[cmd];

    return "";
}


PRIVATE void Push_Context(void)
//
// Saves current Blob_State.
{
	STACK* stk = Blob_State.program_stack;

	Stack_Push(stk,           Blob_State.State);		// Current state (processing command).
	Stack_Push(stk, (uint32_t)Blob_State.prog);			// Next command to run.
	Stack_Push(stk,           Blob_State.wait_counter);	// Ticks left to wait until next step.
	Stack_Push(stk,           Blob_State.repeat);		// Number of times left to repeat.
	Stack_Push(stk, (uint32_t)Blob_State.repeat_start);	// First command in program sequence.
	// XITI* Xiti;										// Transition currently playing.
//	Stack_Push(q, (uint32_t)Blob_State.Xiti_Times);		// Store for transition step time counts.

	Blob_State.repeat = 0;
//	Blob_State.Xiti_Times = 0;
}


PRIVATE void Pop_Context(void)
//
// Restores Blob_State.
{
	STACK* stk = Blob_State.program_stack;

//	Blob_State.Xiti_Times   = (uint32_t*)Stack_Pop(q);		// Store for transition step time counts.
	// XITI* Xiti;											// Transition currently playing.
	Blob_State.repeat_start = (PROG*)Stack_Pop(stk);			// First command in program sequence.
	Blob_State.repeat       =        Stack_Pop(stk);			// Number of times left to repeat.
	Blob_State.wait_counter =        Stack_Pop(stk);			// Ticks left to wait until next step.
	Blob_State.prog         = (PROG*)Stack_Pop(stk);			// Next command to run.
	Blob_State.State        =        Stack_Pop(stk);			// Current state (processing command).
}


// PUBLIC void Command_Shift_LEDS(int count, bool do_shift)
// //
// // Shift (or rotate) led array.
// {
// //#define NEXT_P(startp, p) ((++(p) >= ((startp) + Num_LEDS)) ? ((p)=(startp)) : (p))
// #define NEXT_P(startp, p) 
// //	if (count && (abs(count) < Num_LEDS))
// 	{
// //		LED* srcp_start = LED_Data;
// 		LED* srcp_start = 0;
// 		LED* dstp_start = 0; //ALT_LED_Data();

// 		LED* dstp = dstp_start;

// 		if (count > 0)
// 		{
// 			if (do_shift)
// 			{
// 				LED* srcp = srcp_start;

// //				int i = Num_LEDS;
// 				int i = 4;

// 				int c = count;
// 				while (c--)
// 				{
// 					dstp->val = 0;
// 					NEXT_P(dstp_start, dstp);
// 					--i;
// 				}

// 				while (i--)
// 				{
// 					dstp->val = srcp->val;
// 					NEXT_P(srcp_start, srcp);
// 					NEXT_P(dstp_start, dstp);
// 				}
// 			}
// 			else
// 			{
// //				LED* srcp = srcp_start + Num_LEDS - count;
// 				LED* srcp = 0;

// //				int i = Num_LEDS;
// 				int i = 4;

// 				while (i--)
// 				{
// 					dstp->val = srcp->val;
// 					NEXT_P(srcp_start, srcp);
// 					NEXT_P(dstp_start, dstp);
// 				}
// 			}
// 		}
// 		else
// 		{
// 			count = -count;

// 			if (do_shift)
// 			{
// 				LED* srcp = srcp_start + count;
// //				LED* end_srcp = srcp_start + Num_LEDS - count;
// 				LED* end_srcp = 0;

// 				while (srcp < end_srcp)
// 				{
// 					dstp->val = srcp->val;
// 					++srcp; ++dstp;
// 				}

// //				while (dstp < (dstp_start + Num_LEDS))
// 				{
// 					dstp->val = 0;
// 					++dstp;
// 				}
// 			}
// 			else
// 			{
// 				LED* srcp = srcp_start + count;
// //				LED* end_srcp = srcp_start + Num_LEDS - count;
// 				LED* end_srcp = 0;

// //				int i = Num_LEDS;
// 				int i = 4;

// 				while (i--)
// 				{
// 					dstp->val = srcp->val;
// 					++dstp;
// 					NEXT_P(srcp_start, srcp);
// 				}
// 			}
// 		}

// //		Switch_ALT_LED_Data();  // Flip to alternate.
// 	}
// }

#define DEBUG_BLOB2 (DEBUG_BLOB | DEBUG_BUSY)

PRIVATE bool Process_Command(PROG** cmdpp)
//
// Process the commands starting at cmdp.
// Return: false if command is completed.
{
    PROG* cmdp = *cmdpp;
	bool result = true;

	if (cmdp)
	{
    	bool done = false;

		while (result && !done)
		{
			COMMAND cmd = *(COMMAND*)cmdp++;

            cmd &= CMD_MASK;

			D(DEBUG_BLOB2, printf("PC [%d]: %d '%s'\n", 
                    Prog_Id(cmdp), cmd, Command_Name(cmd));)

			switch (cmd)
			{
				case COMMAND_END:      // end of program, stop running.
				{
					Blob_State.State = STATE_IDLE;
					cmdp = 0;
					result = false;
					break;
				}
                case COMMAND_UPDATE:    // Update LED Strings.
                {
                    LEDS_Do_Update();
                    break;
                }
				case COMMAND_YIELD:     // wait until next tick.
				{
					done = true;
					break;
				}
				case COMMAND_WAIT:     // wait for (n) seconds.
				{
					uint32_t arg = (uint32_t)*cmdp++;
					Blob_State.wait_counter = arg;
					Blob_State.State = STATE_WAITING;
					done = true;
					break;
				}
				case COMMAND_PHYS:    // set number of leds for specific string.
                {
					int32_t phynum = (int32_t)*cmdp++;	// Get the phynum to set.
					uint32_t num_leds = (PROG)*cmdp++;	// Get the number of leds on string.
                    PHY_Set_led_count(phynum, num_leds);
                    D(DEBUG_BLOB, printf(" *** PHYS %d %d\n", phynum, num_leds);)
                    break;
                }
				case COMMAND_SPHY:    // select current phy string.
				{
					int32_t arg = (int32_t)*cmdp++;
					LEDS_Set_Phynum(arg);
					break;
				}
				case COMMAND_SCENE:    // paint scene (n).
				{
					uint32_t arg = (uint32_t)*cmdp++;
					Set_Scene(arg);
                    LEDS_Do_Update();
					break;
				}
				case COMMAND_RENDER:    // paint scene (n), no update.
				{
					uint32_t arg = (uint32_t)*cmdp++;
					Set_Scene(arg);
					break;
				}
				case COMMAND_JUMP:     // jump to a new line in routine.
				{
					PROG arg = (PROG)*cmdp++;
					cmdp = Prog_Ptr(arg);
					break;
				}
				case COMMAND_CALL:     // call a sub routine.
				{
					PROG arg = (PROG)*cmdp++;
					D(DEBUG_BLOB, printf("Call: pgm 0x%X\n", arg);)

					Blob_State.prog = cmdp;   // Make sure State.prog is up to date.
					Push_Context();

					PROG* prog = Prog_Ptr(arg);
					cmdp = Blob_State.prog = prog;      // Set new prog pointer.
					break;
				}
				case COMMAND_REPEAT:   // repeat a sequence (n) times.
				{
					uint32_t repeat = (uint32_t)*cmdp++;	// Number of times left to repeat.
					PROG cmd_idx = (PROG)*cmdp++;	        // First command in program sequence.

					Blob_State.prog = cmdp;
					Push_Context();

					D(DEBUG_BLOB, printf("Repeat: %d, pgm %d\n", repeat, cmd_idx);)

					Blob_State.repeat = repeat;
					cmdp = Blob_State.repeat_start = Blob_State.prog = Prog_Ptr(cmd_idx);;
					break;
				}
				case COMMAND_SHIFT:    // shift led color values (values that are shifted off the end are lost).
				{
					int32_t shift = (int32_t)*cmdp++;			// Number of places to shift.
//					Command_Shift_LEDS(shift, true);
					break;
				}
				case COMMAND_ROTATE:   // rotate led value. (end wraps).
				{
					int32_t shift = (int32_t)*cmdp++;			// Number of places to rotate.
//					Command_Shift_LEDS(shift, false);
					break;
				}
				case COMMAND_MORPH:    // morph current scene into new scene (n) over (t) seconds.
					cmdp++;

				case COMMAND_QUEUE:     // add routine to queue.
				case COMMAND_INTERRUPT: // interrupt current routine.
					cmdp++;
				{
					D(DEBUG_BLOB, printf("Command(%d) '%s' is not implemented yet.\n", cmd, Command_Name(cmd));)
					break;
				}
				default:
				{
					D(DEBUG_BLOB, printf("Process_Command: Bad command %d\n", cmd);)
					break;
				}
			}
		}
	}
	else { result = false; }

    *cmdpp = cmdp;
	return result;
}



///--- Tick ---


PRIVATE void start_program(PROG_ID n)
//
// Start playing a program.
{
	if (n == 0) 
    {
        Blob_State.State = STATE_IDLE; 
        Blob_State.prog = 0;
    }
	else
	{
		D(DEBUG_BLOB, printf("start_program: n %d\n", n);)
		Blob_State.State = STATE_COMMAND;
		Blob_State.prog = Prog_Ptr(n);
	}
}


PRIVATE bool Blob_Program_Tick(struct repeating_timer* ptr)
//
// Tick for player state machine.    Called Tick_Speed times per second. 
{
	static int Tick_Count = 0;

	D(DEBUG_BLOB2, if (Blob_State.State)  { printf("\n== Blob_Tick %d: State %d\n", ++Tick_Count, Blob_State.State); })

	switch (Blob_State.State)
	{
		case STATE_IDLE: { break; }
		case STATE_WAITING:
		{
			if (Blob_State.wait_counter) { --Blob_State.wait_counter; }

			if (Blob_State.wait_counter)	// Waiting done?
			{
				break;  // Nope, still waiting.
			}
			else
			{
				Blob_State.State = STATE_COMMAND;
				// Fall throught to next case ... STATE_COMMAND
			}
		}
		case STATE_COMMAND:
		{
            bool cmd_is_running = Process_Command(&Blob_State.prog);

			if ( !cmd_is_running )
			{ 							// Command sequence ended.
				D(DEBUG_BLOB, printf("-- Sequence End.\n\n");)

				if (Blob_State.repeat)  // Repeating?
				{
					if (--Blob_State.repeat)
					{
						Blob_State.prog = Blob_State.repeat_start;
						Blob_State.State = STATE_COMMAND;
					}
					else  // Done repeat.
					{
						Pop_Context();
                        printf("Done repeat. next %X\n", Blob_State.prog);
					}
				}
				else if (Blob_State.program_stack->count)		// Anything on stack?
				{
					Pop_Context();
                    printf("next command %X\n", Blob_State.prog);
				}
				else
				{
                    printf("To Idle\n");
					Blob_State.State = STATE_IDLE; 
				}
			}
			break;
		}
		case STATE_TRANSITION:
		{
			D(DEBUG_BLOB, printf("Transition State:\n");)
			Blob_State.State = STATE_COMMAND;
//			transition_step();
			break;
		}
	}
	D(DEBUG_BLOB2, if (Blob_State.State)  { printf("== Blob_Tick: Done (%d).\n\n", Tick_Count); })

	return true;
}

//--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

PUBLIC void Blob_Stop(void)
//
// Stop current program and clear stack.
{
	Stack_Clear(Blob_State.program_stack); 
	Blob_State.State = STATE_IDLE;
	Blob_State.repeat = 0;
	Blob_State.wait_counter = 0;
}


PRIVATE PROG_ID Get_Trigger_Prog(TRIG_ID id)
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


PUBLIC void Blob_Trigger(TRIG_ID n)
//
// Immediately Start playing a BLOB program.  (Cancel any running or queue'd)
{
	Blob_Stop();

	PROG_ID pnum = Get_Trigger_Prog(n);

	if (pnum)
	{
		D(DEBUG_BLOB, printf("Blob_Trigger: n %d, program: %d\n", n, pnum);)
		start_program(pnum);
	}	
}


PUBLIC void Blob_Queue_Next(TRIG_ID n)
//
// Start playing a BLOB program (after current completes).
{
	if (Blob_State.State == STATE_IDLE)		// Nothing running right now?
	{
		Blob_Trigger(n);					// Same as trig.
	}
	else if (n && n <= Blob.Num_Trig)
	{
		D(DEBUG_BLOB, printf("Blob_Queue_Next: n %d\n", n);)
		Blob_Stop();

		PROG_ID pnum = Blob.Trigger_Base[n-1];
//		Queue_Add(Blob_State.program_queue, pnum);		// Add to back of queue.
	}
}


PUBLIC void Blob_Unload(void)
//
// Release blob_base memory.
{
	if (Blob.Blob_Base)
	{
		Blob_Stop();

		uint8_t* base = Blob.Blob_Base - sizeof(uint32_t);

		D(DEBUG_BLOB, printf("Blob_Unload: %X\n", base);)
		free(base); 

		memset(&Blob, 0, sizeof(Blob));
	}
}


typedef struct
{
	uint32_t strindx_start;	    // Start of the string table.
	uint32_t strindx_size;	    // Size of the string table.

	uint32_t symtab_start;	    // Start of the program symbol table
	uint32_t symtab_size;	    // Size of the program sysmbol table.

	uint32_t vstr_index;	    // Virual LED string array index starts here (relative to start).
	uint32_t vstr_count;	    // Number of Virual LED string arrays defined.

	uint32_t vstr_array;	    // Virual LED string record start here.
	uint32_t vstr_size;   	    // Size of the Virual LED string array.

    uint32_t scen_index;	    // Scene index starts here (relative to start).
    uint32_t scen_count;        // Number of scenes defined.

    uint32_t scen_array;        // Scene array starts here (relative to start).
    uint32_t scen_size;		    // Size of scene array.

    uint32_t trig_start;	    // Trigger array starts here (relative to start).
    uint32_t trig_size;		    // Number of triggers defined.

    uint32_t prog_start;	    // Program array starts here (relative to start).
    uint32_t prog_size;		    // Size of program array.

} BLOB_HEAD;


PUBLIC void Blob_Load(uint8_t* blob_base)
//
// Load a new blob_base.
{
	if (blob_base)
	{
		Blob_Unload();

	    BLOB_HEAD* bhptr = (BLOB_HEAD*)blob_base;
    
		D(DEBUG_BLOB, printf("trig_size=%d, prog_size=%d, scen_count=%d, scen_size=%d\n",
				(bhptr->trig_size / 2) - 1, bhptr->prog_size, bhptr->scen_count, bhptr->scen_size);)

		Blob_State.State = STATE_IDLE;

		Blob.Blob_Base = blob_base;
		Blob.Blob_Size = *(uint32_t*)(blob_base - sizeof(uint32_t));

	    uint32_t* bptr = (uint32_t*)blob_base;

        Blob.StrindX = (uint8_t*)(bptr + bhptr->strindx_start);     // Point to base of string table.

        Blob.SymTab = bptr + bhptr->symtab_start;       // Pointer to start of symbol table.

        Blob.VStr_Index = bptr + bhptr->vstr_index;     // Pointer to base of virtual string index.
        Blob.VStr_Array = bptr + bhptr->vstr_array;     // Pointer to base of virtual string array.

        Blob.Scene_Index = (SCENE_ID*)(bptr + bhptr->scen_index);	// Scene index start here.
        Blob.Scene_Array = (SCENE*)   (bptr + bhptr->scen_array);	// Scene array start here.

        Blob.Trigger_Base = (PROG_ID*) (bptr + bhptr->trig_start);  // Start of the trigger table.
        Blob.Program_Base = (PROG*)    (bptr + bhptr->prog_start);	// Blob Programs start here.

        Blob.StrindX_Size = bhptr->strindx_size;
        Blob.SymTab_Size = bhptr->symtab_size / 2;

        Blob.VStr_Index_Size = bhptr->vstr_count;
        Blob.VStr_Array_Size = bhptr->vstr_size;
            
        Blob.Num_Scenes = bhptr->scen_count / 2;    // Number of scenes defined.
        Blob.Scene_Size = bhptr->scen_size;	        // Sizeof the scene array.
        Blob.Num_Trig = bhptr->trig_size / 2;   	// Number of trigger records.
        Blob.Num_Prog = bhptr->prog_size;		    // Number of PROG records.

		start_program(1);			// Always start running program from start.
	}
}

//#define XITI_TIMES_SIZE(n) (sizeof(uint32_t) * (n) * LED_SIZE)


PRIVATE bool Blob_Time_Tick(struct repeating_timer* ptr)
{
	++Blob_Time;   // uint64_t
}

PUBLIC void Blob_Init(void)
//
// Prepare BLOB for use.  Call once at startup.
{
	static struct repeating_timer blob_timer_1;		// Timer to call Blob_Tick.
	static struct repeating_timer blob_timer_2;		// Timer to call Blob_Time.

	Blob_State.program_stack = Stack_Initialize(PROG_STACK_SIZE); 

	add_repeating_timer_us(Tick_Speed, Blob_Time_Tick, NULL, &blob_timer_1);
	add_repeating_timer_us(Tick_Speed, Blob_Program_Tick, NULL, &blob_timer_2);
}


// EndFile: Blob.c
