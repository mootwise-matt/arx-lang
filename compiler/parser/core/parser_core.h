/*
 * ARX Core Parser Module
 * Handles core parser functionality, initialization, and utility functions
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../ast/ast.h"
#include "../../lexer/lexer.h"
#include "../../types/types.h"
#include "../../symbols/symbols.h"

// Forward declarations
typedef struct parser_context parser_context_t;

// Method signature structure
typedef struct {
    char *class_name;          // Class containing the method
    char *method_name;         // Method name
    char *return_type;         // Return type (NULL for procedures, type for functions)
    char **param_types;        // Parameter types array
    size_t param_count;        // Number of parameters
    bool is_procedure;         // True if procedure, false if function
} method_signature_t;

// Parser context structure (from parser.h)
struct parser_context {
    lexer_context_t *lexer;        // Lexer context
    ast_node_t *root;              // Root of AST
    ast_node_t *current_node;      // Current node being built
    symbol_table_t symbol_table;   // Symbol table
    int error_count;               // Number of errors encountered
    bool in_error_recovery;        // Error recovery mode
    
    // String literal handling
    char *current_string_literal;  // Current string being parsed
    char **method_string_literals; // Array of string literals in current method
    size_t method_string_count;    // Number of string literals
    size_t method_string_capacity; // Capacity of string literals array
    
    // NEW expression handling
    char *current_new_class;       // Class name for NEW expression
    int constructor_param_count;   // Number of constructor parameters
    bool has_constructor_params;   // Whether constructor has parameters
    
    // Method signature storage
    void *method_signatures;       // Array of method signatures (opaque pointer)
    size_t method_signature_count;          // Number of method signatures
    size_t method_signature_capacity;       // Capacity of method signatures array
};

// Core Parser Functions
bool parser_init(parser_context_t *context, lexer_context_t *lexer);
ast_node_t* parser_parse(parser_context_t *context);
void parser_cleanup(parser_context_t *context);

// Module Parsing
bool parse_module(parser_context_t *context);

// Utility Functions
bool match_token(parser_context_t *context, token_t expected);
bool expect_token(parser_context_t *context, token_t expected);
void parser_error(parser_context_t *context, const char *message);

// Method Signature Management
bool parser_add_method_signature(parser_context_t *context, const char *class_name, 
                                const char *method_name, const char *return_type, 
                                char **param_types, size_t param_count);
method_signature_t* parser_lookup_method_signature(parser_context_t *context, 
                                                  const char *class_name, const char *method_name);
bool parser_is_procedure(parser_context_t *context, const char *class_name, const char *method_name);
void parser_warning(parser_context_t *context, const char *message);
bool advance_token(parser_context_t *context);

// Symbol Table Functions
bool add_symbol_to_current_scope(parser_context_t *context, symbol_t *symbol);
symbol_t* lookup_symbol(parser_context_t *context, const char *name, size_t name_len);
bool enter_scope(parser_context_t *context, const char *name);
bool exit_scope(parser_context_t *context);

// String Literal Collection Functions
bool parser_collect_string_literal(parser_context_t *context, const char *string_literal);
void parser_clear_method_strings(parser_context_t *context);
