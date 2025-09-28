/*
 * ARX Module Format Constants
 * Centralized definitions for all ARX module format constants
 */

#ifndef ARXMOD_CONSTANTS_H
#define ARXMOD_CONSTANTS_H

#include <stdint.h>

// ARX Module Format Constants
#define ARXMOD_MAGIC            "ARXMOD\0\0"
#define ARXMOD_VERSION          1
#define ARXMOD_HEADER_SIZE      80
#define ARXMOD_ALIGNMENT        16

// ARX Module Flags
#define ARXMOD_FLAG_LIBRARY     0x00000001  // Module is a library (no entry point)
#define ARXMOD_FLAG_EXECUTABLE  0x00000002  // Module is an executable (has entry point)

// Section Names
#define ARXMOD_SECTION_CODE     "CODE"
#define ARXMOD_SECTION_STRINGS  "STRINGS"
#define ARXMOD_SECTION_SYMBOLS  "SYMBOLS"
#define ARXMOD_SECTION_DEBUG    "DEBUG"
#define ARXMOD_SECTION_CLASSES  "CLASSES"
#define ARXMOD_SECTION_APP      "APP"

// Structure sizes for validation (defined after structures are declared)
// These will be defined in opcodes.h after the structures are declared

#endif // ARXMOD_CONSTANTS_H
