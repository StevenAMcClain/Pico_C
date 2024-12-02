// File: Morph.c

#include "Common.h"

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
PRIVATE MORPH_STEP morph_step_data[MAX_LED_DATA_SIZE] = {0};

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
		LED_Update();
	}

	return (morph.trans_counter == 0);
}


// EndFile: Morph.c
