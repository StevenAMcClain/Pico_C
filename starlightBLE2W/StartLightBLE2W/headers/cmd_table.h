// File: command_table.h


#ifndef COMMAND_TABLE_H
#define COMMAND_TABLE_H

#include "command.h"

typedef enum 
{   //               v -- must start at one!
    CMD_CODE_CHECK = 1,   // Calculate config file checksum.
    CMD_CODE_CONFIG,      // Dump out config file.
    CMD_CODE_HELP,        // Dump out a list of commands.
    CMD_CODE_IF,          // Test expression.
    CMD_CODE_LIST,        // List the configed ports.
    CMD_CODE_LOAD,        // Load config file and if valid save to flash.
    CMD_CODE_PARMS,       // Dump parameters.
    CMD_CODE_PIN,         // Define pin driver.
    CMD_CODE_PRINT,       // Print values.
    CMD_CODE_RETURN,      // Return result.
    CMD_CODE_SRVS,        // Setup Att services.
    CMD_CODE_WATCH,       // Set or clear watch on vars.

} ALT_CMD_CODE;

#define ALT(x) (command_code_t)(x)    // used to 'join' enums.

// num_params == 0 passes params == NULL. Every command function must guard for this.

extern command_array_t Command_Table;

#endif // COMMAND_TABLE_H


// End of file: command_table.h
