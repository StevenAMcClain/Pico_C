// File: nus_service.h
//
// Nordic UART Service (NUS) interface layer.
// -------------------------------------------
// This module exposes a BLE "UART-like" data channel using the standard
// Nordic UART Service UUIDs. It provides a simple read/write abstraction
// so higher-level code can treat BLE traffic as a byte stream.
//
// The service consists of two characteristics:
//
//   - RX characteristic (Write / Write Without Response)
//       Client -> Server data path
//
//   - TX characteristic (Notify, Read)
//       Server -> Client data path
//
// The functions below integrate NUS into the dynamic ATT dispatch system
// used by the rest of the I/O service framework. They allow BTstack to
// route ATT read/write requests to the NUS handlers and allow the NUS
// module to push notifications back to the client.
//
// NUS_MAX_LEN defines the maximum payload size accepted by the RX handler.
// (244 bytes (or MTU minus 3 bytes) is the maximum ATT payload after MTU expansion on most BLE stacks.)

#ifndef NUS_SERVICE_H
#define NUS_SERVICE_H

#include "BLECore.h"    // For connection_handle

#define NUS_MAX_LEN 244     // Maximum RX payload length accepted by NUS.


// Initialize the Nordic UART Service and register its characteristics
// with the ATT database. Must be called once during BLE startup.
extern void nus_service_initialize(void);


// Returns true if the given ATT handle belongs to the NUS service.
// Used by the dispatcher to route ATT operations to NUS handlers.
extern bool nus_service_handle_match(handle_t handle);


// Handle an incoming ATT Write or Write Without Response directed at
// the NUS RX characteristic. 'buffer' contains the data written by the
// client. Returns ATT_ERROR_SUCCESS on success or an ATT error code.
extern int nus_service_write(handle_t attribute_handle,
                             uint16_t transaction_mode,
                             uint16_t offset,
                             uint8_t *buffer,
                             uint16_t buffer_size);


// Handle an ATT Read request directed at the NUS TX characteristic.
// Fills 'buffer' with outgoing data and returns the number of bytes
// written. If no data is available, returns 0.
extern uint16_t nus_service_read(handle_t attribute_handle,
                                 uint16_t offset,
                                 uint8_t *buffer,
                                 uint16_t buffer_size);

#endif // NUS_SERVICE_H

// End of file: nus_service.h
