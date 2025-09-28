/*
 * ARX AST (Abstract Syntax Tree) Module
 * Handles AST node creation, manipulation, and destruction
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct ast_node ast_node_t;

// AST Node types
typedef enum {
    AST_NONE = 0,
    AST_MODULE,
    AST_CLASS,
    AST_OBJECT_VAR,        // Object variable (was AST_FIELD)
    AST_PROCEDURE,         // Procedure - returns no value
    AST_FUNCTION,          // Function - returns a known type
    AST_VAR_DECL,
    AST_ASSIGNMENT,
    AST_METHOD_CALL,       // Method call (temporarily restored)
    AST_PROCEDURE_CALL,    // Procedure call - no return value
    AST_FUNCTION_CALL,     // Function call - returns a value
    AST_FIELD_ACCESS,      // Keep for now, will be replaced with object variable access
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
    AST_EXPR_STMT,
    // AST_WRITELN_STMT removed - writeln is now accessed via system.writeln()
} ast_node_type_t;

// AST Node structure (from parser.h)
struct ast_node {
    ast_node_type_t type;
    char *value;                    // String value (for identifiers, literals, etc.)
    uint64_t number;                // Numeric value (for number literals)
    ast_node_t **children;          // Array of child nodes
    size_t child_count;             // Number of children
    size_t child_capacity;          // Capacity of children array
    uint32_t line_number;           // Source line number
    uint32_t column_number;         // Source column number
};

// AST Node Management Functions
ast_node_t* ast_create_node(ast_node_type_t type);
void ast_add_child(ast_node_t *parent, ast_node_t *child);
void ast_set_value(ast_node_t *node, const char *value);
void ast_set_value_from_token(ast_node_t *node, const char *token_start, size_t token_length);
void ast_set_number(ast_node_t *node, uint64_t number);
void ast_destroy_node(ast_node_t *node);
