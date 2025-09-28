/*
 * ARX Object-Oriented Module
 * Handles object-oriented parsing including classes, methods, and fields
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

// Object-Oriented Parsing Functions
ast_node_t* parse_class(parser_context_t *context);
ast_node_t* parse_object_variable(parser_context_t *context);
ast_node_t* parse_procedure(parser_context_t *context);
ast_node_t* parse_function(parser_context_t *context);
