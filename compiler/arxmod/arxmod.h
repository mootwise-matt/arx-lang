/*
 * ARX Module File Format (.arxmod)
 * Binary format for compiled ARX programs
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "../common/arxmod_constants.h"
#include "../common/opcodes.h"

// ARX Module Writer
typedef struct {
    FILE *file;                     // Output file
    size_t header_offset;           // Offset to header in file
    size_t toc_offset;              // Offset to TOC in file
    size_t data_offset;             // Offset to data sections
    size_t current_data_offset;     // Current position in data sections
    size_t section_count;           // Number of sections
    arxmod_toc_entry_t *toc_entries; // TOC entries
    size_t toc_capacity;            // TOC capacity
    uint32_t module_flags;          // Module flags (library/executable)
    uint64_t entry_point;           // Entry point offset (pre-calculated by linker)
    bool debug_output;              // Debug output flag
} arxmod_writer_t;

// ARX Module Reader
typedef struct {
    FILE *file;                     // Input file
    arxmod_header_t header;         // Module header
    arxmod_toc_entry_t *toc;        // Table of Contents
    size_t toc_count;               // Number of TOC entries
    bool debug_output;              // Debug output flag
} arxmod_reader_t;

// Section data structures
typedef struct {
    char *name;                     // Section name
    uint8_t *data;                  // Section data
    size_t size;                    // Section size
    uint32_t flags;                 // Section flags
} arxmod_section_t;

// String table entry
typedef struct {
    uint64_t offset;                // Offset in string table
    uint64_t length;                // String length
    uint64_t hash;                  // String hash
} string_entry_t;

// Symbol table entry
typedef struct {
    char *name;                     // Symbol name
    uint64_t name_offset;           // Offset in string table
    uint32_t type;                  // Symbol type
    uint32_t flags;                 // Symbol flags
    uint64_t value;                 // Symbol value/address
    uint32_t scope_level;           // Scope level
} symbol_entry_t;

// Debug information entry
typedef struct {
    uint32_t line_number;           // Source line number
    uint32_t column_number;         // Source column number
    uint64_t instruction_offset;    // Offset in code section
    uint64_t file_name_offset;      // Offset to filename in string table
} debug_entry_t;

// Method entry for class manifest
typedef struct {
    char        method_name[32];   // Method name (null-terminated)
    uint64_t    method_id;         // Unique method ID
    uint64_t    offset;            // Offset from class base address
    uint32_t    parameter_count;   // Number of parameters
    uint32_t    return_type_id;    // Return type ID (0 for procedures, type ID for functions)
    uint32_t    flags;             // Method flags (static, virtual, etc.)
    uint32_t    reserved;          // Reserved for future use
} method_entry_t;

// Field entry for class manifest
typedef struct {
    char        field_name[32];    // Field name (null-terminated)
    uint64_t    field_id;          // Unique field ID
    uint64_t    offset;            // Offset from object base address
    uint32_t    type_id;           // Type ID (for type checking)
    uint32_t    size;              // Field size in bytes
    uint32_t    flags;             // Field flags (private, protected, etc.)
    uint32_t    reserved;          // Reserved for future use
} field_entry_t;

// Class table entry - self-contained with inline methods and fields
typedef struct {
    char        class_name[32];    // Class name (null-terminated)
    uint64_t    class_id;          // Unique class ID
    uint32_t    field_count;       // Number of fields
    uint32_t    method_count;      // Number of methods
    uint64_t    parent_class_id;   // Parent class ID (0 if none)
    uint32_t    instance_size;     // Total size of class instance in bytes
    uint32_t    flags;             // Class flags
    uint32_t    reserved;          // Reserved for future use
    // Methods and fields are stored inline after this structure
    // Each class is self-contained to avoid name conflicts
} class_entry_t;

// Function prototypes for writer
bool arxmod_writer_init(arxmod_writer_t *writer, const char *filename);
bool arxmod_writer_set_flags(arxmod_writer_t *writer, uint32_t flags);
bool arxmod_writer_set_entry_point(arxmod_writer_t *writer, uint64_t entry_point);
bool arxmod_writer_update_header(arxmod_writer_t *writer);
bool arxmod_writer_write_header(arxmod_writer_t *writer, const char *app_name, size_t app_name_len);
bool arxmod_writer_add_code_section(arxmod_writer_t *writer, instruction_t *instructions, size_t instruction_count);
bool arxmod_writer_add_strings_section(arxmod_writer_t *writer, const char **strings, size_t string_count);
bool arxmod_writer_add_symbols_section(arxmod_writer_t *writer, symbol_entry_t *symbols, size_t symbol_count);
bool arxmod_writer_add_debug_section(arxmod_writer_t *writer, debug_entry_t *debug_info, size_t debug_count);
bool arxmod_writer_add_classes_section(arxmod_writer_t *writer, class_entry_t *classes, size_t class_count, method_entry_t *methods, size_t method_count, field_entry_t *fields, size_t field_count);
bool arxmod_writer_add_app_section(arxmod_writer_t *writer, const char *app_name, size_t app_name_len, const uint8_t *app_data, size_t app_data_size);
bool arxmod_writer_finalize(arxmod_writer_t *writer);
void arxmod_writer_cleanup(arxmod_writer_t *writer);

// Function prototypes for reader
bool arxmod_reader_init(arxmod_reader_t *reader, const char *filename);
bool arxmod_reader_validate(arxmod_reader_t *reader);
bool arxmod_reader_load_toc(arxmod_reader_t *reader);
arxmod_toc_entry_t* arxmod_reader_find_section(arxmod_reader_t *reader, const char *section_name);
bool arxmod_reader_load_code_section(arxmod_reader_t *reader, instruction_t **instructions, size_t *instruction_count);
bool arxmod_reader_load_strings_section(arxmod_reader_t *reader, char ***strings, size_t *string_count);
bool arxmod_reader_load_symbols_section(arxmod_reader_t *reader, symbol_entry_t **symbols, size_t *symbol_count);
bool arxmod_reader_load_debug_section(arxmod_reader_t *reader, debug_entry_t **debug_info, size_t *debug_count);
bool arxmod_reader_load_classes_section(arxmod_reader_t *reader, class_entry_t **classes, size_t *class_count, method_entry_t **methods, size_t *method_count, field_entry_t **fields, size_t *field_count);
bool arxmod_reader_load_app_section(arxmod_reader_t *reader, char **app_name, uint8_t **app_data, size_t *app_data_size);
void arxmod_reader_cleanup(arxmod_reader_t *reader);

// Utility functions
void arxmod_dump_info(arxmod_reader_t *reader);
void arxmod_dump_sections(arxmod_reader_t *reader);
bool arxmod_validate_file(const char *filename);
uint64_t arxmod_calculate_hash(const uint8_t *data, size_t length);
