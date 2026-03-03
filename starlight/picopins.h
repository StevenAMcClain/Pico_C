// File: picopins.h

#ifndef PICOPINS_H
#define PICOPINS_H

#define PICOPIN_MIN 0
#define PICOPIN_MAX 31

extern bool picopin_reserve(uint pin);
extern void picopin_release(uint pin);
extern bool picopin_is_reserved(uint pin);

#endif  // PICOPINS_H

// EndFile: picopins.h
