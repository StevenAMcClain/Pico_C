// File: matcher.c

#include "common.h"

//#include <stdio.h>

#include "matcher.h"


typedef struct Matcher
{
    const char* match_str;
    uint8_t const* match_ptr;
    const int match_code;

} MATCHER;


PRIVATE struct Matcher Matches[MATCH_LAST] = 
{
    {"RESE", 0, MATCH_RESET}, 
    {"BLAC", 0, MATCH_BLACK}, 

    {"SENG", 0, MATCH_SENG}, 
    {"SPHY", 0, MATCH_SPHY}, 

    {"BLOB", 0, MATCH_SET_BLOB}, 
    {"XBLB", 0, MATCH_EXPORT_BLOB},

    {"SCEN", 0, MATCH_SCENE}, 
    {"SHOW", 0, MATCH_SHOW}, 
    {"UPDA", 0, MATCH_UPDATE}, 

    {"TRIG", 0, MATCH_TRIGGER}, 
    {"INTR", 0, MATCH_INTERRUPT},
    {"QUEU", 0, MATCH_QUEUE}, 

    {"SETV", 0, MATCH_SETV}, 
    {"GETV", 0, MATCH_GETV}, 

    {"LOAD", 0, MATCH_LOAD},
    {"SAVE", 0, MATCH_SAVE},
    {"ERAS", 0, MATCH_ERASE},

    {"DUMP", 0, MATCH_DUMP},

    // {"PHYS", 0, MATCH_PHYS}, 
    {"BRIG", 0, MATCH_BRIGHTNESS}, 
    {"DEBU", 0, MATCH_DEBUG},
};


PUBLIC void Matchers_Reset()
{
    for (int i = 0; i < ARRAY_SIZE(Matches); ++i)
    {
        Matches[i].match_ptr = Matches[i].match_str;
    }
}
 

// PUBLIC void Matchers_Init()
// {
//     Matchers_Reset();
// }


PUBLIC MATCH_CODE Is_Match(uint8_t ch)
{
    MATCHER* m = Matches;

    for (int i = 0; i < ARRAY_SIZE(Matches); ++i, ++m)
    {
        if ((char)ch == *m->match_ptr++)
        {
            if (!*m->match_ptr) { return m->match_code; }
        }
        else { m->match_ptr = m->match_str; }
    }

    return MATCH_NONE;
}


// EndFile: Matchers.c
