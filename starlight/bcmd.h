// File: bcmd.h

#ifndef BCMD_H
#define BCMD_H

#include "beng.h"

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
    
    COMMAND_TRIGGER   = 13,   // trig (engm) (trig)       -- interrupt current routine.
    COMMAND_INTERRUPT = 14,   // intr (engm) (trig)       -- interrupt current routine.
    COMMAND_QUEUE     = 15,   // queu (engm) (trig)       -- add routine to queue.

    COMMAND_SET       = 16,   // SET <var> <val>          -- Set a variable to a value.
    COMMAND_GET       = 17,   // GET <var>			      -- Get value (into accumulator)
    COMMAND_MOV       = 18,   // MOV <var> <var>          -- Move variable value to another variable.

    COMMAND_ADD       = 19,   // ADD <val>	              -- Add a value to accumulator.
    COMMAND_SUB       = 20,   // SUB <val>	              -- Subtract a value from accumulator.
    COMMAND_MUL       = 21,   // MUL <val>	              -- Multiply accumulator by a value.
    COMMAND_DIV       = 22,   // DIV <val>                -- Divide accumulator by a value.
    COMMAND_MOD       = 23,   // MOD <val>                -- Divide a value to accumulator.

    COMMAND_INC       = 24,   // INC>                     -- Increment accumulator (add one).
    COMMAND_DEC       = 25,   // DEC                      -- Decrement accumulator (subtract one).

    COMMAND_AND       = 26,   // AND <val>                -- Bitwise AND a value with accumulator.
    COMMAND_OR        = 27,   // OR <val>                 -- Bitwise OR a value with accumulator.
    COMMAND_XOR       = 28,   // XOR <val>                -- Bitwise XOR a value with accumulator.
    COMMAND_NOT       = 29,   // NOT                      -- Bitwise complement value in accumulator.

    COMMAND_PUSH      = 30,   // PUSH <var>               -- Push variable onto stack (if var is omitted then accumulator is used)
    COMMAND_POP       = 31,   // POP <var>                -- Push stack into variable (if var is omitted then accumulator is used)

    COMMAND_TST       = 32,   // TST <val>                -- Subtract a value from accumulator (set flags and discard result).
    COMMAND_BEQ       = 33,   // BEQ (cp)                 -- Branch if accumulator is zero.
    COMMAND_BNE       = 34,   // BNE (cp)                 -- Branch if accumulator is not zero.
    COMMAND_BRA       = 35,   // BRA (cp)                 -- Branch always.

    // COMMAND_TRIGGER   = 15,   // trig (n) [routine] (n)   -- bind a trigger to a routine.
    // COMMAND_PHYS      = 16,   // phys (s) (n)             -- set number of leds for specific string.

    COMMAND_LAST              // This must always be last!

} COMMAND;


#define COMMAND_MASK (0xFFFF)

#define COMMAND_ARG1_IS_VARIABLE (1 << 29)
#define COMMAND_ARG2_IS_VARIABLE (1 << 30)


extern const char* Command_Name(COMMAND /*cmd*/);

extern bool Process_Command(BENG_STATE* /*bs*/);

#endif    // BCMD_H

// Endfile: bcmd.h
