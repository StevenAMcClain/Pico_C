// File: Blob.c

#include "Common.h"
#include "Blob.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>

#include "obled.h"
#include "Queue.h"
#include "Led.h"
#include "Morph.h"


PUBLIC const int Tick_Speed = 
#ifdef DEBUG
1000000;
#else
1000;
#endif

PRIVATE LED LED_Morph_Data[MAX_NUM_LEDS];

#define PROG_QUEUE_SIZE 10   // Maximum number of pending programs.

// #define RUNNING_QUEUE_SIZE 10   // Maximum number of pending programs.


// #define All_Program_Stop(msg, n) { printf(msg, n); Blob_Stop(); }

typedef enum Command
{
    COMMAND_END       = 0,    // end                      -- end of program, stop running.
    COMMAND_YIELD     = 1,    // yiel                     -- wait until next tick.
    COMMAND_WAIT      = 2,    // wait (n)                 -- wait for (n) milli seconds.
    COMMAND_JUMP      = 3,    // jump (n)                 -- jump to a new line in routine.
    COMMAND_CALL      = 4,    // call (n)                 -- call a sub routine.
    COMMAND_REPEAT    = 5,    // repe (n)                 -- repeat a sequence (n) times.
    COMMAND_SCENE     = 6,    // scen (n)                 -- paint scene (n).
    COMMAND_SHIFT     = 7,    // shif (+/-n)              -- shift led color values (values that are shifted off the end are lost).
    COMMAND_ROTATE    = 8,    // rota (+/-n)              -- rotate led value. (end wraps).
    COMMAND_MORPH     = 9,    // morp (t) [scene] (n)     -- morph current scene into new scene (n) over (t) seconds.
    COMMAND_INTERRUPT = 10,   // intr (n)                 -- interrupt current routine.
    COMMAND_QUEUE     = 11,   // queu (n)                 -- add routine to queue.
    COMMAND_TRIGGER   = 12,   // trig (n) [routine] (n)   -- bind a trigger to a routine.
    COMMAND_LEDS      = 13,   // leds (n)                 -- set number of leds.

    COMMAND_LAST             // This must always be last!
} COMMAND;


typedef enum
{
	STATE_IDLE,             // Nothing running.
	STATE_WAITING,          // In wait.
	STATE_COMMAND,          // Running commands.
	STATE_SCENE,			// Setting a scene.
	STATE_TRANSITION,       // In transition.

} STATE;


typedef struct Blob_State
{
	QUEUE* program_queue;		// Pending program stack.

	STATE State;				// Start in IDLE state.

	uint32_t wait_counter;		// Ticks left to wait until next step.

	uint32_t repeat;			// Number of times left to repeat.
	PROG* repeat_start;			// First command in program sequence.
	PROG* prog;				    // Points to next program command.

	SCENE_ID Scene_ID;			// Used for scene state.

	// XITI* Xiti;					// Transition currently playing.
	uint32_t* Xiti_Times;		// Store for transition step time counts.

} BLOB_STATE;


PRIVATE BLOB Blob = {0};
PRIVATE BLOB_STATE Blob_State = {0};


PRIVATE PROG* Prog_Ptr(PROG_ID id)
{
	return Blob.Program_Base + (id - 1);	
}

PRIVATE PROG_ID Prog_Id(PROG *cmdp)
{
	return cmdp - Blob.Program_Base + 1;
}


PUBLIC const char* Command_Name(COMMAND cmd)
{
    const char* command_name[] = {
            "end",  // COMMAND_END      -- end of program, stop running.
            "yiel", // COMMAND_YIELD    -- wait until next tick.
            "wait", // COMMAND_WAIT     -- wait for (n) seconds.
            "jump", // COMMAND_JUMP     -- jump to a new line in routine.
            "call", // COMMAND_CALL     -- call a sub routine.
            "repe", // COMMAND_REPEAT   -- repeat a sequence (n) times.
            "scen", // COMMAND_SCENE    -- paint scene (n).
            "shif", // COMMAND_SHIFT    -- shift led color values (values that are shifted off the end are lost).
            "rota", // COMMAND_ROTATE   -- rotate led value. (end wraps).
            "morp", // COMMAND_MORPH    -- morph current scene into new scene (n) over (t) seconds.
            "intr", // COMMAND_INTRRUPT -- interrupt current routine.
            "queu", // COMMAND_QUEUE    -- add routine to queue.
            "trig", // COMMAND_TRIGGER  -- bind a trigger to a routine.
            "leds", // COMMAND_LEDS     -- set number of leds.
        };

    if (cmd >= COMMAND_END && cmd < COMMAND_LAST)
        return command_name[cmd];

    return "";
}


PRIVATE void Push_Context(void)
{
	QUEUE* q = Blob_State.program_queue;

	Queue_Add(q, (uint32_t)Blob_State.Xiti_Times);		// Store for transition step time counts.
	// XITI* Xiti;										// Transition currently playing.
	Queue_Add(q,           Blob_State.Scene_ID);		// Used for scene state.
	Queue_Add(q, (uint32_t)Blob_State.repeat_start);	// First command in program sequence.
	Queue_Add(q,           Blob_State.repeat);			// Number of times left to repeat.
	Queue_Add(q,           Blob_State.wait_counter);	// Ticks left to wait until next step.
	Queue_Add(q, (uint32_t)Blob_State.prog);
	Queue_Add(q,           Blob_State.State);

	Blob_State.repeat = 0;
	Blob_State.Xiti_Times = 0;
}


PRIVATE void Pop_Context(void)
{
	QUEUE* q = Blob_State.program_queue;

	Blob_State.Xiti_Times   = (uint32_t*)Queue_Pop(q);		// Store for transition step time counts.
	// XITI* Xiti;								// Transition currently playing.
	Blob_State.Scene_ID     =        Queue_Pop(q);	// Used for scene state.
	Blob_State.repeat_start = (PROG*)Queue_Pop(q);	// First command in program sequence.
	Blob_State.repeat       =        Queue_Pop(q);	// Number of times left to repeat.
	Blob_State.wait_counter =        Queue_Pop(q);	// Ticks left to wait until next step.
	Blob_State.prog         = (PROG*)Queue_Pop(q);
	Blob_State.State        =        Queue_Pop(q);
}


PRIVATE bool Process_Command(void)
{
	bool done = false;
	bool result = true;
	PROG* cmdp = Blob_State.prog;

	if (cmdp)
	{
		while (result && !done)
		{
			D(printf("PC [%d]:", Prog_Id(cmdp));)

			COMMAND cmd = *(COMMAND*)cmdp++;

			D(printf("%d '%s'\n", cmd, Command_Name(cmd));)

			switch (cmd)
			{
				case COMMAND_END:      // end of program, stop running.
				{
					Blob_State.State = STATE_IDLE;
					cmdp = 0;
					result = false;
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
				case COMMAND_SCENE:    // paint scene (n).
				{
					uint32_t arg = (uint32_t)*cmdp++;
					Blob_State.Scene_ID = arg;
					Blob_State.State = STATE_SCENE;
					done = true;
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
					printf("Call: pgm 0x%X\n", arg);

					Blob_State.prog = cmdp;
					Push_Context();

					PROG* prog = Prog_Ptr(arg);
					cmdp = Blob_State.prog = prog;
					break;
				}
				case COMMAND_REPEAT:   // repeat a sequence (n) times.
				{
					uint32_t repeat = (uint32_t)*cmdp++;			// Number of times left to repeat.
					PROG cmd_idx = (PROG)*cmdp++;	// First command in program sequence.

					Blob_State.prog = cmdp;
					Push_Context();

					printf("Repeat: %d, pgm %d\n", repeat, cmd_idx);

					Blob_State.repeat = repeat;
					cmdp = Blob_State.repeat_start = Blob_State.prog = Prog_Ptr(cmd_idx);;
					break;
				}
				case COMMAND_MORPH:    // morph current scene into new scene (n) over (t) seconds.
//				case COMMAND_TRIGGER:  // bind a trigger to a routine.
					cmdp++;
				case COMMAND_SHIFT:    // shift led color values (values that are shifted off the end are lost).
				case COMMAND_ROTATE:   // rotate led value. (end wraps).

				case COMMAND_QUEUE:     // add routine to queue.
				case COMMAND_INTERRUPT: // interrupt current routine.
				case COMMAND_LEDS:      // set number of leds.
					cmdp++;
				{
					printf("Command(%d) '%s' is not implemented yet.\n", cmd, Command_Name(cmd));
					break;
				}
				default:
				{
					printf("Process_Command: Bad command %d\n", cmd);
					break;
				}
			}
		}
	}
	else { result = false; }

	Blob_State.prog = cmdp;

	return result;
}


///--- Scene ---

PRIVATE void Unpack_Scene(uint32_t val, uint8_t* buff)
{
	buff[0] = val & 0xFF;
	buff[1] = (val >> 8) & 0xFF;
	buff[2] = (val >> 16) & 0xFF;
	buff[3] = (val >> 24) & 0xFF;
}

#define FLAGS_END      0xFF
#define FLAGS_END_ALL  0xFE
#define FLAGS_END_LAST 0xFD
#define FLAGS_SKIP     0xFC


PRIVATE void Set_Scene(SCENE_ID id)
{
	if (id)
	{
		D(printf("Set scene: id = %d, ", id);)

		--id;

		uint32_t start_idx = Blob.Scene_Index[id];

		D(printf("start_idx= %d\n", start_idx);)

		uint32_t* start_ptr = Blob.Scene_Array + start_idx;
		uint32_t* ptr = start_ptr;

		uint32_t i = 0;

		while (i < Num_LEDS)
		{
			uint8_t buff[4];
			D(printf(": i = %d, ", i);)

			Unpack_Scene(*ptr++, buff);

			uint8_t flags = buff[0];

			if (flags == FLAGS_END)
			{
				D(printf("End\n");)
				break;
			}
			else if (flags == FLAGS_END_ALL)  
			{
				D(printf("End All\n");)
				ptr = start_ptr;
			}
			else if (flags == FLAGS_END_LAST)
			{
				D(printf("End Last");)
				while (i < Num_LEDS)
				{
					LED_Set_RGB(i, buff[1], buff[2], buff[3]);
					++i;
					D(printf(".");)
				}
				D(printf("\n");)
			}
			else if (flags == FLAGS_SKIP)
			{
				D(printf("skip %d\n", buff[1]);)
				 i += buff[1];
			}
			else
			{
				D(printf("%d %d %d", buff[1], buff[2], buff[3]);)
				int count = flags ? flags : 1;
				while (count--)
				{
					LED_Set_RGB(i, buff[1], buff[2], buff[3]);
					++i;
					D(printf(".");)
				}
				D(printf("\n");)
			}
		}
		LED_Update();
	}
}

///--- Tick ---


PRIVATE void start_program(PROG_ID n)
//
// Start playing a program.
{
	if (n == 0)
	{
		Blob_State.State = STATE_IDLE;
	}
	else
	{
		D(printf("start_program: n %d\n", n);)
		Blob_State.State = STATE_COMMAND;
		Blob_State.prog = Prog_Ptr(n);
		Process_Command();
	}

}


PUBLIC bool Blob_Tick(struct repeating_timer* ptr)
//
// Tick for player state machine.    Called Tick_Speed times per second. 
{
	static int Tick_Count = 0;

	LED_Do_Update();

//	ObLED_On();

	D(if (Blob_State.State)  { printf("\n== Blob_Tick %d: State %d\n", ++Tick_Count, Blob_State.State); })

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
			if ( !Process_Command() )
			{ 							// Command sequence ended.
				printf("-- Sequence End.\n\n");

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
					}
				}
				else if (Blob_State.program_queue->count)		// Anything in queue?
				{
					Pop_Context();
				}
				else
				{
					Blob_State.State = STATE_IDLE; 
				}
			}
			break;
		}
		case STATE_SCENE:
		{
			Set_Scene(Blob_State.Scene_ID);
			Blob_State.State = STATE_COMMAND;
			break;
		}
		case STATE_TRANSITION:
		{
			printf("Transition State:\n");
			Blob_State.State = STATE_COMMAND;
//			transition_step();
			break;
		}
	}
	D(if (Blob_State.State)  { printf("== Blob_Tick: Done (%d).\n\n", Tick_Count); })

//	ObLED_Off();
	return true;
}

//--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

PUBLIC void Blob_Stop(void)
//
// Stop current program and clear queues.
{
	Queue_Clear(Blob_State.program_queue); 
	Blob_State.State = STATE_IDLE;
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
		printf("Blob_Trigger: n %d, program: %d\n", n, pnum);
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
		printf("Blob_Queue_Next: n %d\n", n);
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
		uint8_t* base = Blob.Blob_Base - sizeof(uint32_t);

		printf("Blob_Unload: %X\n", base);
		free(base); 

		memset(&Blob, 0, sizeof(Blob));
	}
}


PUBLIC void Blob_Load(uint8_t* blob_base)
//
// Load a new blob_base.
{
	if (blob_base)
	{
		Blob_Unload();

		uint32_t* bptr = (uint32_t*)blob_base;

		uint32_t trig_start = *bptr++;
		uint32_t trig_size  = *bptr++;

		uint32_t prog_start = *bptr++;
		uint32_t prog_size  = *bptr++;

		uint32_t scen_index = *bptr++;
		uint32_t scen_count = *bptr++;

		uint32_t scen_array = *bptr++;
		uint32_t scen_size  = *bptr++;

		printf("trig_size=%d, prog_size=%d, scen_count=%d, scen_size=%d\n",
				(trig_size / 2) - 1, prog_size, scen_count, scen_size);

		Blob_State.State = STATE_IDLE;

		Blob.Blob_Base = blob_base;
		Blob.Blob_Size = *blob_base - sizeof(uint32_t);

		bptr = (uint32_t*)blob_base;   // Reset bptr.

		Blob.Trigger_Base = (PROG_ID*)(bptr + trig_start);
		Blob.Program_Base = (PROG*)   (bptr + prog_start);
		Blob.Scene_Index = (SCENE_ID*)(bptr + scen_index);
		Blob.Scene_Array = (SCENE*)   (bptr + scen_array);

		Blob.Num_Trig = trig_size;		// Number of trigger records.
		Blob.Num_Prog = prog_size;		// Number of PROG records.
		Blob.Num_Scenes = scen_count;	// Number of Scenes defined.
		Blob.Scene_Size = scen_size;	// Sizeof Scenes array.

		int i;
		uint32_t* Trig = Blob.Trigger_Base;

		for (i=0; i < (Blob.Num_Trig / 2) - 1; ++i)
		{
			printf("\tTrigger(%d) id= %d, prog= %d\n", i, *Trig++, *Trig++);
		}

		start_program(1);			// Always start running program from start.
	}
}

#define XITI_TIMES_SIZE(n) (sizeof(uint32_t) * (n) * LED_SIZE)

PUBLIC void Blob_NumLeds(int n)
//
// Set number of LEDS.   Call when Num_Leds changes.
{
	if (Blob_State.Xiti_Times)
	{
		if (n == Blob.Num_Leds)
		{
			return;  // already setup.
		}
		else
		{
			free(Blob_State.Xiti_Times);
			Blob_State.Xiti_Times = NULL;
		}
	}
	size_t size = XITI_TIMES_SIZE(n);

	uint32_t* xiti_times = malloc(size);

	if (xiti_times)
	{
		memset(xiti_times, 0, size);
		Blob_State.Xiti_Times = xiti_times;
		Blob.Num_Leds = n;
	}
}


PUBLIC void Blob_Init(void)
//
// Prepare BLOB for use.  Call once at startup.
{
	static struct repeating_timer blob_timer;		// Timer to generate Bob_Tick.

	Blob_State.program_queue = Queue_Initialize(PROG_QUEUE_SIZE); 

	Blob_NumLeds(Num_LEDS);

	add_repeating_timer_us(Tick_Speed, Blob_Tick, NULL, &blob_timer);
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


// #define PROG_SIZE 8			// sizeof(PROG)
// #define SEQU_SIZE 12		// sizeof(SEQ)
// #define SCEN_SIZE (LED_DATA_SIZE)		// One byte for each color of each led.
// #define XITI_SIZE (sizeof(uint32_t) + (sizeof(XITI) * LED_DATA_SIZE))

// PRIVATE uint8_t* get_scene(SCENE_ID n)
// //
// // Get a pointer to first value in scene.
// // Returns: NULL if invalid scene number.  (n == 0 or n > Num_Scen)
// {
// 	return (n && n <= Blob.Num_Scenes) ? &Blob.Scene_Base[(n - 1) * SCEN_SIZE] : NULL;
// }


// PRIVATE bool set_scene(SCENE_ID n)
// //
// // Copy Scene(n) to LED array.   (instant transition).
// // Returns: false if invalid scene number.  (n == 0 or n > Num_Scen)
// {
// 	uint8_t* sptr = get_scene(n);

// 	if (sptr)
// 	{
// 		for (int i = 0;  i < Num_LEDS; ++i)
// 		{
// 			led_set_buf(i, sptr);
// 			sptr += LED_SIZE;
// 		}
// 		led_send();
// 	}
// }


// PRIVATE bool start_sequence(SEQ_ID n);

// PRIVATE void transition_step(void)
// {
// 	if (morph_step())
// 	{
//  		start_sequence(Blob_State.sequence_num + 1);
// 	}
// }


// PRIVATE void transition_step(void)
// //
// // Perform next step in the current transition.
// // When transition is completed move on to next program step.
// {
// 	bool in_transition = false;				// set to true if still in transition.

// 	if (Blob_State.trans_time)				// Is there still time left before hittng trans_time?
// 	{
// 		if (--Blob_State.trans_time)	// Not the last tick?
// 		{
// 			XITI* xitip = Blob_State.Xiti;							// Current transition.
// 			uint8_t* ledp = led_data;								// Current value.
// 			uint8_t* scene = get_scene(Blob_State.Seq->scene);		// Target value.
// 			uint32_t* xtime = Blob_State.Xiti_Times;				// Current tick value for each XITI.
// 			bool do_send = false;									// At least one Led color changed.

// 			D(printf("transition step: ttime %d\n", Blob_State.trans_time);)

// 			for (int i = 0; i < LED_DATA_SIZE; ++i, ++scene, ++ledp, ++xitip, ++xtime)    // For each color of each led.
// 			{
// 				int8_t step_size = xitip->step_size;		// Get step size.

// 				if (step_size && *ledp != *scene)		// Is current value at target yet?
// 				{
// 					in_transition = true;     // Transition still running.

// 					if (*xtime)  // Waiting for next change?
// 					{
// 						--*xtime;   // Count one more tick.
// 					}
// 					else  // do transition step.
// 					{
// 						D(printf("add(%d): before %d, step %d", i, *ledp, step_size);)
// 						*xtime = xitip->step_time;  // Reset the counter.
// 						step_size = (step_size > 0) ? MIN(step_size, 255 - *ledp) : MIN(step_size, *ledp);
// 						*ledp += step_size;
// 						D(printf(", after %d, step %d\n", *ledp, step_size);)
// 						do_send = true;
// 					}
// 				}
// 			}
// 			if (do_send) { led_send(); }
// 		}
// 	}
// 	if (!in_transition) 
// 	{
// 		D(printf("transition_step: Start next sequence(%d).\n", Blob_State.sequence_num + 1);)
// 		start_sequence(Blob_State.sequence_num + 1);
		
// 		// printf("not transition: Step_Done\n");
// 		// Program_Step_Done();
// 	}
// }


// PRIVATE void start_transition(XITI_ID n, SCENE_ID scene_id)
// {
//  	if (n && n <= Blob.Num_Xiti)
//  	{
//  		uint32_t* timep = (uint32_t*)(Blob.Xiti_Base + ((n-1) * XITI_SIZE));		 // Get pointer to transistion.
// 		FLOAT trans_time = (FLOAT)*(timep++) / 1000.0;
// 		uint8_t* to_scene = get_scene(n);
// 		morph_start(trans_time, to_scene);
// 		Blob_State.State = STATE_TRANSITION;			// Now in transition state.
// 	}
// }


// PRIVATE void start_transition(XITI_ID n)
// {
// 	if (n && n <= Blob.Num_Xiti)
// 	{
// 		uint32_t* timep = (uint32_t*)(Blob.Xiti_Base + ((n-1) * XITI_SIZE));		 // Get pointer to transistion.

// 		Blob_State.trans_time = *timep;						// Set max transistion time.

// 		XITI* xitip = (XITI*)(timep + 1);					// Get pointer to first XITI record.

// 		uint32_t* xtimep = Blob_State.Xiti_Times;			// Point to first transition timer value.

// 		// printf("start_transition: set times (");
// 		if (xitip && xtimep)
// 		{
// 			for (int i = 0; i < LED_DATA_SIZE; ++i)		// Copy initial transistion time values.
// 			{
// 				// printf("t(%d)=%d, ", i, xitip->step_time);
// 				*xtimep++ = xitip->step_time;
// 				++xitip;
// 			}
// 			// (" done.\n");

// 			Blob_State.Xiti = (XITI*)(timep + 1);			// Set as current transition.
// 			Blob_State.State = STATE_TRANSITION;			// Now in transition state.
// 		}

// 		transition_step();									// Do first step of the transiiton.
// 	}
// 	else { All_Program_Stop("start_transition: Bad (%d), Done. *********************************\n\n", n); }
// }

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// PRIVATE bool Program_Step_Done(void);

// PRIVATE bool start_sequence(SEQ_ID n)
// //
// // Start to play a sequence.
// {
// 	bool program_done = false;
// // top:
// 	D(printf("start_sequence: n %d\n", n);)

// 	if (n && n <= Blob.Num_Sequ)
// 	{
// 		SEQ* seq = &Blob.Sequ_Base[n-1];

// 		Blob_State.sequence_num = n;
// 		Blob_State.Seq = seq;

// 		D(printf("get_seq(%d): hold %d, xiti %d, scene %d\n", 
// 				          n, seq->hold, seq->xiti, seq->scene);)

// 		uint32_t hold = seq->hold;

// 		if (!hold)
// 		{
// 			uint32_t xiti = seq->xiti;
			
// 			if (!xiti)					// No transistion.
// 			{
// 				D(printf("start_sequence: Set Scene(%d).\n", seq->scene);)

// 				if (set_scene(seq->scene))		// Set scene immediate.
// 				{
// 					Blob_State.hold_counter = hold;
// 					Blob_State.State = STATE_HOLDING;
// 					// n = Blob_State.sequence_num + 1;		// Start next sequence immediately
// 					// goto top;
// 				}
// 				else 
// 				{
// 					// E(printf("start_sequence: Bad Scene Program Done. *********************************\n\n");)
// 					program_done = Program_Step_Done();
// 				} 
// 			}
// 			else
// 			{
// 				D(printf("start_sequence: Start transition.\n");)
// 				start_transition(xiti, seq->scene);
// 			}
// 		}
// 		else
// 		{
// 			D(printf("start_sequence: Start hold (%d).\n", hold);)
// 			Blob_State.hold_counter = hold;
// 			Blob_State.State = STATE_HOLDING;
// 		}
// 	}
// 	else 
// 	{
// 		E(printf("start_sequence: Bad Sequence Program Done. *********************************\n\n");)
// 		program_done = Program_Step_Done();
// 	} 
// 	return program_done;
// }
 

//PRIVATE void start_program(uint32_t n);

// PRIVATE bool Program_Step_Done(void)
// //
// // Called when sequence is completed.
// // Either repeat program sequence or start next program.
// // If program is ended then pop off Running_Queue and continue.
// // If Running_Queue is empty then get next program from Program_Queue.
// // If Program_Queue is empty then go back to idle.
// {
// 	bool end = true;

// 	while (end)
// 	{
// 		if (Blob_State.Prog->repeat != 0 && (--Blob_State.repeat == 0))
// 		{
// 			uint32_t pnum = Blob_State.prog_num + 1;
// 			PROG* prog = &Blob.Prog_Base[pnum-1];		// Get program record.

// 			if (prog->sequence)			// Last program in series?
// 			{
// 				start_program(pnum);	// No, start next program in series.
// 			}
// 			else
// 			{
// 				uint32_t pnum = DeQueu_Pop(Running_Queue);		// Are we in a nested program?

// 				if (pnum)
// 				{
// 					Blob_State.prog_num = pnum;
// 					uint32_t repeat = DeQueu_Pop(Running_Queue);				// Save curent state.
// 					Blob_State.repeat = repeat;
// 				}
// 				else			// Not nested, check for next program in queue.
// 				{
// 					uint32_t pnum = DeQueu_Pop(Program_Queue);

// 					if (pnum) { start_program(pnum); }
// 					else { Blob_State.State = STATE_IDLE; }
// 				}
// 			}
// 		}
// 		else
// 		{
// 			if (Blob_State.Prog->repeat == 0)   // Infinate repeat?
// 			{
// 				uint32_t pnum = DeQueu_Pop(Program_Queue);

// 				if (pnum)		// Program in the queue?
// 				{
// 					start_program(pnum);
// 				}
// 			}

// 			D(printf("Program_Step_Done: Program repeat %d now %d.\n\n", Blob_State.Prog->repeat, Blob_State.repeat);)
// 			if (Blob_State.Prog->sequence)
// 			{
// 				end = start_sequence(Blob_State.Prog->sequence);
// 				continue;
// 			}
// 		}
// 		break;
// 	}
// 	return end;
// }

		// 		SEQ* seq = Blob_State.Seq;			// Get sequence record.
		// 		if (!seq->scene) { Program_Step_Done(); }    // End of program?
		// 		else
		// 		{
		// 			if (seq->xiti) { start_transition(seq->xiti, seq->scene); }		// Has transition?
		// 			else  // or instant transition
		// 			{
		// 				D(printf("next_blob_step: Set Scene(%d).\n", seq->scene);)
		// 				if (set_scene(seq->scene))
		// 				{
		// 					D(printf("next_blob_step: Start next sequence(%d).\n", Blob_State.sequence_num + 1);)
		// 					start_sequence(Blob_State.sequence_num + 1);
		// 				}
		// 				else { All_Program_Stop("\nBlob_Tick: Bad Scene (%d)!\n\n", seq->scene); }
		// 			}
		// 		}
// 	if (n && n <= Blob.Num_Prog)		// Valid program?
// 	{
// 		PROG* prog = &Blob.Prog_Base[n-1];		// Get program record.

// 		Blob_State.prog_num = n;		// Set as current running program.
// 		Blob_State.Prog = prog;
		
// 		D(printf("start_program(%d): repeat %d, seq %d\n", 
// 						 		n, prog->repeat, prog->sequence);)

// 		int32_t seq = prog->sequence;		// Get first step of program.

// 		if (seq)
// 		{
// 			if (seq > 0)
// 			{
// 				Blob_State.repeat = prog->repeat;		// Set repeat count.
// 				start_sequence(seq);
// 			}
// 			else
// 			{
// 				DeQueu_Push(Running_Queue, Blob_State.repeat);				// Save curent state.
// 				DeQueu_Push(Running_Queue, Blob_State.prog_num);
// 				start_program(-seq);
// 			}
// 		}
// 		else
// 		{
// 			printf("start_program: seq is zero, prog step done.\n");
// 			Program_Step_Done();
// 		}
// 	}
// 	else 
// 	{
// 		leds_all_black();
// 		All_Program_Stop("start_program: Bad (%d), Done. *********************************\n\n", n);
// 	}
