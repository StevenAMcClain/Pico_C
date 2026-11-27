// Filename: globmatch.c

#include "common.h"
#include "globmatch.h"

#include <ctype.h>


PRIVATE int ch_eq(char a, char b, int flags)
{
    if (flags & FNM_CASEFOLD) {
        a = (char)tolower((unsigned char)a);
        b = (char)tolower((unsigned char)b);
    }
    return a == b;
}

PUBLIC int globmatch(const char *pat, const char *str, int flags)
{
    while (*pat) 
    {
        if (*pat == '*') 
        {
            while (*pat == '*') { ++pat; }

            if (*pat == '\0') { return 0; }

            while (*str) 
            {
                if (globmatch(pat, str, flags) == 0) { return 0; }
                ++str;
            }
            return FNM_NOMATCH;
        }

        if (*pat == '?') 
        {
            if (*str == '\0') { return FNM_NOMATCH; }
            ++pat;    ++str;
            continue;
        }

        if (*str == '\0') { return FNM_NOMATCH; }

        if (!ch_eq(*pat, *str, flags))  { return FNM_NOMATCH; }

        ++pat;    ++str;
    }

    return (*str == '\0') ? 0 : FNM_NOMATCH;
}


// EndFile: globmatch.c
