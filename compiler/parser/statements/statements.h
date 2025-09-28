/*
 * ARX Statements Module
 * Handles statement parsing including variable declarations, assignments, and writeln
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

// Statement Parsing Functions
bool parse_statement(parser_context_t *context);
ast_node_t* parse_statement_ast(parser_context_t *context);

// Variable Declaration Functions
ast_node_t* parse_variable_declaration(parser_context_t *context);

// Assignment Statement Functions
ast_node_t* parse_assignment_statement_with_var(parser_context_t *context, const char *var_name);
ast_node_t* parse_assignment_statement(parser_context_t *context);

// Output Statement Functions
ast_node_t* parse_writeln_statement(parser_context_t *context);

// Control Flow Statement Functions
ast_node_t* parse_for_statement(parser_context_t *context);
ast_node_t* parse_while_statement(parser_context_t *context);
ast_node_t* parse_if_statement(parser_context_t *context);
ast_node_t* parse_return_statement(parser_context_t *context);
