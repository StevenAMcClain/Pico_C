// File: spsc_queue.h

#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include "common.h"

extern void spsc_queue_init(void);

extern void ble_rx(const char* data, size_t len);   // called from BLE thread

typedef void (*packet_processor_t)(char* data, size_t len);

extern void consume(packet_processor_t processor);   // called from core1 thread

#endif // SPSC_QUEUE_H


// End of file: spsc_queue.h
