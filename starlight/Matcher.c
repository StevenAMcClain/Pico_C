// File: matcher.c

#include "Common.h"

#include <stdio.h>

#include "Matcher.h"


typedef struct Matcher
{
    const uint8_t* match_str;
    uint8_t const* match_ptr;
    const int match_code;

} MATCHER;


PRIVATE struct Matcher Matches[MATCH_LAST] = 
{
    {"PHYS", 0, MATCH_PHYS}, 
    {"SPHY", 0, MATCH_SPHY}, 
    {"SHOW", 0, MATCH_SHOW}, 
    {"SCEN", 0, MATCH_SCENE}, 
    {"BLAC", 0, MATCH_BLACK}, 
    {"UPDA", 0, MATCH_UPDATE}, 
    {"BLOB", 0, MATCH_SET_BLOB}, 
    {"TRIG", 0, MATCH_TRIGGER}, 
    {"QUEU", 0, MATCH_QUEUE}, 
    {"BRIG", 0, MATCH_BRIGHTNESS}, 
    {"INTR", 0, MATCH_INTERRUPT},
    {"DUMP", 0, MATCH_DUMP},
    {"DEBU", 0, MATCH_DEBUG},
    {"GETB", 0, MATCH_GET_BLOB},
    {"SAVE", 0, MATCH_SAVE},
    {"LOAD", 0, MATCH_LOAD},
};


PUBLIC void Matchers_Reset()
{
    for (int i = 0; i < ARRAY_SIZE(Matches); ++i)
    {
        Matches[i].match_ptr = Matches[i].match_str;
    }
}
 

PUBLIC void Matchers_Init()
{
    Matchers_Reset();
}


PUBLIC MATCH_CODE Is_Match(uint8_t ch)
{
    MATCHER* m = Matches;

    for (int i = 0; i < ARRAY_SIZE(Matches); ++i)
    {
        if (ch == *m->match_ptr++)
        {
            if (!*m->match_ptr) { return m->match_code; }
        }
        else { m->match_ptr = m->match_str; }
        ++m;
    }

    return MATCH_NONE;
}


// EndFile: Matchers.c
