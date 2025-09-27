/*
 * ARX Expressions Module
 * Handles expression parsing including arithmetic, logical, and method calls
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../ast/ast.h"
#include "../../lexer/lexer.h"
#include "../../types/types.h"

// Forward declarations
typedef struct parser_context parser_context_t;

// Expression Parsing Functions
ast_node_t* parse_expression(parser_context_t *context);
ast_node_t* parse_logical_or(parser_context_t *context);
ast_node_t* parse_equality_ast(parser_context_t *context);
ast_node_t* parse_relational_ast(parser_context_t *context);
bool parse_logical_and(parser_context_t *context);
bool parse_equality(parser_context_t *context);
bool parse_relational(parser_context_t *context);
ast_node_t* parse_additive(parser_context_t *context);
ast_node_t* parse_multiplicative(parser_context_t *context);
ast_node_t* parse_unary(parser_context_t *context);
ast_node_t* parse_primary(parser_context_t *context);

// Literal Parsing Functions
bool parse_array_literal(parser_context_t *context);
ast_node_t* parse_number_literal(parser_context_t *context);
ast_node_t* parse_string_literal(parser_context_t *context);
ast_node_t* parse_identifier(parser_context_t *context);

// Postfix and Method Call Functions
ast_node_t* parse_postfix_operations(parser_context_t *context, char *base_name);
ast_node_t* parse_dot_expression(parser_context_t *context, char *base_name);
ast_node_t* parse_method_call_expression(parser_context_t *context, char *base_name, char *member_name);
ast_node_t* parse_field_access_expression(parser_context_t *context, char *base_name, char *member_name);

// Object Creation Functions
bool parse_new_expression(parser_context_t *context);
bool parse_constructor_parameters(parser_context_t *context);
bool parse_parenthesized_expression(parser_context_t *context);
