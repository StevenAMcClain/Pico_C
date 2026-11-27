// File: io_service_strip.h

#ifndef IO_STRIP_H
#define IO_STRIP_H

#include "io_service.h"

extern ioserv_function_t IOServ_Strip;

extern bool IOSERV_Initialize_Strip(char*name, uint8_t port_pin, uint32_t num_leds, bool is_rgbw, char* user_desc);

#endif // IO_STRIP_H


// End of file: io_service_strip.h
