// File: beng.c    -- Blob engine.

#include "common.h"
#include "beng.h"

#include <stdlib.h>
#include <string.h>

#include "bcmd.h"
#include "debug.h"

//#define TRACE_BENG_TICK

#ifdef TRACE_BENG_TICK
#include "trace.h"
#endif

#define TICK_TIME 1000  // us or 1ms.    Blob_Time units.

PUBLIC volatile uint32_t Engine_Idx = 1;

PUBLIC volatile uint64_t Blob_Time = 0;

PUBLIC BENG_STATE Beng_State[MAX_BENG] = {0};     // One of these for each engine!


PUBLIC BENG_STATE* Get_Beng_State(int beng_idx)
{
    return (ENGINE_VALID(beng_idx)) ? &Beng_State[beng_idx] : NIL;
}


PRIVATE int Get_Beng_State_Idx(BENG_STATE* bs)
{
    int beng_idx = bs - Beng_State;

    return (ENGINE_VALID(beng_idx)) ? beng_idx : BAD_BENG_IDX;
}


PUBLIC BENG_VAR* BVar_Find_Local_By_Index(BENG_STATE* bs, uint32_t idx)
{
    idx &= BENG_VAR_IDX_MASK;
    return (bs && idx < MAX_BENG_LOCAL) ? bs->local_vars + idx : NIL;
}


PUBLIC void Beng_Set_Phy_mask(int beng_mask, int phy_mask)
{
    uint32_t mask = 1;
    int beng_idx = 0;

    while (beng_mask && beng_idx < MAX_BENG)
    {
        if (beng_mask & mask)
        {
            BENG_STATE* bs = Get_Beng_State(beng_idx);

            if (bs) { bs->phy_idx = phy_mask; }

            beng_mask &= ~mask;
        }
        ++beng_idx;   mask <<= 1;
    }
}


PUBLIC void Push_Context(BENG_STATE* bs)
//
// Saves current Blob_State.
{
	STACK* stk = bs->program_stack;

	Stack_Push(stk,           bs->State);		    // Current state (processing command).
	Stack_Push(stk, (uint32_t)bs->phy_idx);		// LED Phy mask.
	Stack_Push(stk, (uint32_t)bs->prog);			// Next command to run.
	Stack_Push(stk,           bs->pause_counter);	// Ticks left to wait until next step.
	Stack_Push(stk,           bs->repeat);		    // Number of times left to repeat.
	Stack_Push(stk, (uint32_t)bs->repeat_start);	// First command in program sequence.
	// XITI* Xiti;									// Transition currently playing.
//	Stack_Push(q, (uint32_t)bs->Xiti_Times);		// Store for transition step time counts.

	bs->repeat = 0;
//	bs->Xiti_Times = 0;
}


PUBLIC void Pop_Context(BENG_STATE* bs)
//
// Restores Blob_State.
{
	STACK* stk = bs->program_stack;

//	Blob_State.Xiti_Times   = (uint32_t*)Stack_Pop(q);		// Store for transition step time counts.
	// XITI* Xiti;											// Transition currently playing.
	bs->repeat_start = (PROG*)Stack_Pop(stk);		    	// First command in program sequence.
	bs->repeat       =        Stack_Pop(stk);			    // Number of times left to repeat.
	bs->pause_counter =        Stack_Pop(stk);			    // Ticks left to wait until next step.
	bs->prog         = (PROG*)Stack_Pop(stk);			    // Next command to run.
	bs->phy_idx     =        Stack_Pop(stk);			    // LED Phy mask.
	bs->State        =        Stack_Pop(stk);			    // Current state (processing command).
}



#define DEBUG_BLOB2 (DEBUG_BLOB | DEBUG_BUSY)

//----------------------------------------------------------------------------------------------------------
PRIVATE int64_t Blob_Program_Tick(alarm_id_t id, void *user_data)
//
// Tick for player state machine.    Called Tick_Speed times per second. 
{
    BENG_STATE* bs = (BENG_STATE*)user_data;

#ifdef TRACE_BENG_TICK
    Trace_Start();
#endif

//	D(DEBUG_BLOB2, if (bs->State)  { PRINTF("\n== Blob_Tick %d: State %d\n", ++bs->Tick_Count, bs->State); })

	switch (bs->State)
	{
		case STATE_IDLE: 
        {
            bs->tick_is_running = false;
            break; 
        }
		case STATE_PAUSED:
		{
			if (bs->pause_counter) { --bs->pause_counter; }

			if (bs->pause_counter)	// Waiting done?
			{
				break;  // Nope, still waiting.
			}
			else
			{
				bs->State = STATE_COMMAND;
				// Fall throught to next case ... STATE_COMMAND
			}
		}
		case STATE_WAITING:
        {
            if (time_reached(bs->wait_time))
            {
				bs->State = STATE_COMMAND;
				// Fall throught to next case ... STATE_COMMAND
            }
            else { break; }   // Still waiting.
        }
		case STATE_COMMAND:  // must be right after STATE_WAITING.
		{
            bool cmd_is_running = Process_Command(bs);

			if ( !cmd_is_running )
			{ 							// Command sequence ended.
				D(DEBUG_BLOB, PRINTF("-- Sequence End.\n\n");)

				if (bs->repeat)  // Repeating?
				{
					if (--bs->repeat)
					{
						bs->prog = bs->repeat_start;
						bs->State = STATE_COMMAND;
					}
					else  // Done repeat.
					{
						Pop_Context(bs);
                        D(DEBUG_BLOB, PRINTF("Done repeat. next %X\n", bs->prog);)
					}
				}
				else if (bs->program_stack->count)		// Anything on stack?
				{
					Pop_Context(bs);
                    D(DEBUG_BLOB, PRINTF("next command %X\n", bs->prog);)
				}
				else
				{
                    D(DEBUG_BLOB, PRINTF("To Idle\n");)
					bs->State = STATE_IDLE; 
				}
			}
			break;
		}
		case STATE_TRANSITION:
		{
			D(DEBUG_BLOB, PRINTF("Transition State:\n");)
			bs->State = STATE_COMMAND;
//			transition_step();
			break;
		}
	}
	//D(DEBUG_BLOB2, if (bs->State)  { PRINTF("== Blob_Tick: Done (%d).\n\n", bs->Tick_Count); })

#ifdef TRACE_BENG_TICK
    Trace_End();
#endif

	return (bs->tick_is_running) ? -bs->Tick_Speed : 0;
}


//--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
PRIVATE void start_prog_tick(BENG_STATE* bs)
{
    if (!bs->tick_is_running)
    {
        bs->tick_is_running = true;
        add_alarm_in_us(bs->Tick_Speed, Blob_Program_Tick, bs, true);
        PRINTF("start_prog_tick\n");
    }
}


PUBLIC void Blob_Stop(BENG_STATE* bs)
//
// Stop current program and clear stack.
{
	bs->State = STATE_IDLE;
    while (bs->tick_is_running)  { sleep_us(100); } // Wait for stop
    Stack_Clear(bs->program_stack); 
	bs->repeat = 0;
	bs->pause_counter = 0;
    bs->prog = 0;
}


PUBLIC void Beng_All_Stop(void)
//
// Stop ALL Blob engines.
{
    int beng_idx = MAX_BENG;
    BENG_STATE* bs = Beng_State;

    PRINTF("Beng_All_Stop\n");

    while (beng_idx--) { Blob_Stop(bs++); }
}


PRIVATE void start_program(BENG_STATE* bs, PROG_ID n)
//
// Start playing a program.
{
	if (n == 0) 
    {
        bs->State = STATE_IDLE; 
        bs->prog = 0;
    }
	else
	{
		D(DEBUG_BLOB, PRINTF("start_program: n %d\n", n);)
		bs->prog = Prog_Ptr(n);
		bs->State = STATE_COMMAND;
        start_prog_tick(bs);
	}
}

PUBLIC void Blob_Run(int beng_idx, PROG_ID pnum)
{
	if (pnum)
	{
        BENG_STATE* bs = Get_Beng_State(beng_idx);

        Blob_Stop(bs);

		D(DEBUG_BLOB, PRINTF("Blob_Run: n %d, program: %d\n", pnum, pnum);)
		start_program(bs, pnum);
	}	
}


PUBLIC void Blob_Run_mask(int beng_mask, PROG_ID pnum)
{
    uint32_t mask = 1;
    int beng_idx = 0;

    while (beng_mask && beng_idx < MAX_BENG)
    {
        if (beng_mask & mask)
        {
            Blob_Run(beng_idx, pnum);
            beng_mask &= ~mask;
        }
        ++beng_idx;   mask <<= 1;
    }
}


PUBLIC void Blob_Trigger(int beng_idx, TRIG_ID trig_id)
//
// Immediately Start playing a BLOB program.  (Cancel any running or queue'd)
{
	PROG_ID pnum = Get_Trigger_Prog(trig_id);
    Blob_Run(beng_idx, pnum);
}


PUBLIC void Blob_Trigger_mask(int beng_mask, TRIG_ID trig_id)
{
    uint32_t mask = 1;
    int beng_idx = 0;

    if (!beng_mask) { beng_mask = Engine_Idx; }

    while (beng_mask && beng_idx < MAX_BENG)
    {
        if (beng_mask & mask)
        {
            Blob_Trigger(beng_idx, trig_id);
            beng_mask &= ~mask;
        }
        ++beng_idx;   mask <<= 1;
    }
}




// PUBLIC void Blob_Queue_Next(int bsn, TRIG_ID n
// //
// // Start playing a BLOB program (after current completes).
// {
// 	if (Blob_State.State == STATE_IDLE)		// Nothing running right now?
// 	{
// 		Blob_Trigger(bsn, n);					// Same as trig.
// 	}
// 	else if (n && n <= Blob.Num_Trig)
// 	{
// 		D(DEBUG_BLOB, PRINTF("Blob_Queue_Next: n %d\n", n);)
// 		Blob_Stop();

// 		PROG_ID pnum = Blob.Trigger_Base[n-1];
// //		Queue_Add(Blob_State.program_queue, pnum);		// Add to back of queue.
// 	}
// }


PRIVATE bool Blob_Time_Tick(struct repeating_timer* ptr)
{
	++Blob_Time;   // uint64_t
}


PRIVATE void Beng_State_Init(BENG_STATE* bs)
{
    bs->Tick_Speed = 50000;
    bs->program_stack = Stack_Initialize(&bs->program_stack_buffer, PROG_STACK_SIZE);
    bs->phy_idx = 0xFFFF;
}


PUBLIC void Beng_Init(void)
//
// Prepare BLOB engine(s) for use.  Call once at startup.
{
	static struct repeating_timer blob_timer_1;		// Timer to call Blob_Time_Tick.
	add_repeating_timer_us(TICK_TIME, Blob_Time_Tick, NULL, &blob_timer_1);

    int beng_idx = MAX_BENG;
    BENG_STATE* bs = Beng_State;

    while (beng_idx--) { Beng_State_Init(bs++); }
}


// EndFile: beng.c
