// File: beng.h

#ifndef BENG_H
#define BENG_H

#include "blob.h"
#include "bvar.h"
#include "stack.h"
#include "shifter.h"

#define BAD_BENG_IDX (-1)

typedef enum
{
	STATE_IDLE,             // Nothing running.
	STATE_WAITING,          // In wait.
	STATE_COMMAND,          // Running commands.
	STATE_TRANSITION,       // In transition.

} STATE;


#define CONTEXT_ITEMS 5         // Number of uint32_t that are saved.
#define CONTEXT_DEPTH 5         // Maximum number of saves.

#define PROG_STACK_SIZE (CONTEXT_ITEMS * CONTEXT_DEPTH)      // Maximum number of pending programs.

#define MAX_BENG 7
#define ENGINE_VALID(beng) ((beng) >= 0 && (beng) <= MAX_BENG)

typedef struct Prog_Stack
{
    STACK stk;
    uint32_t buff[PROG_STACK_SIZE];

} PROG_STACK;


typedef struct Beng_State
{
    struct repeating_timer blob_timer_prog_tick;		// Timer to call Blob_Tick.
    bool tick_is_running;

	int Tick_Count;             // Used by state machine tick.
    int Tick_Speed;             // Clock for blob player.    

    STATE State;				// Start in IDLE state.
   // bool is_idle;               // True if in idle state.

	uint32_t wait_counter;		// Ticks left to wait until next step.

	uint32_t repeat;			// Number of times left to repeat.
	PROG* repeat_start;			// First command in program sequence.
	PROG* prog;				    // Points to next program command.

	// XITI* Xiti;				// Transition currently playing.
//	uint32_t* Xiti_Times;		// Store for transition step time counts.

	PROG_STACK program_stack_buffer;
    STACK* program_stack;	// Pending program stack. 
    
    LED led_rotate_buff[MAX_LED_ROTATE];

    int phy_mask;               // Current phy mask for led writes.

    BENG_VAR local_vars[MAX_BENG_LOCAL + 1];   // Accumulator is at idx 0.

} BENG_STATE;

extern volatile uint64_t Blob_Time;

//extern BENG_STATE Beng_State;   // Do deprecate... eventually.

//extern const int Tick_Speed;                    // Clock for blob player.

extern void Beng_Init(void);                    // Prepare BLOB engines for use.  Call once at startup.

extern void Beng_All_Stop(void);                // Stop all engines and clear all queues.

extern BENG_STATE* Get_Beng_State(int beng_idx);

extern BENG_VAR* BVar_Find_Local(BENG_STATE* /*bs*/, uint32_t /*idx*/);

extern void Beng_Set_Phy_mask(int beng_mask, int phy_mask);

extern void Push_Context(BENG_STATE* /*bs*/);   // Save state context.
extern void Pop_Context(BENG_STATE* /*bs*/);    // Recover saved state context.

extern void Blob_Run(int beng_idx, PROG_ID pnum);
extern void Blob_Run_mask(int beng_mask, PROG_ID pnum);

extern void Blob_Trigger(int /*beng_idx*/, TRIG_ID /*trig_id*/);    // Immediately Start playing a BLOB program.  (Cancel any running or queue'd).
extern void Blob_Trigger_mask(int /*beng_mask*/, TRIG_ID /*trig_id*/);

//extern void Blob_Queue_Next(int /*bsn*/, TRIG_ID /*n*/);         // Start playing a BLOB program (after current completes).

#endif  // BENG_H

// Endfile: beng.h
