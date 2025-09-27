# ARX Module File Format (.arxmod)

The ARX Module format is a binary file format used to store compiled ARX programs. It provides a structured way to package bytecode, metadata, and debugging information.

## Overview

The `.arxmod` file format is designed to be:
- **Portable**: Works across different platforms and architectures
- **Extensible**: Easy to add new sections and features
- **Efficient**: Optimized for fast loading and execution
- **Debuggable**: Includes debugging and symbol information

## File Structure

```
┌─────────────────┐
│   Header (64B)  │  ← File format identification and metadata
├─────────────────┤
│   TOC (var)     │  ← Table of Contents with section info
├─────────────────┤
│   Data Sections │  ← Actual program data
│   ┌───────────┐ │
│   │   CODE    │ │  ← Bytecode instructions
│   ├───────────┤ │
│   │  STRINGS  │ │  ← String literals
│   ├───────────┤ │
│   │  SYMBOLS  │ │  ← Symbol table
│   ├───────────┤ │
│   │   DEBUG   │ │  ← Debug information
│   ├───────────┤ │
│   │    APP    │ │  ← Application data
│   └───────────┘ │
└─────────────────┘
```

## Header Format

The header is exactly 64 bytes and contains:

```c
typedef struct {
    char        magic[8];        // "ARXMOD\0\0" - file format identifier
    uint32_t    version;         // Format version (1)
    uint32_t    flags;           // Format flags (reserved for future use)
    uint64_t    header_size;     // Size of this header (64)
    uint64_t    toc_offset;      // Offset to Table of Contents
    uint64_t    toc_size;        // Size of Table of Contents
    uint64_t    data_offset;     // Offset to data sections
    uint64_t    data_size;       // Total size of all data sections
    uint64_t    app_name_len;    // Length of APP object name
    uint64_t    app_data_size;   // Size of APP object data
    uint64_t    reserved[1];     // Reserved for future use
} arxmod_header_t;
```

### Header Fields

| Field | Type | Description |
|-------|------|-------------|
| `magic` | char[8] | File format identifier: "ARXMOD\0\0" |
| `version` | uint32_t | Format version number (currently 1) |
| `flags` | uint32_t | Format flags (reserved, must be 0) |
| `header_size` | uint64_t | Size of header (always 64) |
| `toc_offset` | uint64_t | Byte offset to Table of Contents |
| `toc_size` | uint64_t | Size of Table of Contents in bytes |
| `data_offset` | uint64_t | Byte offset to data sections |
| `data_size` | uint64_t | Total size of all data sections |
| `app_name_len` | uint64_t | Length of application name |
| `app_data_size` | uint64_t | Size of application data |
| `reserved` | uint64_t[1] | Reserved for future use |

## Table of Contents (TOC)

The TOC is an array of section descriptors:

```c
typedef struct {
    char        section_name[16]; // Section name (null-terminated)
    uint64_t    offset;           // Offset from start of data sections
    uint64_t    size;             // Size of this section
    uint32_t    flags;            // Section flags
    uint32_t    reserved;         // Reserved for future use
} arxmod_toc_entry_t;
```

### TOC Entry Fields

| Field | Type | Description |
|-------|------|-------------|
| `section_name` | char[16] | Section name (null-terminated) |
| `offset` | uint64_t | Offset from start of data sections |
| `size` | uint64_t | Size of section in bytes |
| `flags` | uint32_t | Section-specific flags |
| `reserved` | uint32_t | Reserved for future use |

## Data Sections

### CODE Section

Contains the compiled bytecode instructions:

```c
typedef struct {
    uint8_t     opcode; // lower nibble is opcode_t, upper is level
    uint64_t    opt64;  // optional 64-bit payload
} instruction_t;
```

**Section Name**: `"CODE"`
**Purpose**: Store executable bytecode instructions
**Alignment**: 16 bytes

### STRINGS Section

Contains string literals used in the program:

```
┌─────────────┬─────────────┬─────────────┐
│ String 1    │ String 2    │ String 3    │
│ (null-term) │ (null-term) │ (null-term) │
└─────────────┴─────────────┴─────────────┘
```

**Section Name**: `"STRINGS"`
**Purpose**: Store string literals and identifiers
**Format**: Concatenated null-terminated strings

### SYMBOLS Section

Contains symbol table information:

```c
typedef struct {
    char *name;                     // Symbol name
    uint64_t name_offset;           // Offset in string table
    uint32_t type;                  // Symbol type
    uint32_t flags;                 // Symbol flags
    uint64_t value;                 // Symbol value/address
    uint32_t scope_level;           // Scope level
} symbol_entry_t;
```

**Section Name**: `"SYMBOLS"`
**Purpose**: Store symbol table for debugging and linking
**Alignment**: 8 bytes

### DEBUG Section

Contains debugging information:

```c
typedef struct {
    uint32_t line_number;           // Source line number
    uint32_t column_number;         // Source column number
    uint64_t instruction_offset;    // Offset in code section
    uint64_t file_name_offset;      // Offset to filename in string table
} debug_entry_t;
```

**Section Name**: `"DEBUG"`
**Purpose**: Map bytecode to source code locations
**Alignment**: 8 bytes

### APP Section

Contains application-specific data:

```
┌─────────────┬─────────────┐
│ App Name    │ App Data    │
│ (var len)   │ (var size)  │
└─────────────┴─────────────┘
```

**Section Name**: `"APP"`
**Purpose**: Store application metadata and data
**Format**: App name followed by application data

## Format Constants

```c
#define ARXMOD_MAGIC            "ARXMOD\0\0"
#define ARXMOD_VERSION          1
#define ARXMOD_HEADER_SIZE      64
#define ARXMOD_ALIGNMENT        16
```

## Section Names

```c
#define ARXMOD_SECTION_CODE     "CODE"
#define ARXMOD_SECTION_STRINGS  "STRINGS"
#define ARXMOD_SECTION_SYMBOLS  "SYMBOLS"
#define ARXMOD_SECTION_DEBUG    "DEBUG"
#define ARXMOD_SECTION_APP      "APP"
```

## Byte Order

All multi-byte values are stored in **little-endian** format for consistency across platforms.

## Alignment

- Header: 64 bytes (fixed)
- TOC entries: 8-byte aligned
- Data sections: 16-byte aligned
- Individual data types: Natural alignment

## Validation

A valid ARX module file must:

1. Start with the magic number "ARXMOD\0\0"
2. Have version number 1
3. Have header size of 64 bytes
4. Have valid TOC offset and size
5. Have valid data offset and size
6. Have properly aligned sections

## Tools

### arxmod_info

Command-line tool for inspecting ARX module files:

```bash
# Show module information
arxmod_info module.arxmod

# Show section details
arxmod_info -sections module.arxmod

# Validate file format
arxmod_info -validate module.arxmod

# Show hex dump of header
arxmod_info -hex module.arxmod
```

## Example Usage

### Reading an ARX Module

```c
#include "arxmod.h"

arxmod_reader_t reader;
if (arxmod_reader_init(&reader, "program.arxmod")) {
    if (arxmod_reader_validate(&reader)) {
        if (arxmod_reader_load_toc(&reader)) {
            // Load code section
            instruction_t *instructions;
            size_t instruction_count;
            if (arxmod_reader_load_code_section(&reader, &instructions, &instruction_count)) {
                // Execute instructions...
                free(instructions);
            }
        }
    }
    arxmod_reader_cleanup(&reader);
}
```

### Writing an ARX Module

```c
#include "arxmod.h"

arxmod_writer_t writer;
if (arxmod_writer_init(&writer, "program.arxmod")) {
    if (arxmod_writer_write_header(&writer, "MyApp", 5)) {
        if (arxmod_writer_add_code_section(&writer, instructions, instruction_count)) {
            if (arxmod_writer_finalize(&writer)) {
                printf("Module written successfully\n");
            }
        }
    }
    arxmod_writer_cleanup(&writer);
}
```

## Future Extensions

The format is designed to be extensible. Future versions may include:

- **Compression**: Compressed sections for smaller file sizes
- **Encryption**: Encrypted sections for security
- **Dependencies**: Module dependency information
- **Resources**: Embedded resources (images, sounds, etc.)
- **Metadata**: Additional metadata sections
- **Optimization**: Optimized bytecode variants

## Version History

- **Version 1**: Initial format specification
  - Basic header and TOC structure
  - CODE, STRINGS, SYMBOLS, DEBUG, APP sections
  - Little-endian byte order
  - 16-byte alignment
