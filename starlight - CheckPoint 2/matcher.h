// File: Matcher.h

typedef enum match_code
{
    MATCH_NONE,    // Must be first.

    MATCH_RESET,        // Stop all engines and set to no blob loaded.
    MATCH_BLACK,        // All leds black (stop running blob and clears queue)

// - Variables.

    MATCH_GETV,         // Get variable.
    MATCH_SETV,         // Set variable.
    MATCH_SETQ,         // Set variable (quiet).

// - Blobs.

    MATCH_LOAD_BLOB,     // Loads a new blob.  Start running.
    MATCH_STAGE_BLOB,    // Loads a new blob.  Waits for activate to start running.
    MATCH_ACTIVATE_BLOB, // Start running new blob.
    MATCH_EXPORT_BLOB,   // Export current blob.

    MATCH_LOAD,         // Load flash blob to RAM.
    MATCH_LOAD_WAIT,    // Load flash blob to RAM.  Wait for activate to start running.
    MATCH_SAVE,         // Save RAM blob to flash.
    MATCH_ERASE,        // Erase flash page.

// Scenes.

    MATCH_SCENE,        // Set leds to scene from terminal.
    MATCH_SHOW,         // Set leds to scene from loaded blob.
    MATCH_UPDATE,       // Update leds that need updaing.

// Events.

    MATCH_TRIGGER,      // Trigger a program to start running immediately.
    MATCH_INTERRUPT,    // Halt running program, run new program, then resume halted program.
    MATCH_QUEUE,        // Queue a program to start running after current program completes.

// Instrumentation.

    MATCH_DUMP,         // Dump debug info.

// Dregs.

    MATCH_SPHY,         // Set the current led array to use.
//    MATCH_BRIGHTNESS,   // Set led brighness (0.0 -> 1.0)
//    MATCH_SENG,         // Set the current engine to use.
//    MATCH_DEBUG,        // Set debug mask.

    MATCH_LAST     // Must be last.

} MATCH_CODE;

extern void Matchers_Reset();
#define Matchers_Init Matchers_Reset
extern MATCH_CODE Is_Match(uint8_t ch);


// Endfile: Matcher.h
