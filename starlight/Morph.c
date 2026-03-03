// File: Morph.c

#include "Common.h"
#include "Morph.h"

#include "beng.h"
#include "scene.h"

#define ABS(x) ((x < 0) ? -x : x)

// typedef struct morph_step
// {
//     int8_t white;
//     int8_t blue;
//     int8_t red;
//     int8_t green;

// } MORPH_STEP;

// typedef struct morph_count
// {
//     uint8_t white;
//     uint8_t blue;
//     uint8_t red;
//     uint8_t green;

// } MORPH_COUNT;

// typedef struct led_morph_single
// {
//     MORPH_STEP  steps;
//     MORPH_COUNT counts;
//     MORPH_COUNT periods;

// } LED_MORPH_SINGLE;

// typedef struct led_morph
// {
//     size_t num_leds;
//     LED* dests;
//     LED_MORPH_SINGLE* morphs;

// } LED_MORPH;



PRIVATE int ticks(FLOAT val, FLOAT tick_time)
//
// Convert time in seconds into tick count.
{
	FLOAT aval = ABS(val) / tick_time;
	int rval = (int)(aval + 0.5); 

	return (int)((ABS(val) / tick_time) + 0.5);
}


PRIVATE void calc_morph_step(FLOAT trans_time, FLOAT tick_time, LED_VAL starting, LED_VAL ending, int8_t* step, uint8_t* count, uint8_t* period)
{
    int diff = ending - starting;

    if (!diff)
	{
		*count = *period = 0;
		*step = 0;
	}
	else
	{
		int isneg = diff < 0 ? -1 : 1;
		FLOAT step_time = trans_time / diff;
		int ms = ticks(step_time, tick_time);

		if (ms)
		{
			*count = *period = ms;
			*step = isneg;
		}
		else
		{
			for (int i = 2; i < 255; ++i)
			{
				FLOAT tt = step_time * i;

				if (tt > 1.0)
				{
					ms = ticks(tt, tick_time);
					*count = *period = ms;
					*step = i * isneg;
					return;
				}
			}
			*count = *period = 1;
			*step = diff;
		}
	}
}


PUBLIC void Morph_Start(LED* leds, LED_MORPH* ledmorph, int morph_time, int tick_speed, SCENE_ID scene_id)
{
    tick_speed /= 1000;   // Convert tick speed into milliseconds.
    FLOAT mt = (FLOAT)morph_time / 1000.0;
    FLOAT tick_time = tick_speed / 1000.0;

    PRINTF("Morph_Start:\n");

    LED* dest = ledmorph->dests;
    LED_MORPH_SINGLE* sing = ledmorph->morphs;

    Render_Scene_Id(dest, ledmorph->num_leds, scene_id);

    for (int i = 0; i < ledmorph->num_leds; ++i)
    {
        calc_morph_step(mt, tick_time, leds->led.red,   dest->led.red,   &sing->steps.red,   &sing->counts.red,   &sing->periods.red);
        calc_morph_step(mt, tick_time, leds->led.blue,  dest->led.blue,  &sing->steps.blue,  &sing->counts.blue,  &sing->periods.blue);
        calc_morph_step(mt, tick_time, leds->led.green, dest->led.green, &sing->steps.green, &sing->counts.green, &sing->periods.green);

        leds++; dest++; sing++;
    }
}


// PRIVATE void dump_led(LED* led, char*str)
// {
//     PRINTF("%s r(%d), g(%d), b(%d)\n", str, led->led.red, led->led.green, led->led.blue);
// }


// PRIVATE void dump_morph(LED_MORPH_SINGLE* morph)
// {
//     PRINTF("\tred. count %u, period %u, step %d\n", morph->counts.red, morph->periods.red, morph->steps.red);
//     PRINTF("\tgreen. count %u, period %u, step %d\n", morph->counts.green, morph->periods.green, morph->steps.green);
//     PRINTF("\tred. count %u, period %u, step %d\n", morph->counts.blue, morph->periods.blue, morph->steps.blue);
// }


#define MORPH_STEP(color) \
        if (led->led.color != dest_led->led.color)\
        {\
            got_one = true;\
            if (msing->counts.color)\
            {\
              if (--msing->counts.color == 0)\
              {\
                  led->led.color += msing->steps.color;\
                  msing->counts.color = msing->periods.color;\
                  LED_Needs_Update(1 << (led->led.phy_num - 1));\
              }\
            }\
        }
// MORPH_STEP should check for overshoot!


PUBLIC bool Morph_Step(void* bsv)
{
    BENG_STATE* bs = bsv;
    bool got_one = false;
    LED_MORPH* morph = &bs->morph;
    LEDS_PHY* phy = LED_Get_Phy(bs->phy_idx);
  
    LED* led = phy->led_data;
    LED* dest_led = morph->dests;  
    LED_MORPH_SINGLE* msing = morph->morphs;
    int n = morph->num_leds;
    bool first = true;

    while (n--)
    {
        int r = led->led.red;
        MORPH_STEP(red);
        int b = led->led.blue;
        MORPH_STEP(blue);
        int g = led->led.green;
        MORPH_STEP(green);

        if (/*n > 30 &&*/ ((r != led->led.red) || (g != led->led.green) || (b != led->led.blue)))
        {
            if (first)
            {
                PRINTF("\nMorph Step(%d):\n", n);    
                first = false;
            }
            PRINTF("%d]  r (%d)->%d, g (%d)->%d, b (%d)->%d\n", n, r, led->led.red, g, led->led.green, b, led->led.blue);
        }

        // dump_led(led, "dump_led: 'start' ");
        // dump_led(dest_led, "\tdest");
        // dump_morph(msing);

        ++led; ++dest_led; ++msing;
    }

    if (!first) { PRINTF("\n"); }

//    if (got_one) { LEDS_Do_Update(); }

    return got_one;
}


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