// File: bcmd.c

#include "common.h"
#include "bcmd.h"

#include "beng.h"
#include "debug.h"
#include "led.h"
#include "scene.h"
#include "shifter.h"


PUBLIC const char* Command_Name(COMMAND cmd)
//
// Returns string given command code.
{
    const char* command_name[] = {
            "end",  // COMMAND_END      -- end of program, stop running.
            "upda", // COMMAND_UPDATE   -- update all leds that need updaing.
            "yiel", // COMMAND_YIELD    -- wait until next tick.
            "wait", // COMMAND_WAIT     -- wait for (n) milli-seconds.
            "paus", // COMMAND_PAUS     -- pause for (n) engine ticks.

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

            "set",  // COMMAND_SET      -- Set a variable to a value.
            "get",  // COMMAND_GET      -- Get value (into accumulator)
            "mov",  // COMMAND_MOV      -- Move variable value to another variable.

            "add",  // COMMAND_ADD      -- Add a value to accumulator.
            "sub",  // COMMAND_SUB      -- Subtract a value from accumulator.
            "mul",  // COMMAND_MUL      -- Multiply accumulator by a value.
            "div",  // COMMAND_DIV      -- Divide accumulator by a value.
            "mod",  // COMMAND_MOD      -- Divide a value to accumulator.

            "inc",  // COMMAND_INC      -- Increment accumulator (add one).
            "dec",  // COMMAND_DEC      -- Decrement accumulator (subtract one).

            "and",  // COMMAND_AND      -- Bitwise AND a value with accumulator.
            "or",   // COMMAND_OR       -- Bitwise OR a value with accumulator.
            "xor",  // COMMAND_XOR      -- Bitwise XOR a value with accumulator.
            "not",  // COMMAND_NOT      -- Bitwise complement value in accumulator.

            "shl",  // COMMAND_SHL      -- Bitwise shift value in accumulator left.
            "shr",  // COMMAND_SHR      -- Bitwise shift value in accumulator right.
            "rol",  // COMMAND_ROL      -- Bitwise rotate value in accumulator left.
            "ror",  // COMMAND_ROR      -- Bitwise rotate value in accumulator right.

            "push", // COMMAND_PUSH     -- Push variable onto stack (if var is omitted then accumulator is used)
            "pop",  // COMMAND_POP      -- Push stack into variable (if var is omitted then accumulator is used)

            "tst",  // COMMAND_TST      -- Subtract a value from accumulator (set flags and discard result).
            "beq",  // COMMAND_BEQ      -- Branch if accumulator is zero.
            "bne",  // COMMAND_BNE      -- Branch if accumulator is not zero.
            "bra",  // COMMAND_BRA      -- Branch always.

            "trig", // COMMAND_TRIGGER  -- bind a trigger to a routine.
            "phys", // COMMAND_PHYS     -- set number of leds for specific string.
        };

    cmd &= COMMAND_MASK;

    if (cmd >= COMMAND_END && cmd < COMMAND_LAST)
        return command_name[cmd];

    return "";
}


#define DEBUG_BLOB2 (DEBUG_BLOB | DEBUG_BUSY)

PUBLIC bool Process_Command(BENG_STATE* bs)
//
// Process the commands starting at cmdp.
// Return: false if command is completed.
{
    PROG** cmdpp = &bs->prog;
    PROG* cmdp = *cmdpp;
	bool result = true;

	if (cmdp)
	{
    	bool done = false;

		while (result && !done)
		{
			PROG base = *(PROG*)cmdp++;

            COMMAND cmd = (COMMAND)base & COMMAND_MASK;

            BENG_VAR* var = (base & COMMAND_ARG1_IS_VARIABLE) ? BVar_Find(bs, (uint32_t)*cmdp) : NIL;

			D(DEBUG_BLOB2, PRINTF("PC [%d]: %d '%s'\n", Prog_Id(cmdp), cmd, Command_Name(cmd));)

			switch (cmd)
			{
				case COMMAND_END:      // end of program, stop running.
				{
					bs->State = STATE_IDLE;
					cmdp = 0;
					result = false;
					break;
				}
                case COMMAND_UPDATE:    // Update LED Strings.
                {
                    LEDS_Do_Update();
					done = true;        // Always yield after update.
                    break;
                }
				case COMMAND_YIELD:     // wait until next tick.
				{
					done = true;
					break;
				}
				case COMMAND_PAUS:     // pause for (n) engine ticks.
				case COMMAND_WAIT:     // wait for (n) milli-seconds.
                {
                    uint32_t ms = var ? BVar_Get_int(var) : (uint32_t)*cmdp;
                    ++cmdp;

                    if (cmd == COMMAND_PAUS)
                    {
                        bs->pause_counter = ms;
                        bs->State = STATE_PAUSED;
                    }
                    else
                    {
                        bs->wait_time = make_timeout_time_ms(ms);
                        bs->State = STATE_WAITING;
                    }
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

                    D(DEBUG_BLOB, PRINTF("Call: pgm 0x%X\n", arg);)

					bs->prog = cmdp;   // Make sure State.prog is up to date.
					Push_Context(bs);  // Save for later. 

					PROG* prog = Prog_Ptr(arg);     // Find new command.
					cmdp = bs->prog = prog;         // Set new prog pointer.
					break;
				}
				case COMMAND_REPEAT:   // repeat a sequence (n) times.
				{
					uint32_t repeat = (uint32_t)*cmdp++;	// Number of times left to repeat.
					PROG cmd_idx = (PROG)*cmdp++;	        // First command in program sequence.

					bs->prog = cmdp;
					Push_Context(bs);

					D(DEBUG_BLOB, PRINTF("Repeat: %d, pgm %d\n", repeat, cmd_idx);)

					bs->repeat = repeat;
					cmdp = bs->repeat_start = bs->prog = Prog_Ptr(cmd_idx);;
					break;
				}
                case COMMAND_SPHY:    // select current phy string.
				{
					int32_t arg = (int32_t)*cmdp++;
                    bs->phy_mask = arg;
					break;
				}
				case COMMAND_SCENE:    // paint scene (n).
				{
					uint32_t arg = (uint32_t)*cmdp++;
					Set_Scene_idx(bs->phy_mask, arg);
                    LEDS_Do_Update();
					break;
				}
				case COMMAND_RENDER:    // paint scene (n), no update.
				{
					uint32_t arg = (uint32_t)*cmdp++;
					Set_Scene_idx(bs->phy_mask, arg);
					break;
				}
				case COMMAND_SHIFT:    // shift led color values (values that are shifted off the end are lost).
				{
                    LED led_rotate_buff[MAX_LED_ROTATE];
					int32_t shift = (int32_t)*cmdp++;			// Number of places to shift.
					Command_Shift_LEDS_mask(bs->phy_mask, true, shift, led_rotate_buff);
					break;
				}
				case COMMAND_ROTATE:   // rotate led value. (end wraps).
				{
                    LED led_rotate_buff[MAX_LED_ROTATE];
					int32_t shift = (int32_t)*cmdp++;			// Number of places to rotate.
					Command_Shift_LEDS_mask(bs->phy_mask, false, shift, led_rotate_buff);
					break;
				}
				case COMMAND_MORPH:    // morph current scene into new scene (n) over (t) seconds.
                {
					cmdp++;
                    break;
                }
				case COMMAND_TRIGGER:   // immediate trigger to selected engines.
				case COMMAND_QUEUE:     // add routine to queue.
				case COMMAND_INTERRUPT: // interrupt current routine.
					cmdp++;
				{
					D(DEBUG_BLOB, PRINTF("Command(%d) '%s' is not implemented yet.\n", cmd, Command_Name(cmd));)
					break;
				}


				case COMMAND_SET:   // Set a variable to a value.
				case COMMAND_GET:   // Get value (into accumulator)
				case COMMAND_MOV:   // Move variable value to another variable.

				case COMMAND_ADD:   // Add a value to accumulator.
				case COMMAND_SUB:   // Subtract a value from accumulator.
				case COMMAND_MUL:   // Multiply accumulator by a value.
				case COMMAND_DIV:   // Divide accumulator by a value.
				case COMMAND_MOD:   // Divide accumulator by a value and store remainder to accumulator.

                case COMMAND_INC:   // Increment accumulator (add one).
                case COMMAND_DEC:   // Decrement accumulator (subtract one).

                case COMMAND_AND:   // Bitwise AND a value with accumulator.
				case COMMAND_OR:    // Bitwise OR a value with accumulator.
				case COMMAND_XOR:   // Bitwise XOR a value with accumulator.
				case COMMAND_NOT:   // Bitwise complement value in accumulator.

				case COMMAND_PUSH:   // Push variable onto stack (if var is omitted then accumulator is used)
				case COMMAND_POP:    // Push stack into variable (if var is omitted then accumulator is used)

				case COMMAND_TST:    // Subtract a value from accumulator (set flags and discard result).
				case COMMAND_BEQ:    // Branch if accumulator is zero.
				case COMMAND_BNE:    // Branch if accumulator is not zero.
				{
					D(DEBUG_BLOB, PRINTF("Process_Command: Not implemented %d '%s'\n", cmd, Command_Name(cmd));)
					break;
				}
                case COMMAND_BRA:    // Branch always.
				{
					PROG arg = (PROG)*cmdp++;
					cmdp = Prog_Ptr(arg);
					break;
				}
                default:
				{
					D(DEBUG_BLOB, PRINTF("Process_Command: Bad command %d\n", cmd);)
					break;
				}
			}
		}
	}
	else { result = false; }

    *cmdpp = cmdp;
	return result;
}


// EndFile: bcmd.c
