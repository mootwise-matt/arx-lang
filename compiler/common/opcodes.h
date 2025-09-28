/*
 * ARX Virtual Machine Opcodes and Instruction Format
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

// Virtual Machine Context
typedef struct
{
    uint8_t  *mem;      /* program memory               */
    int64_t  *dstack;   /* data stack memory            */
    uint64_t pc;        /* program counter     (mem)    */
    uint64_t t;         /* stack pointer/index (dstack) */
    uint64_t b;         /* base pointer/index  (dstack) */
    size_t   inscount;  /* number of instructions executed */
    uint64_t memsize;   /* number of bytes in mem buffer */
} vm_context_t;

// Opcodes (lower nibble)
typedef enum
{
    VM_LIT      = 0,        // load literal constant 0,n
    VM_OPR      = 1,        // arithmetic or logical operation 0,n
    VM_LOD      = 2,        // load variable v,d
    VM_STO      = 3,        // store variable v,d
    VM_CAL      = 4,        // call procedure or function v,a
    VM_INT      = 5,        // increment stack pointer 0,n
    VM_JMP      = 6,        // unconditional jump 0,a
    VM_JPC      = 7,        // jump if false (tos = 0) 0,a
    VM_LODX     = 8,        // load indexed v,d with offset loaded onto stack
    VM_STOX     = 9,        // store indexed v,d with offset loaded onto stack
    VM_HALT     = 10        // halt execution
} opcode_t;

// Operations for VM_OPR (stored in immediate field)
typedef enum
{
    OPR_RET     = 0,        // return from procedure
    OPR_NEG     = 1,        // negate
    OPR_ADD     = 2,        // addition
    OPR_SUB     = 3,        // subtraction
    OPR_MUL     = 4,        // multiplication
    OPR_DIV     = 5,        // division
    OPR_POW     = 6,        // exponentiation (^)
    OPR_MOD     = 7,        // modulo (%)
    OPR_ODD     = 8,        // odd test
    OPR_NULL    = 9,        // null operation
    OPR_EQ      = 10,       // equality
    OPR_NEQ     = 11,       // not equal
    OPR_LESS    = 12,       // less than
    OPR_LEQ     = 13,       // less than or equal
    OPR_GREATER = 14,       // greater than
    OPR_GEQ     = 15,       // greater than or equal
    OPR_AND     = 16,       // logical AND
    OPR_OR      = 17,       // logical OR
    OPR_NOT     = 18,       // logical NOT
    OPR_SHR     = 19,       // shift right
    OPR_SHL     = 20,       // shift left
    OPR_SAR     = 21,       // shift arithmetic right
    OPR_OUTCHAR = 22,       // output character
    OPR_OUTINT  = 23,       // output integer
    OPR_OUTSTRING = 24,     // output string
    // OPR_WRITELN removed - writeln is now accessed via system.writeln()
    OPR_INCHAR  = 26,       // input character
    OPR_ININT   = 27,       // input integer
    
    // String operations
    OPR_STR_CREATE = 28,    // Create string from literal
    OPR_STR_SLICE = 29,     // Create string slice
    OPR_STR_CONCAT = 30,    // Concatenate strings
    OPR_STR_LEN = 31,       // Get string length
    OPR_STR_EQ = 32,        // String equality
    OPR_STR_CMP = 33,       // String comparison
    OPR_STR_BUILDER_CREATE = 34,  // Create string builder
    OPR_STR_BUILDER_APPEND = 35,  // Append to builder
    OPR_STR_BUILDER_TO_STR = 36,  // Convert builder to string
    OPR_STR_DATA = 37,      // String data marker
    OPR_INT_TO_STR = 38,    // Convert integer to string
    OPR_STR_TO_INT = 39,    // Convert string to integer
    
    // Object system operations
    OPR_OBJ_CREATE = 40,    // Create object instance
    OPR_OBJ_CALL_METHOD = 41, // Call object method
    OPR_OBJ_RETURN = 42,    // Return from method
    OPR_OBJ_GET_FIELD = 43, // Get object field value
    OPR_OBJ_SET_FIELD = 44, // Set object field value
    OPR_OBJ_SELF = 45,      // Reference to self
    OPR_OBJ_NEW = 46,       // NEW operator
    OPR_OBJ_DOT = 47,       // Dot operator for field/method access
    OPR_SQRT = 48,          // Square root function
    OPR_REAL_LIT = 49       // Real number literal
} opr_t;

// Instruction format (packed for efficiency)
#pragma pack(push, 1)
typedef struct
{
    uint8_t     opcode;     // lower nibble is opcode_t, upper is level for procedures/functions
    uint64_t    opt64;      // optional 64-bit payload (address or literal)
} instruction_t;
#pragma pack(pop)

// ARX Module File Format (.arxmod)
// 80-byte header (packed)
#pragma pack(push, 1)
typedef struct
{
    char        magic[8];        // "ARXMOD\0\0" - file format identifier
    uint32_t    version;         // Format version (1)
    uint32_t    flags;           // Format flags (reserved for future use)
    uint64_t    header_size;     // Size of this header (80)
    uint64_t    toc_offset;      // Offset to Table of Contents
    uint64_t    toc_size;        // Size of Table of Contents
    uint64_t    data_offset;     // Offset to data sections
    uint64_t    data_size;       // Total size of all data sections
    uint64_t    app_name_len;    // Length of APP object name
    uint64_t    app_data_size;   // Size of APP object data
    uint64_t    entry_point;     // Entry point offset (pre-calculated by linker)
} arxmod_header_t;
#pragma pack(pop)

// Table of Contents entry
#pragma pack(push, 1)
typedef struct
{
    char        section_name[16]; // Section name (null-terminated)
    uint64_t    offset;           // Offset from start of data sections
    uint64_t    size;             // Size of this section
    uint32_t    flags;            // Section flags
    uint32_t    reserved;         // Reserved for future use
} arxmod_toc_entry_t;
#pragma pack(pop)

// ARX Module Section Names
// Section names moved to arxmod_constants.h

// Include centralized constants
#include "arxmod_constants.h"

// String implementation
typedef struct
{
    uint8_t  *data;     // UTF-8 byte sequence (immutable)
    uint64_t len;       // Length in bytes (not characters)
    uint64_t hash;      // Precomputed hash for fast comparison
} arx_string_t;

// String slice (zero-copy substring)
typedef struct
{
    arx_string_t *base; // Reference to base string
    uint64_t      start; // Start offset in bytes
    uint64_t      len;   // Length in bytes
} arx_string_slice_t;

// String builder (mutable, for concatenation)
typedef struct
{
    uint8_t  *data;     // Mutable buffer
    uint64_t len;       // Current length
    uint64_t cap;       // Capacity
} arx_string_builder_t;
