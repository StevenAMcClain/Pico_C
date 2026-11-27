// File: config.h

#ifndef CONFIG_H
#define CONFIG_H

extern char* Config_File(void);

extern bool Load_Config(void);
extern int Save_Config(const char* str);
extern bool Process_Config(const char* configp, bool apply_config);

#define CONFIG_FILE_MAX_LEN (4 * 1024)   // 4 sectors of flash for config file.

// Configuration buffer format
//
// Line format rules:
//
// - Every line ends with '\n'.
// - Every line begins with a non‑space ASCII character.
// - Fields are separated by one or more spaces or tabs.
// - Leading or trailing whitespace is allowed only where the parser
//   currently skips it (skip_space).
// - Mixed whitespace (space + tab) is allowed.
// - Empty lines are not allowed.
// - All characters must be ASCII (0x00–0x7F).
//
// Line 1: BLE header
//
//   Must begin with "BLE", followed by:
//     - item count (decimal integer)
//     - checksum (decimal integer)
//
//   The checksum is computed over all subsequent characters,
//   including whitespace and newline characters.
//
// Line 2: ADV header
//
//   Must begin with "ADV", followed by the device name.
//
//   Device name rules (as implemented):
//     - Single field (no spaces or tabs).
//     - First character must be alphabetic (A–Z, a–z).
//     - Subsequent characters must be alphanumeric (A–Z, a–z, 0–9)
//       or underscore ('_').
//
// Remaining lines: configuration items
//
//   If a line begins with "PIN", it defines a pin configuration:
//
//       - next field: pin number (decimal integer)
//       - next field: pin type (IN, OUT, PWM)
//       - next field: pin name (single field, no spaces or tabs)
//                      first char alphabetic, rest alphanumeric or '_'
//       - remainder of the line: pin description
//         (may contain spaces or tabs; may be empty)
//         description ends at the newline character
//
//   Pin types:
//       IN   = GPIO input
//       OUT  = GPIO output
//       PWM  = PWM output
//
// Additional notes:
//
//   - Lines must not exceed the maximum buffer size.
//   - Unknown keywords must be rejected as errors.
//   - The parser stops on the first error and reports it.
//   - Whitespace between fields is flexible because skip_space()
//     consumes any number of spaces or tabs.
//   - Description text is accepted exactly as-is until '\n'.// Configuration buffer format

#endif // CONFIG_H


// End of file: config.h
