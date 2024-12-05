// File: Matcher.h

typedef enum match_code
{
    MATCH_NONE,    // Must be first.

    MATCH_PHYS,         // Set configuration (num_leds)
    MATCH_LARR,         // Set the current led array to use.
    MATCH_SHOW,         // Set leds to scene from loaded blob.
    MATCH_SCENE,        // Set leds to scene.
    MATCH_BLACK,        // All leds black (stop running blob and clears queue)
    MATCH_BLOB,         // Loads a new blob.
    MATCH_TRIGGER,      // Trigger a program to start running immediately.
    MATCH_QUEUE,        // Queue a program to start running after current program completes.
    MATCH_BRIGHTNESS,   // Set led brighness (0.0 -> 1.0)
    MATCH_INTERRUPT,    // Halt running program, run new program, then resume halted program.

    MATCH_LAST     // Must be last.

} MATCH_CODE;

extern void Matchers_Reset();
extern void Matchers_Init();
extern MATCH_CODE Is_Match(uint8_t ch);


// Endfile: Matcher.h
