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
#include "ast/ast.h"
#include "core/parser_core.h"
#include "expressions/expressions.h"
#include "statements/statements.h"
#include "object_oriented/object_oriented.h"
#include "types/parser_types.h"

// Forward declarations
typedef struct parser_context parser_context_t;

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
ast_node_t* parse_new_expression(parser_context_t *context);
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
