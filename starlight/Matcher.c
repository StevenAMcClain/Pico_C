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
    {"PHYS", 0, MATCH_PHYS}, 
    {"SCEN", 0, MATCH_SCENE}, 
    {"BLAC", 0, MATCH_BLACK}, 
    {"BLOB", 0, MATCH_BLOB}, 
    {"TRIG", 0, MATCH_TRIGGER}, 
    {"QUEU", 0, MATCH_QUEUE}, 
    {"BRIG", 0, MATCH_BRIGHTNESS}, 
    {"INTR", 0, MATCH_INTERRUPT}
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
    // struct Matcher* mp = Matches;

    // mp->match_str = "PHYS";     mp++->match_code = MATCH_PHYS;
    // mp->match_str = "SCEN";     mp++->match_code = MATCH_SCENE;
    // mp->match_str = "BLAC";     mp++->match_code = MATCH_BLACK;
    // mp->match_str = "BLOB";     mp++->match_code = MATCH_BLOB;
    // mp->match_str = "TRIG";     mp++->match_code = MATCH_TRIGGER;
    // mp->match_str = "QUEU";     mp++->match_code = MATCH_QUEUE;
    // mp->match_str = "BRIG";     mp++->match_code = MATCH_BRIGHTNESS;
    // mp->match_str = "INTR";     mp++->match_code = MATCH_INTERRUPT;

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
