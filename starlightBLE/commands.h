// File: commands.h

#ifndef COMMANDS_H
#define COMMANDS_H

#include "matcher.h"
#include "parms.h"

extern matcher_t Command_Matchers;

typedef void (*Command_Function)(matcher_item_t* /*match*/, size_t /*num_params*/, parameter_t* /*params*/);

extern void Command_Run(matcher_item_t* /*match*/, size_t /*num_params*/, parameter_t* /*params*/);
extern void Command_Word(char* word, size_t num_params, parameter_t* params);

#endif // COMMANDS_H

// End of file: commands.h
