// File: Morph.c

#include "Common.h"





#ifdef COMMENT

// #include <stdio.h>
#include <stdlib.h>
// #include <string.h>
// #include <time.h>

#include "Blob.h"
#include "Led.h"

#include "Morph.h"


typedef struct Morph_Step
{
	int wait;		// How long to wait between steps.
	int count;		// Current wait count.
	int step;		// Size of each step.

} MORPH_STEP;

typedef struct Morph
{
	FLOAT trans_time;	// Time for the whole transition (seconds).
	int trans_counter;  // Number of ticks left in transistion.

	int starting_value;	// Inital value.
	int ending_value;	// Final value.

} MORPH;


PRIVATE MORPH morph = {0};
PRIVATE MORPH_STEP morph_step_data[xx] = {0};

#define myabs(x) (((x) < 0.0) ? (-(x)) : (x))


PRIVATE int ticks(FLOAT val)
//
// Convert time in seconds into tick count.
{
	FLOAT aval = abs(val * Tick_Speed);
	int rval = (int)(aval + 0.5); 

	return (int)((myabs(val) * Tick_Speed) + 0.5);
}


PRIVATE void calc_morph_step(MORPH* morph, MORPH_STEP* step)
{
    //# print(f"dif: trans = {trans}, starting_value = {starting_value}, ending_value = {ending_value}")

    int diff = morph->ending_value - morph->starting_value;

    if (!diff)
	{
		step->count = step->wait = 0;
		step->step = 0;
	}
	else
	{
		int isneg = diff < 0 ? -1 : 1;
//		diff += isneg;
		FLOAT step_time = morph->trans_time / diff;
		int ms = ticks(step_time);

		//# print(f"\ndif({trans}, {starting_value}, {ending_value}) : diff = {diff}, step_time = {step_time}, ms = {ms}")

		if (ms)
		{
			step->count = step->wait = ms;
			step->step = isneg;
		}
		else
		{
			for (int i = 2; i < 255; ++i)
			{
				FLOAT tt = step_time * i;

				if (tt)
				{
					ms = ticks(tt);
					step->count = step->wait = ms;
					step->step = i * isneg;
					return;
				}
			}
			step->count = step->wait = 1;
			step->step = diff;
		}
	}
}


PUBLIC void morph_start(FLOAT trans_time, LED* to_scene)
{
	// MORPH_STEP* step = morph_step_data;
	// uint8_t* from_scene = LED_Data;

	// morph.trans_time = trans_time;
	// morph.trans_counter = ticks(trans_time);

	// for (int i = 0; i < LED_DATA_SIZE; ++i)
	// {
	// 	morph.starting_value = *from_scene++;
	// 	morph.ending_value = *to_scene++;
	// 	calc_morph_step(&morph, step++);
	// }
}


PUBLIC bool morph_step(void)
//
// Return true when morph is complete.
{
	bool do_send = false;

	if (morph.trans_counter && --morph.trans_counter)
	{
		// MORPH_STEP* step = morph_step_data;
		// uint8_t* data = LED_Data;

		// for (int i = 0; i < LED_DATA_SIZE; ++i)
		// {
		// 	if (step->count == 0 || --step->count == 0)
		// 	{
		// 		do_send = true;
		// 		*data += step->step;
		// 		step->count = step->wait;
		// 	}
		// 	++step; ++data;
		// }
	}

	if (do_send)
	{
		//LED_Update(0);
	}

	return (morph.trans_counter == 0);
}


// EndFile: Morph.c





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

// 			D(PRINTF("transition step: ttime %d\n", Blob_State.trans_time);)

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
// 						D(PRINTF("add(%d): before %d, step %d", i, *ledp, step_size);)
// 						*xtime = xitip->step_time;  // Reset the counter.
// 						step_size = (step_size > 0) ? MIN(step_size, 255 - *ledp) : MIN(step_size, *ledp);
// 						*ledp += step_size;
// 						D(PRINTF(", after %d, step %d\n", *ledp, step_size);)
// 						do_send = true;
// 					}
// 				}
// 			}
// 			if (do_send) { led_send(); }
// 		}
// 	}
// 	if (!in_transition) 
// 	{
// 		D(PRINTF("transition_step: Start next sequence(%d).\n", Blob_State.sequence_num + 1);)
// 		start_sequence(Blob_State.sequence_num + 1);
		
// 		// PRINTF("not transition: Step_Done\n");
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

// 		// PRINTF("start_transition: set times (");
// 		if (xitip && xtimep)
// 		{
// 			for (int i = 0; i < LED_DATA_SIZE; ++i)		// Copy initial transistion time values.
// 			{
// 				// PRINTF("t(%d)=%d, ", i, xitip->step_time);
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

#endif