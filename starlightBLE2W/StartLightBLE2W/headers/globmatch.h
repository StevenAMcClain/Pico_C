// File: globmatch.h

#ifndef GLOBMATCH_H
#define GLOBMATCH_H

#define FNM_NOMATCH 1
#define FNM_CASEFOLD 0x01

int globmatch(const char *pattern, const char *string, int flags);

#endif // GLOBMATCH_H

// End of file: globmatch.h
