// File: matcher.c

#include "common.h"

//#include <stdio.h>

#include "matcher.h"


typedef struct Matcher
{
    const uint8_t* match_str;
    uint8_t const* match_ptr;
    const int match_code;

} MATCHER;


PRIVATE struct Matcher Matches[MATCH_LAST] = 
{
    // {"PHYS", 0, MATCH_PHYS}, 
    {(uint8_t*)"SPHY", 0, MATCH_SPHY}, 
    {(uint8_t*)"SHOW", 0, MATCH_SHOW}, 
    {(uint8_t*)"SCEN", 0, MATCH_SCENE}, 
    {(uint8_t*)"BLAC", 0, MATCH_BLACK}, 
    {(uint8_t*)"UPDA", 0, MATCH_UPDATE}, 
    {(uint8_t*)"BLOB", 0, MATCH_SET_BLOB}, 
    {(uint8_t*)"TRIG", 0, MATCH_TRIGGER}, 
    {(uint8_t*)"QUEU", 0, MATCH_QUEUE}, 
    {(uint8_t*)"BRIG", 0, MATCH_BRIGHTNESS}, 
    {(uint8_t*)"INTR", 0, MATCH_INTERRUPT},
    {(uint8_t*)"DUMP", 0, MATCH_DUMP},
    {(uint8_t*)"DEBU", 0, MATCH_DEBUG},
    {(uint8_t*)"GETB", 0, MATCH_GET_BLOB},
    {(uint8_t*)"SAVE", 0, MATCH_SAVE},
    {(uint8_t*)"LOAD", 0, MATCH_LOAD},
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
