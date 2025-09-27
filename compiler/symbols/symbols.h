/*
 * ARX Symbol Table
 * Manages symbols, scopes, and symbol resolution
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../types/types.h"

// Symbol types
typedef enum {
    SYMBOL_NONE = 0,
    SYMBOL_VARIABLE,
    SYMBOL_CONSTANT,
    SYMBOL_PROCEDURE,
    SYMBOL_FUNCTION,
    SYMBOL_CLASS,
    SYMBOL_FIELD,
    SYMBOL_METHOD,
    SYMBOL_PARAMETER
} symbol_type_t;

// Symbol information
typedef struct symbol {
    char *name;                     // Symbol name
    size_t name_len;                // Name length
    symbol_type_t type;             // Symbol type
    type_info_t *type_info;         // Type information
    int scope_level;                // Scope level (0 = global)
    int line_number;                // Declaration line
    int column_number;              // Declaration column
    union {
        struct {
            int64_t value;          // Constant value
        } constant;
        struct {
            size_t offset;          // Stack offset
            bool is_parameter;      // Is function parameter
        } variable;
        struct {
            size_t address;         // Code address
            size_t parameter_count; // Number of parameters
            type_info_t **parameter_types; // Parameter types
        } procedure;
        struct {
            size_t address;         // Code address
            type_info_t *return_type; // Return type
            size_t parameter_count; // Number of parameters
            type_info_t **parameter_types; // Parameter types
        } function;
        struct {
            size_t field_count;     // Number of fields
            size_t method_count;    // Number of methods
            size_t instance_size;   // Instance size in bytes
            char *parent_class;     // Parent class name (for inheritance)
        } class_info;
        struct {
            size_t offset;          // Field offset in class
            char *class_name;       // Containing class name
        } field;
        struct {
            size_t address;         // Method address
            char *class_name;       // Containing class name
            type_info_t *return_type; // Return type
            size_t parameter_count; // Number of parameters
            type_info_t **parameter_types; // Parameter types
        } method;
    } data;
    struct symbol *next;            // Next symbol in hash chain
} symbol_t;

// Scope information
typedef struct scope {
    int level;                      // Scope level
    char *name;                     // Scope name (for functions/classes)
    symbol_t **symbols;             // Hash table of symbols
    size_t symbol_count;            // Number of symbols
    size_t symbol_capacity;         // Hash table capacity
    struct scope *parent;           // Parent scope
    struct scope *child;            // First child scope
    struct scope *sibling;          // Next sibling scope
} scope_t;

// Symbol table
typedef struct symbol_table {
    scope_t *global_scope;          // Global scope
    scope_t *current_scope;         // Current scope
    int next_scope_level;           // Next scope level
    size_t total_symbols;           // Total symbols across all scopes
} symbol_table_t;

// Symbol table functions
bool symbol_table_init(symbol_table_t *table);
void symbol_table_cleanup(symbol_table_t *table);

// Scope management
scope_t* scope_create(int level, const char *name);
void scope_destroy(scope_t *scope);
bool scope_enter(symbol_table_t *table, const char *name);
bool scope_exit(symbol_table_t *table);
scope_t* scope_find(symbol_table_t *table, int level);

// Symbol management
symbol_t* symbol_create(const char *name, size_t name_len, symbol_type_t type);
void symbol_destroy(symbol_t *symbol);
bool symbol_add(symbol_table_t *table, symbol_t *symbol);
symbol_t* symbol_lookup(symbol_table_t *table, const char *name, size_t name_len);
symbol_t* symbol_lookup_in_scope(scope_t *scope, const char *name, size_t name_len);
symbol_t* symbol_lookup_global(symbol_table_t *table, const char *name, size_t name_len);

// Symbol type specific functions
symbol_t* symbol_create_variable(const char *name, size_t name_len, type_info_t *type_info, int scope_level);
symbol_t* symbol_create_constant(const char *name, size_t name_len, type_info_t *type_info, int64_t value);
symbol_t* symbol_create_procedure(const char *name, size_t name_len, size_t address, size_t parameter_count);
symbol_t* symbol_create_function(const char *name, size_t name_len, type_info_t *return_type, size_t address, size_t parameter_count);
symbol_t* symbol_create_class(const char *name, size_t name_len, size_t field_count, size_t method_count, const char *parent_class);
symbol_t* symbol_create_field(const char *name, size_t name_len, type_info_t *type_info, size_t offset, const char *class_name);
symbol_t* symbol_create_method(const char *name, size_t name_len, type_info_t *return_type, size_t address, const char *class_name, size_t parameter_count);

// Utility functions
uint32_t symbol_hash(const char *name, size_t name_len);
void symbol_table_dump(symbol_table_t *table);
void scope_dump(scope_t *scope, int indent);
void symbol_dump(symbol_t *symbol);

// Type checking functions
bool symbol_type_check(symbol_t *symbol, type_info_t *expected_type);
bool symbol_assignable(symbol_t *symbol, type_info_t *from_type);
