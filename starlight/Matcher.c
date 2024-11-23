// File: matcher.c

#include "Common.h"

#include <stdio.h>

#include "Matcher.h"


typedef struct Matcher
{
    uint8_t* match_str;
    uint8_t* match_ptr;
    int match_code;

} MATCHER;


PRIVATE struct Matcher Matches[MATCH_LAST] = {0};


PUBLIC void Matchers_Reset()
{
    for (int i = 0; i < ARRAY_SIZE(Matches); ++i)
    {
        Matches[i].match_ptr = Matches[i].match_str;
    }
}
 

PUBLIC void Matchers_Init()
{
    struct Matcher* mp = Matches;

    mp->match_str = "CONF";     mp++->match_code = MATCH_CONFIG;
    mp->match_str = "SCEN";     mp++->match_code = MATCH_SCENE;
    mp->match_str = "BLAC";     mp++->match_code = MATCH_BLACK;
    mp->match_str = "BLOB";     mp++->match_code = MATCH_BLOB;
    mp->match_str = "TRIG";     mp++->match_code = MATCH_TRIGGER;
    mp->match_str = "QUEU";     mp++->match_code = MATCH_QUEUE;
    mp->match_str = "BRIG";     mp++->match_code = MATCH_BRIGHTNESS;
    mp->match_str = "INTR";     mp++->match_code = MATCH_INTERRUPT;

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
