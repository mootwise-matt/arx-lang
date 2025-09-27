/*
 * ARX Types Implementation
 * Handles type parsing including primitive types, object types, and array types
 */

#include "parser_types.h"
#include "../core/parser_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External debug flag
extern bool debug_mode;

type_info_t* parse_type(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL) {
        return NULL;
    }
    
    switch (context->lexer->token) {
        case TOK_INTEGER:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_INTEGER_PREDEF);
            
        case TOK_BOOLEAN:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_BOOLEAN_PREDEF);
            
        case TOK_CHAR:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_CHAR_PREDEF);
            
        case TOK_REAL:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_REAL_PREDEF);
            
        case TOK_STRING:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_STRING_PREDEF);
            
        case TOK_IDENT:
            // Class type
            return parse_class_type(context);
            
        case TOK_ARRAY:
            // Array type
            return parse_array_type(context);
            
        default:
            parser_error(context, "Expected type");
            return NULL;
    }
}

type_info_t* parse_primitive_type(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL) {
        return NULL;
    }
    
    switch (context->lexer->token) {
        case TOK_INTEGER:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_INTEGER_PREDEF);
            
        case TOK_BOOLEAN:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_BOOLEAN_PREDEF);
            
        case TOK_CHAR:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_CHAR_PREDEF);
            
        case TOK_REAL:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_REAL_PREDEF);
            
        default:
            return NULL;
    }
}

type_info_t* parse_object_type(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL) {
        return NULL;
    }
    
    switch (context->lexer->token) {
        case TOK_STRING:
            if (!advance_token(context)) {
                return NULL;
            }
            return type_copy(TYPE_STRING_PREDEF);
            
        case TOK_ARRAY:
            return parse_array_type(context);
            
        default:
            return NULL;
    }
}

type_info_t* parse_class_type(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL || context->lexer->token != TOK_IDENT) {
        return NULL;
    }
    
    char *class_name = context->lexer->tokstart;
    size_t class_name_len = context->lexer->toklen;
    
    if (!advance_token(context)) {
        return NULL;
    }
    
    return type_create_class(class_name, class_name_len);
}

type_info_t* parse_array_type(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL || context->lexer->token != TOK_ARRAY) {
        return NULL;
    }
    
    if (!advance_token(context)) {
        return NULL;
    }
    
    if (!expect_token(context, TOK_OF)) {
        return NULL;
    }
    
    type_info_t *element_type = parse_type(context);
    if (element_type == NULL) {
        return NULL;
    }
    
    return type_create_array(element_type, 0); // Dynamic array
}
