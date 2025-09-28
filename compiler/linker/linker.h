#ifndef ARX_LINKER_H
#define ARX_LINKER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../arxmod/arxmod.h"

// Linker context for resolving addresses
typedef struct {
    class_entry_t *classes;           // Class manifest
    size_t class_count;               // Number of classes
    method_entry_t *methods;          // Method manifest with offsets
    size_t method_count;              // Number of methods
    field_entry_t *fields;            // Field manifest with offsets
    size_t field_count;               // Number of fields
    uint64_t base_address;            // Base address for class layout
    uint64_t current_offset;          // Current offset in memory layout
} linker_context_t;

// Method resolution entry
typedef struct {
    uint64_t class_id;                // Class ID
    char *method_name;                // Method name
    uint64_t address;                 // Resolved address
    uint64_t offset;                  // Offset within class
} method_resolution_t;

// Field layout entry
typedef struct {
    uint64_t class_id;                // Class ID
    char *field_name;                 // Field name
    uint64_t offset;                  // Offset within class instance
    uint64_t size;                    // Size of field
} field_layout_t;

// Linker functions
bool linker_init(linker_context_t *linker, class_entry_t *classes, size_t class_count, method_entry_t *methods, size_t method_count, field_entry_t *fields, size_t field_count);
void linker_cleanup(linker_context_t *linker);
bool linker_resolve_method_address(linker_context_t *linker, uint64_t class_id, const char *method_name, uint64_t *address);
bool linker_calculate_class_layout(linker_context_t *linker, uint64_t class_id, uint64_t *instance_size, field_layout_t **fields, size_t *field_count);
bool linker_patch_bytecode(linker_context_t *linker, instruction_t *instructions, size_t instruction_count, const char **string_table, size_t string_count);
bool linker_update_class_manifest(linker_context_t *linker, instruction_t *instructions, size_t instruction_count);

#endif // ARX_LINKER_H
