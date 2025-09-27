/*
 * ARX Types Module
 * Handles type parsing including primitive types, object types, and array types
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../types/types.h"
#include "../../lexer/lexer.h"

// Forward declarations
typedef struct parser_context parser_context_t;

// Type Parsing Functions
type_info_t* parse_type(parser_context_t *context);
type_info_t* parse_primitive_type(parser_context_t *context);
type_info_t* parse_object_type(parser_context_t *context);
type_info_t* parse_class_type(parser_context_t *context);
type_info_t* parse_array_type(parser_context_t *context);
