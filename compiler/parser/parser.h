/*
 * ARX Parser - Builds AST from token stream
 * Fresh implementation with modern C practices
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../lexer/lexer.h"
#include "../types/types.h"
#include "../symbols/symbols.h"
#include "../common/opcodes.h"

// Forward declarations
typedef struct ast_node ast_node_t;
typedef struct parser_context parser_context_t;

// AST Node types
typedef enum {
    AST_NONE = 0,
    AST_MODULE,
    AST_CLASS,
    AST_FIELD,
    AST_METHOD,
    AST_PROCEDURE,
    AST_FUNCTION,
    AST_VAR_DECL,
    AST_ASSIGNMENT,
    AST_METHOD_CALL,
    AST_FIELD_ACCESS,
    AST_NEW_EXPR,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_BLOCK,
    AST_EXPR_STMT
} ast_node_type_t;

// AST Node structure
struct ast_node {
    ast_node_type_t type;
    char *value;                    // String value (for identifiers, literals)
    uint64_t number;               // Numeric value (for literals)
    ast_node_t **children;         // Array of child node pointers
    size_t child_count;            // Number of children
    size_t child_capacity;         // Capacity of children array
    int line_number;               // Source line number
    int column_number;             // Source column number
};

// Parser context
struct parser_context {
    lexer_context_t *lexer;        // Lexer context
    ast_node_t *root;              // Root of AST
    ast_node_t *current_node;      // Current node being built
    symbol_table_t symbol_table;   // Symbol table
    int error_count;               // Number of errors encountered
    bool in_error_recovery;        // Error recovery mode
    char *current_string_literal;  // Current string literal being parsed
    char *current_new_class;       // Current NEW expression class name
    int constructor_param_count;   // Number of constructor parameters
    bool has_constructor_params;   // Whether constructor has parameters
    
    // String literals collection for code generation
    char **method_string_literals; // Array of string literals found in current method
    size_t method_string_count;    // Number of string literals collected
    size_t method_string_capacity; // Capacity of string literals array
};

// Function prototypes
bool parser_init(parser_context_t *context, lexer_context_t *lexer);
ast_node_t* parser_parse(parser_context_t *context);
void parser_cleanup(parser_context_t *context);

// AST node management
ast_node_t* ast_create_node(ast_node_type_t type);
void ast_add_child(ast_node_t *parent, ast_node_t *child);
void ast_set_value(ast_node_t *node, const char *value);
void ast_set_number(ast_node_t *node, uint64_t number);
void ast_destroy_node(ast_node_t *node);

// Parsing functions
bool parse_module(parser_context_t *context);
ast_node_t* parse_class(parser_context_t *context);
ast_node_t* parse_field(parser_context_t *context);
ast_node_t* parse_method(parser_context_t *context);
bool parse_statement(parser_context_t *context);
ast_node_t* parse_statement_ast(parser_context_t *context);
ast_node_t* parse_assignment_statement(parser_context_t *context);
ast_node_t* parse_assignment_statement_with_var(parser_context_t *context, const char *var_name);
ast_node_t* parse_variable_declaration(parser_context_t *context);
ast_node_t* parse_writeln_statement(parser_context_t *context);
ast_node_t* parse_expression(parser_context_t *context);
ast_node_t* parse_logical_or(parser_context_t *context);
bool parse_logical_and(parser_context_t *context);
bool parse_equality(parser_context_t *context);
bool parse_relational(parser_context_t *context);
ast_node_t* parse_additive(parser_context_t *context);
ast_node_t* parse_multiplicative(parser_context_t *context);
ast_node_t* parse_unary(parser_context_t *context);
ast_node_t* parse_primary(parser_context_t *context);
ast_node_t* parse_number_literal(parser_context_t *context);
ast_node_t* parse_string_literal(parser_context_t *context);
ast_node_t* parse_identifier(parser_context_t *context);
ast_node_t* parse_postfix_operations(parser_context_t *context, char *base_name);
ast_node_t* parse_dot_expression(parser_context_t *context, char *base_name);
ast_node_t* parse_method_call_expression(parser_context_t *context, char *base_name, char *member_name);
ast_node_t* parse_field_access_expression(parser_context_t *context, char *base_name, char *member_name);
bool parse_new_expression(parser_context_t *context);
bool parse_constructor_parameters(parser_context_t *context);
bool parse_array_literal(parser_context_t *context);
bool parse_parenthesized_expression(parser_context_t *context);

// String literal collection functions
bool parser_collect_string_literal(parser_context_t *context, const char *string_literal);
void parser_clear_method_strings(parser_context_t *context);

// Utility functions
bool match_token(parser_context_t *context, token_t expected);
bool expect_token(parser_context_t *context, token_t expected);
void parser_error(parser_context_t *context, const char *message);
void parser_warning(parser_context_t *context, const char *message);
bool advance_token(parser_context_t *context);

// Type checking functions
type_info_t* parse_type(parser_context_t *context);
type_info_t* parse_primitive_type(parser_context_t *context);
type_info_t* parse_object_type(parser_context_t *context);
type_info_t* parse_class_type(parser_context_t *context);
type_info_t* parse_array_type(parser_context_t *context);

// Symbol table integration
bool add_symbol_to_current_scope(parser_context_t *context, symbol_t *symbol);
symbol_t* lookup_symbol(parser_context_t *context, const char *name, size_t name_len);
bool enter_scope(parser_context_t *context, const char *name);
bool exit_scope(parser_context_t *context);
