/*
 * ARX Symbol Table Implementation
 * Implements symbol management, scoping, and symbol resolution
 */

#include "symbols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

// Hash table size (should be prime)
#define SYMBOL_HASH_SIZE 101

bool symbol_table_init(symbol_table_t *table)
{
    if (table == NULL) {
        return false;
    }
    
    table->global_scope = scope_create(0, "global");
    if (table->global_scope == NULL) {
        return false;
    }
    
    table->current_scope = table->global_scope;
    table->next_scope_level = 1;
    table->total_symbols = 0;
    
    if (debug_mode) {
        printf("Symbol table initialized\n");
    }
    
    return true;
}

void symbol_table_cleanup(symbol_table_t *table)
{
    if (table == NULL) {
        return;
    }
    
    if (table->global_scope != NULL) {
        scope_destroy(table->global_scope);
        table->global_scope = NULL;
    }
    
    table->current_scope = NULL;
    table->next_scope_level = 0;
    table->total_symbols = 0;
}

scope_t* scope_create(int level, const char *name)
{
    scope_t *scope = malloc(sizeof(scope_t));
    if (scope == NULL) {
        return NULL;
    }
    
    scope->level = level;
    scope->name = NULL;
    if (name != NULL) {
        scope->name = malloc(strlen(name) + 1);
        if (scope->name == NULL) {
            free(scope);
            return NULL;
        }
        strcpy(scope->name, name);
    }
    
    scope->symbols = calloc(SYMBOL_HASH_SIZE, sizeof(symbol_t*));
    if (scope->symbols == NULL) {
        free(scope->name);
        free(scope);
        return NULL;
    }
    
    scope->symbol_count = 0;
    scope->symbol_capacity = SYMBOL_HASH_SIZE;
    scope->parent = NULL;
    scope->child = NULL;
    scope->sibling = NULL;
    
    return scope;
}

void scope_destroy(scope_t *scope)
{
    if (scope == NULL) {
        return;
    }
    
    // Destroy all symbols in this scope
    for (size_t i = 0; i < scope->symbol_capacity; i++) {
        symbol_t *symbol = scope->symbols[i];
        while (symbol != NULL) {
            symbol_t *next = symbol->next;
            symbol_destroy(symbol);
            symbol = next;
        }
    }
    
    // Destroy child scopes
    scope_t *child = scope->child;
    while (child != NULL) {
        scope_t *next = child->sibling;
        scope_destroy(child);
        child = next;
    }
    
    free(scope->symbols);
    free(scope->name);
    free(scope);
}

bool scope_enter(symbol_table_t *table, const char *name)
{
    if (table == NULL) {
        return false;
    }
    
    scope_t *new_scope = scope_create(table->next_scope_level++, name);
    if (new_scope == NULL) {
        return false;
    }
    
    new_scope->parent = table->current_scope;
    new_scope->sibling = table->current_scope->child;
    table->current_scope->child = new_scope;
    table->current_scope = new_scope;
    
    if (debug_mode) {
        printf("Entered scope: %s (level %d)\n", name ? name : "anonymous", new_scope->level);
    }
    
    return true;
}

bool scope_exit(symbol_table_t *table)
{
    if (table == NULL || table->current_scope == NULL || table->current_scope->parent == NULL) {
        return false;
    }
    
    scope_t *old_scope = table->current_scope;
    table->current_scope = old_scope->parent;
    
    if (debug_mode) {
        printf("Exited scope: %s (level %d)\n", old_scope->name ? old_scope->name : "anonymous", old_scope->level);
    }
    
    return true;
}

scope_t* scope_find(symbol_table_t *table, int level)
{
    if (table == NULL) {
        return NULL;
    }
    
    scope_t *scope = table->current_scope;
    while (scope != NULL) {
        if (scope->level == level) {
            return scope;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

symbol_t* symbol_create(const char *name, size_t name_len, symbol_type_t type)
{
    symbol_t *symbol = malloc(sizeof(symbol_t));
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->name = malloc(name_len + 1);
    if (symbol->name == NULL) {
        free(symbol);
        return NULL;
    }
    
    strncpy(symbol->name, name, name_len);
    symbol->name[name_len] = '\0';
    symbol->name_len = name_len;
    
    symbol->type = type;
    symbol->type_info = NULL;
    symbol->scope_level = 0;
    symbol->line_number = 0;
    symbol->column_number = 0;
    symbol->next = NULL;
    
    // Initialize type-specific data
    memset(&symbol->data, 0, sizeof(symbol->data));
    
    return symbol;
}

void symbol_destroy(symbol_t *symbol)
{
    if (symbol == NULL) {
        return;
    }
    
    free(symbol->name);
    
    if (symbol->type_info != NULL) {
        type_destroy(symbol->type_info);
    }
    
    // Clean up type-specific data
    switch (symbol->type) {
        case SYMBOL_PROCEDURE:
        case SYMBOL_FUNCTION:
            if (symbol->data.procedure.parameter_types != NULL) {
                for (size_t i = 0; i < symbol->data.procedure.parameter_count; i++) {
                    if (symbol->data.procedure.parameter_types[i] != NULL) {
                        type_destroy(symbol->data.procedure.parameter_types[i]);
                    }
                }
                free(symbol->data.procedure.parameter_types);
            }
            break;
        case SYMBOL_METHOD:
            if (symbol->data.method.parameter_types != NULL) {
                for (size_t i = 0; i < symbol->data.method.parameter_count; i++) {
                    if (symbol->data.method.parameter_types[i] != NULL) {
                        type_destroy(symbol->data.method.parameter_types[i]);
                    }
                }
                free(symbol->data.method.parameter_types);
            }
            if (symbol->data.method.class_name != NULL) {
                free(symbol->data.method.class_name);
            }
            break;
        case SYMBOL_FIELD:
            if (symbol->data.field.class_name != NULL) {
                free(symbol->data.field.class_name);
            }
            break;
        default:
            break;
    }
    
    free(symbol);
}

bool symbol_add(symbol_table_t *table, symbol_t *symbol)
{
    if (table == NULL || symbol == NULL || table->current_scope == NULL) {
        return false;
    }
    
    // Check if symbol already exists in current scope
    symbol_t *existing = symbol_lookup_in_scope(table->current_scope, symbol->name, symbol->name_len);
    if (existing != NULL) {
        if (debug_mode) {
            printf("Error: Symbol '%s' already exists in current scope\n", symbol->name);
        }
        return false;
    }
    
    // Set scope level
    symbol->scope_level = table->current_scope->level;
    
    // Add to hash table
    uint32_t hash = symbol_hash(symbol->name, symbol->name_len) % table->current_scope->symbol_capacity;
    symbol->next = table->current_scope->symbols[hash];
    table->current_scope->symbols[hash] = symbol;
    
    table->current_scope->symbol_count++;
    table->total_symbols++;
    
    if (debug_mode) {
        printf("Added symbol: %s (type: %d, scope: %d)\n", symbol->name, symbol->type, symbol->scope_level);
    }
    
    return true;
}

symbol_t* symbol_lookup(symbol_table_t *table, const char *name, size_t name_len)
{
    if (table == NULL || name == NULL) {
        return NULL;
    }
    
    // Search from current scope up to global scope
    scope_t *scope = table->current_scope;
    while (scope != NULL) {
        symbol_t *symbol = symbol_lookup_in_scope(scope, name, name_len);
        if (symbol != NULL) {
            return symbol;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

symbol_t* symbol_lookup_in_scope(scope_t *scope, const char *name, size_t name_len)
{
    if (scope == NULL || name == NULL) {
        return NULL;
    }
    
    uint32_t hash = symbol_hash(name, name_len) % scope->symbol_capacity;
    symbol_t *symbol = scope->symbols[hash];
    
    while (symbol != NULL) {
        if (symbol->name_len == name_len && strncmp(symbol->name, name, name_len) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }
    
    return NULL;
}

symbol_t* symbol_lookup_global(symbol_table_t *table, const char *name, size_t name_len)
{
    if (table == NULL || table->global_scope == NULL) {
        return NULL;
    }
    
    return symbol_lookup_in_scope(table->global_scope, name, name_len);
}

symbol_t* symbol_create_variable(const char *name, size_t name_len, type_info_t *type_info, int scope_level)
{
    symbol_t *symbol = symbol_create(name, name_len, SYMBOL_VARIABLE);
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->type_info = type_copy(type_info);
    symbol->scope_level = scope_level;
    symbol->data.variable.offset = 0; // Will be set during code generation
    symbol->data.variable.is_parameter = false;
    
    return symbol;
}

symbol_t* symbol_create_constant(const char *name, size_t name_len, type_info_t *type_info, int64_t value)
{
    symbol_t *symbol = symbol_create(name, name_len, SYMBOL_CONSTANT);
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->type_info = type_copy(type_info);
    symbol->data.constant.value = value;
    
    return symbol;
}

symbol_t* symbol_create_procedure(const char *name, size_t name_len, size_t address, size_t parameter_count)
{
    symbol_t *symbol = symbol_create(name, name_len, SYMBOL_PROCEDURE);
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->data.procedure.address = address;
    symbol->data.procedure.parameter_count = parameter_count;
    symbol->data.procedure.parameter_types = calloc(parameter_count, sizeof(type_info_t*));
    
    return symbol;
}

symbol_t* symbol_create_function(const char *name, size_t name_len, type_info_t *return_type, size_t address, size_t parameter_count)
{
    symbol_t *symbol = symbol_create(name, name_len, SYMBOL_FUNCTION);
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->type_info = type_copy(return_type);
    symbol->data.function.address = address;
    symbol->data.function.return_type = type_copy(return_type);
    symbol->data.function.parameter_count = parameter_count;
    symbol->data.function.parameter_types = calloc(parameter_count, sizeof(type_info_t*));
    
    return symbol;
}

symbol_t* symbol_create_class(const char *name, size_t name_len, size_t field_count, size_t method_count, const char *parent_class)
{
    symbol_t *symbol = symbol_create(name, name_len, SYMBOL_CLASS);
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->data.class_info.field_count = field_count;
    symbol->data.class_info.method_count = method_count;
    symbol->data.class_info.instance_size = 0; // Will be calculated
    
    // Set parent class
    if (parent_class != NULL) {
        symbol->data.class_info.parent_class = malloc(strlen(parent_class) + 1);
        if (symbol->data.class_info.parent_class == NULL) {
            symbol_destroy(symbol);
            return NULL;
        }
        strcpy(symbol->data.class_info.parent_class, parent_class);
    } else {
        symbol->data.class_info.parent_class = NULL;
    }
    
    return symbol;
}

symbol_t* symbol_create_field(const char *name, size_t name_len, type_info_t *type_info, size_t offset, const char *class_name)
{
    symbol_t *symbol = symbol_create(name, name_len, SYMBOL_FIELD);
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->type_info = type_copy(type_info);
    symbol->data.field.offset = offset;
    symbol->data.field.class_name = malloc(strlen(class_name) + 1);
    if (symbol->data.field.class_name == NULL) {
        symbol_destroy(symbol);
        return NULL;
    }
    strcpy(symbol->data.field.class_name, class_name);
    
    return symbol;
}

symbol_t* symbol_create_method(const char *name, size_t name_len, type_info_t *return_type, size_t address, const char *class_name, size_t parameter_count)
{
    symbol_t *symbol = symbol_create(name, name_len, SYMBOL_METHOD);
    if (symbol == NULL) {
        return NULL;
    }
    
    symbol->type_info = type_copy(return_type);
    symbol->data.method.address = address;
    symbol->data.method.return_type = type_copy(return_type);
    symbol->data.method.class_name = malloc(strlen(class_name) + 1);
    if (symbol->data.method.class_name == NULL) {
        symbol_destroy(symbol);
        return NULL;
    }
    strcpy(symbol->data.method.class_name, class_name);
    symbol->data.method.parameter_count = parameter_count;
    symbol->data.method.parameter_types = calloc(parameter_count, sizeof(type_info_t*));
    
    return symbol;
}

uint32_t symbol_hash(const char *name, size_t name_len)
{
    uint32_t hash = 5381;
    for (size_t i = 0; i < name_len; i++) {
        hash = ((hash << 5) + hash) + name[i];
    }
    return hash;
}

void symbol_table_dump(symbol_table_t *table)
{
    if (table == NULL) {
        printf("Symbol table is NULL\n");
        return;
    }
    
    printf("\n=== Symbol Table Dump ===\n");
    printf("Total symbols: %zu\n", table->total_symbols);
    printf("Current scope level: %d\n", table->current_scope ? table->current_scope->level : -1);
    printf("\n");
    
    scope_dump(table->global_scope, 0);
    printf("\n=== End Symbol Table ===\n");
}

void scope_dump(scope_t *scope, int indent)
{
    if (scope == NULL) {
        return;
    }
    
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    printf("Scope: %s (level %d, %zu symbols)\n", 
           scope->name ? scope->name : "anonymous", 
           scope->level, 
           scope->symbol_count);
    
    // Dump symbols in this scope
    for (size_t i = 0; i < scope->symbol_capacity; i++) {
        symbol_t *symbol = scope->symbols[i];
        while (symbol != NULL) {
            for (int j = 0; j < indent + 1; j++) {
                printf("  ");
            }
            symbol_dump(symbol);
            symbol = symbol->next;
        }
    }
    
    // Dump child scopes
    scope_t *child = scope->child;
    while (child != NULL) {
        scope_dump(child, indent + 1);
        child = child->sibling;
    }
}

void symbol_dump(symbol_t *symbol)
{
    if (symbol == NULL) {
        printf("NULL symbol\n");
        return;
    }
    
    printf("Symbol: %s (type: %d, scope: %d, line: %d)", 
           symbol->name, 
           symbol->type, 
           symbol->scope_level, 
           symbol->line_number);
    
    // Show inheritance information for classes
    if (symbol->type == SYMBOL_CLASS && symbol->data.class_info.parent_class != NULL) {
        printf(" extends %s", symbol->data.class_info.parent_class);
    }
    
    printf("\n");
    
    if (symbol->type_info != NULL) {
        printf("    Type: %s\n", type_to_string(symbol->type_info));
    }
}

bool symbol_type_check(symbol_t *symbol, type_info_t *expected_type)
{
    if (symbol == NULL || expected_type == NULL) {
        return false;
    }
    
    return type_equals(symbol->type_info, expected_type);
}

bool symbol_assignable(symbol_t *symbol, type_info_t *from_type)
{
    if (symbol == NULL || from_type == NULL) {
        return false;
    }
    
    return type_assignable(from_type, symbol->type_info);
}
