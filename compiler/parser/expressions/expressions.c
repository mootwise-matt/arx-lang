/*
 * ARX Expressions Implementation
 * Handles expression parsing including arithmetic, logical, and method calls
 */

#include "expressions.h"
#include "../core/parser_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External debug flag
extern bool debug_mode;

ast_node_t* parse_expression(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing expression\n");
    }
    
    return parse_logical_or(context);
}

ast_node_t* parse_logical_or(parser_context_t *context)
{
    // Parse logical OR expressions (||)
    ast_node_t *left = parse_logical_and_ast(context);
    if (!left) {
        return NULL;
    }
    
    while (context->lexer->token == TOK_OR) {
        if (!advance_token(context)) {
            ast_destroy_node(left);
            return NULL;
        }
        
        ast_node_t *right = parse_logical_and_ast(context);
        if (!right) {
            ast_destroy_node(left);
            return NULL;
        }
        
        // Create binary operation node
        ast_node_t *op_node = ast_create_node(AST_BINARY_OP);
        if (!op_node) {
            ast_destroy_node(left);
            ast_destroy_node(right);
            return NULL;
        }
        
        op_node->value = strdup("||");
        ast_add_child(op_node, left);
        ast_add_child(op_node, right);
        left = op_node;
    }
    
    return left;
}

ast_node_t* parse_logical_and_ast(parser_context_t *context)
{
    // Parse logical AND expressions (&&)
    ast_node_t *left = parse_equality_ast(context);
    if (!left) {
        return NULL;
    }
    
    while (context->lexer->token == TOK_AND) {
        if (!advance_token(context)) {
            ast_destroy_node(left);
            return NULL;
        }
        
        ast_node_t *right = parse_equality_ast(context);
        if (!right) {
            ast_destroy_node(left);
            return NULL;
        }
        
        // Create binary operation node
        ast_node_t *op_node = ast_create_node(AST_BINARY_OP);
        if (!op_node) {
            ast_destroy_node(left);
            ast_destroy_node(right);
            return NULL;
        }
        
        op_node->value = strdup("&&");
        ast_add_child(op_node, left);
        ast_add_child(op_node, right);
        left = op_node;
    }
    
    return left;
}

bool parse_logical_and(parser_context_t *context)
{
    if (!parse_equality(context)) {
        return false;
    }
    
    while (match_token(context, TOK_AND)) {
        if (!advance_token(context)) {
            return false;
        }
        if (!parse_equality(context)) {
            return false;
        }
    }
    
    return true;
}

ast_node_t* parse_equality_ast(parser_context_t *context)
{
    // Parse equality expressions (== and !=)
    ast_node_t *left = parse_relational_ast(context);
    if (!left) {
        return NULL;
    }
    
    while (match_token(context, TOK_EQUAL) || match_token(context, TOK_NEQ)) {
        int op = context->lexer->token;
        if (!advance_token(context)) {
            ast_destroy_node(left);
            return NULL;
        }
        
        ast_node_t *right = parse_relational_ast(context);
        if (!right) {
            ast_destroy_node(left);
            return NULL;
        }
        
        // Create binary operation node
        ast_node_t *binary_op = ast_create_node(AST_BINARY_OP);
        if (!binary_op) {
            ast_destroy_node(left);
            ast_destroy_node(right);
            return NULL;
        }
        
        // Set the operator
        if (op == TOK_EQUAL) {
            ast_set_value(binary_op, "==");
        } else if (op == TOK_NEQ) {
            ast_set_value(binary_op, "!=");
        }
        
        // Add children
        ast_add_child(binary_op, left);
        ast_add_child(binary_op, right);
        
        left = binary_op;
    }
    
    return left;
}

bool parse_equality(parser_context_t *context)
{
    if (!parse_relational(context)) {
        return false;
    }
    
    while (match_token(context, TOK_EQUAL) || match_token(context, TOK_NEQ)) {
        if (!advance_token(context)) {
            return false;
        }
        if (!parse_relational(context)) {
            return false;
        }
    }
    
    return true;
}

ast_node_t* parse_relational_ast(parser_context_t *context)
{
    // Parse relational expressions (<, <=, >, >=)
    ast_node_t *left = parse_additive(context);
    if (!left) {
        return NULL;
    }
    
    while (match_token(context, TOK_LESS) || match_token(context, TOK_LEQ) ||
           match_token(context, TOK_GREATER) || match_token(context, TOK_GEQ)) {
        int op = context->lexer->token;
        if (!advance_token(context)) {
            ast_destroy_node(left);
            return NULL;
        }
        
        ast_node_t *right = parse_additive(context);
        if (!right) {
            ast_destroy_node(left);
            return NULL;
        }
        
        // Create binary operation node
        ast_node_t *binary_op = ast_create_node(AST_BINARY_OP);
        if (!binary_op) {
            ast_destroy_node(left);
            ast_destroy_node(right);
            return NULL;
        }
        
        // Set the operator
        if (op == TOK_LESS) {
            ast_set_value(binary_op, "<");
        } else if (op == TOK_LEQ) {
            ast_set_value(binary_op, "<=");
        } else if (op == TOK_GREATER) {
            ast_set_value(binary_op, ">");
        } else if (op == TOK_GEQ) {
            ast_set_value(binary_op, ">=");
        }
        
        // Add children
        ast_add_child(binary_op, left);
        ast_add_child(binary_op, right);
        
        left = binary_op;
    }
    
    return left;
}

bool parse_relational(parser_context_t *context)
{
    if (!parse_additive(context)) {
        return false;
    }
    
    while (match_token(context, TOK_LESS) || match_token(context, TOK_LEQ) ||
           match_token(context, TOK_GREATER) || match_token(context, TOK_GEQ)) {
        if (!advance_token(context)) {
            return false;
        }
        if (!parse_additive(context)) {
            return false;
        }
    }
    
    return true;
}

ast_node_t* parse_additive(parser_context_t *context)
{
    ast_node_t *left = parse_multiplicative(context);
    if (!left) {
        return NULL;
    }
    
    while (match_token(context, TOK_PLUS) || match_token(context, TOK_MINUS)) {
        // Create binary operation node
        ast_node_t *op_node = ast_create_node(AST_BINARY_OP);
        if (!op_node) {
            ast_destroy_node(left);
            return NULL;
        }
        
        // Set the operator
        if (match_token(context, TOK_PLUS)) {
            ast_set_value(op_node, "+");
        } else {
            ast_set_value(op_node, "-");
        }
        
        if (!advance_token(context)) {
            ast_destroy_node(left);
            ast_destroy_node(op_node);
            return NULL;
        }
        
        ast_node_t *right = parse_multiplicative(context);
        if (!right) {
            ast_destroy_node(left);
            ast_destroy_node(op_node);
            return NULL;
        }
        
        // Add left and right operands as children
        ast_add_child(op_node, left);
        ast_add_child(op_node, right);
        
        // The result becomes the new left operand
        left = op_node;
    }
    
    return left;
}

ast_node_t* parse_multiplicative(parser_context_t *context)
{
    ast_node_t *left = parse_unary(context);
    if (!left) {
        return NULL;
    }
    
    while (match_token(context, TOK_STAR) || match_token(context, TOK_SLASH) || 
           match_token(context, TOK_CARET) || match_token(context, TOK_PERCENT)) {
        // Create binary operation node
        ast_node_t *op_node = ast_create_node(AST_BINARY_OP);
        if (!op_node) {
            ast_destroy_node(left);
            return NULL;
        }
        
        // Set the operator
        if (match_token(context, TOK_STAR)) {
            ast_set_value(op_node, "*");
        } else if (match_token(context, TOK_SLASH)) {
            ast_set_value(op_node, "/");
        } else if (match_token(context, TOK_CARET)) {
            ast_set_value(op_node, "^");
        } else {
            ast_set_value(op_node, "%");
        }
        
        if (!advance_token(context)) {
            ast_destroy_node(left);
            ast_destroy_node(op_node);
            return NULL;
        }
        
        ast_node_t *right = parse_unary(context);
        if (!right) {
            ast_destroy_node(left);
            ast_destroy_node(op_node);
            return NULL;
        }
        
        // Add left and right operands as children
        ast_add_child(op_node, left);
        ast_add_child(op_node, right);
        
        // The result becomes the new left operand
        left = op_node;
    }
    
    return left;
}


ast_node_t* parse_unary(parser_context_t *context)
{
    if (match_token(context, TOK_MINUS) || match_token(context, TOK_EXCLAMATION)) {
        // Create unary operation node
        ast_node_t *op_node = ast_create_node(AST_UNARY_OP);
        if (!op_node) {
            return NULL;
        }
        
        // Set the operator
        if (match_token(context, TOK_MINUS)) {
            ast_set_value(op_node, "-");
        } else {
            ast_set_value(op_node, "!");
        }
        
        if (!advance_token(context)) {
            ast_destroy_node(op_node);
            return NULL;
        }
        
        ast_node_t *operand = parse_primary(context);
        if (!operand) {
            ast_destroy_node(op_node);
            return NULL;
        }
        
        ast_add_child(op_node, operand);
        return op_node;
    }
    
    return parse_primary(context);
}

ast_node_t* parse_primary(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing primary\n");
    }
    
    ast_node_t *node = NULL;
    
    switch (context->lexer->token) {
        case TOK_NUMBER:
            node = parse_number_literal(context);
            break;
            
        case TOK_STRING:
            printf("PARSE_EXPRESSION: Found TOK_STRING, calling parse_string_literal\n");
            node = parse_string_literal(context);
            printf("PARSE_EXPRESSION: parse_string_literal returned: %p\n", node);
            break;
            
        case TOK_IDENT:
            node = parse_identifier(context);
            break;
            
        case TOK_NEW:
            node = parse_new_expression(context);
            break;
            
        case TOK_LPAREN:
            // Parse parenthesized expression: (expression)
            if (debug_mode) {
                printf("Parsing parenthesized expression\n");
            }
            
            // Consume opening parenthesis
            if (!advance_token(context)) {
                return NULL;
            }
            
            // Parse the expression inside parentheses
            ast_node_t *expr_node = parse_expression(context);
            if (!expr_node) {
                return NULL;
            }
            
            // Expect closing parenthesis
            if (!match_token(context, TOK_RPAREN)) {
                parser_error(context, "Expected ')' after parenthesized expression");
                ast_destroy_node(expr_node);
                return NULL;
            }
            
            if (!advance_token(context)) {
                ast_destroy_node(expr_node);
                return NULL;
            }
            
            // Return the expression node (parentheses don't create a new node)
            return expr_node;
            
        case TOK_LBRACKET:
            // TODO: Implement array literal parsing
            if (debug_mode) {
                printf("Array literal not yet implemented in AST\n");
            }
            return NULL;
            
        default:
            parser_error(context, "Expected primary expression");
            return NULL;
    }
    
    return node;
}

bool parse_array_literal(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing array literal\n");
    }
    
    // Consume opening bracket
    if (!advance_token(context)) {
        return false;
    }
    
    // Parse array elements
    int element_count = 0;
    while (context->lexer->token != TOK_RBRACKET && context->lexer->token != TOK_EOF) {
        if (!parse_expression(context)) {
            return false;
        }
        element_count++;
        
        // Check for comma separator
        if (context->lexer->token == TOK_COMMA) {
            if (!advance_token(context)) {
                return false;
            }
        }
    }
    
    // Expect closing bracket
    if (!match_token(context, TOK_RBRACKET)) {
        parser_error(context, "Expected closing bracket");
        return false;
    }
    
    if (!advance_token(context)) {
        return false;
    }
    
    if (debug_mode) {
        printf("Array literal parsed with %d elements\n", element_count);
    }
    
    return true;
}

ast_node_t* parse_number_literal(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing number literal: %lld\n", context->lexer->number);
    }
    
    // Create AST node for number literal
    ast_node_t *node = ast_create_node(AST_LITERAL);
    if (node) {
        ast_set_number(node, context->lexer->number);
        if (debug_mode) {
            printf("Created AST node for number literal: %lld\n", context->lexer->number);
        }
    }
    
    if (!advance_token(context)) {
        if (node) ast_destroy_node(node);
        return NULL;
    }
    
    return node;
}

ast_node_t* parse_string_literal(parser_context_t *context)
{
    printf("PARSE_STRING_LITERAL: Parsing string literal: %.*s\n", (int)context->lexer->toklen, context->lexer->tokstart);
    printf("PARSE_STRING_LITERAL: Token text: '%.*s', length: %d\n", 
           (int)context->lexer->toklen, context->lexer->tokstart, (int)context->lexer->toklen);
    
    // Create AST node for string literal
    ast_node_t *node = ast_create_node(AST_LITERAL);
    if (node) {
        // Store the string literal value
        char *string_value = malloc(context->lexer->toklen + 1);
        if (string_value) {
            strncpy(string_value, context->lexer->tokstart, context->lexer->toklen);
            string_value[context->lexer->toklen] = '\0';
            ast_set_value(node, string_value);
            if (debug_mode) {
                printf("Created AST node for string literal: '%s'\n", string_value);
            }
        }
    }
    
    // Store the string literal for later use
    if (context->current_string_literal) {
        free(context->current_string_literal);
    }
    
    context->current_string_literal = malloc(context->lexer->toklen + 1);
    if (context->current_string_literal) {
        strncpy(context->current_string_literal, context->lexer->tokstart, context->lexer->toklen);
        context->current_string_literal[context->lexer->toklen] = '\0';
    }
    
    // String literals will be collected by codegen during AST traversal
    // No need to collect them here in the parser
    
    if (!advance_token(context)) {
        if (node) ast_destroy_node(node);
        return NULL;
    }
    
    return node;
}

ast_node_t* parse_identifier(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing identifier: %.*s\n", (int)context->lexer->toklen, context->lexer->tokstart);
    }
    
    // Create AST node for identifier
    ast_node_t *node = ast_create_node(AST_IDENTIFIER);
    if (node) {
        // Store the identifier name
        char *ident_name = malloc(context->lexer->toklen + 1);
        if (ident_name) {
            strncpy(ident_name, context->lexer->tokstart, context->lexer->toklen);
            ident_name[context->lexer->toklen] = '\0';
            ast_set_value(node, ident_name);
            if (debug_mode) {
                printf("Created AST node for identifier: '%s'\n", ident_name);
            }
        }
    }
    
    if (!advance_token(context)) {
        if (node) ast_destroy_node(node);
        return NULL;
    }
    
    // Check for postfix operations (dot operator, method calls)
    if (node && node->value) {
        char *base_name = malloc(strlen(node->value) + 1);
        if (base_name) {
            strcpy(base_name, node->value);
            ast_node_t *postfix_result = parse_postfix_operations(context, base_name);
            if (postfix_result) {
                ast_destroy_node(node);
                return postfix_result;
            }
        }
    }
    
    // No postfix operations, just return the identifier node
    return node;
}
ast_node_t* parse_postfix_operations(parser_context_t *context, char *base_name)
{
    if (context == NULL || base_name == NULL) {
        return NULL;
    }
    
    // Check for dot operator
    if (match_token(context, TOK_PERIOD)) {
        return parse_dot_expression(context, base_name);
    }
    
    // Check for method call (parentheses)
    if (match_token(context, TOK_LPAREN)) {
        return parse_method_call_expression(context, base_name, NULL);
    }
    
    // No postfix operations, just a simple identifier
    free(base_name);
    return NULL; // This should be handled by the caller
}

ast_node_t* parse_dot_expression(parser_context_t *context, char *base_name)
{
    if (debug_mode) {
        printf("Parsing dot expression: %s\n", base_name);
    }
    
    // Consume the dot
    if (!advance_token(context)) {
        free(base_name);
        return NULL;
    }
    
    // Expect identifier after dot
    if (!match_token(context, TOK_IDENT)) {
        parser_error(context, "Expected identifier after dot");
        free(base_name);
        return NULL;
    }
    
    // Store the member name
    char *member_name = malloc(context->lexer->toklen + 1);
    if (member_name) {
        strncpy(member_name, context->lexer->tokstart, context->lexer->toklen);
        member_name[context->lexer->toklen] = '\0';
    }
    
    if (!advance_token(context)) {
        free(base_name);
        free(member_name);
        return NULL;
    }
    
    // Check if this is a method call (parentheses)
    if (match_token(context, TOK_LPAREN)) {
        return parse_method_call_expression(context, base_name, member_name);
    } else {
        // Field access
        return parse_field_access_expression(context, base_name, member_name);
    }
}

ast_node_t* parse_method_call_expression(parser_context_t *context, char *base_name, char *member_name)
{
    if (debug_mode) {
        printf("Parsing method call: %s.%s()\n", base_name, member_name ? member_name : "unknown");
    }
    
    // Create method call AST node (temporarily revert to AST_METHOD_CALL)
    ast_node_t *method_call = ast_create_node(AST_METHOD_CALL);
    if (!method_call) {
        free(base_name);
        if (member_name) free(member_name);
        return NULL;
    }
    
    // Set the method call info
    size_t base_len = strlen(base_name);
    size_t member_len = member_name ? strlen(member_name) : 0;
    char *call_info = malloc(base_len + member_len + 3);
    if (call_info) {
        snprintf(call_info, base_len + member_len + 3, "%s.%s", base_name, member_name ? member_name : "");
        ast_set_value(method_call, call_info);
        free(call_info);
    }
    
    // Consume opening parenthesis
    if (!advance_token(context)) {
        free(base_name);
        if (member_name) free(member_name);
        ast_destroy_node(method_call);
        return NULL;
    }
    
    // Parse parameters
    int param_count = 0;
    while (context->lexer->token != TOK_RPAREN && context->lexer->token != TOK_EOF) {
        ast_node_t *param = parse_expression(context);
        if (param) {
            ast_add_child(method_call, param);
            param_count++;
        }
        
        // Check for comma separator
        if (context->lexer->token == TOK_COMMA) {
            if (!advance_token(context)) {
                free(base_name);
                if (member_name) free(member_name);
                ast_destroy_node(method_call);
                return NULL;
            }
        }
    }
    
    // Expect closing parenthesis
    if (!match_token(context, TOK_RPAREN)) {
        parser_error(context, "Expected closing parenthesis");
        free(base_name);
        if (member_name) free(member_name);
        ast_destroy_node(method_call);
        return NULL;
    }
    
    if (!advance_token(context)) {
        free(base_name);
        if (member_name) free(member_name);
        ast_destroy_node(method_call);
        return NULL;
    }
    
    if (debug_mode) {
        printf("Method call parsed: %s.%s() with %d parameters\n", 
               base_name, member_name ? member_name : "unknown", param_count);
    }
    
    free(base_name);
    if (member_name) free(member_name);
    return method_call;
}

ast_node_t* parse_field_access_expression(parser_context_t *context, char *base_name, char *member_name)
{
    if (debug_mode) {
        printf("Parsing field access: %s.%s\n", base_name, member_name);
    }
    
    // ENCAPSULATION ENFORCEMENT: Direct field access is not allowed
    // Fields can only be accessed through methods (getters/setters)
    parser_error(context, "Direct field access not allowed - use methods instead (e.g., obj.getField())");
    
    if (debug_mode) {
        printf("Field access rejected due to encapsulation: %s.%s\n", base_name, member_name);
    }
    
    free(base_name);
    free(member_name);
    return NULL;
}


bool parse_constructor_parameters(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing constructor parameters\n");
    }
    
    // Consume opening parenthesis
    if (!advance_token(context)) {
        return false;
    }
    
    // Parse parameters
    int param_count = 0;
    while (context->lexer->token != TOK_RPAREN && context->lexer->token != TOK_EOF) {
        if (!parse_expression(context)) {
            return false;
        }
        param_count++;
        
        // Check for comma separator
        if (context->lexer->token == TOK_COMMA) {
            if (!advance_token(context)) {
                return false;
            }
        }
    }
    
    // Expect closing parenthesis
    if (!match_token(context, TOK_RPAREN)) {
        parser_error(context, "Expected closing parenthesis in constructor call");
        return false;
    }
    
    if (!advance_token(context)) {
        return false;
    }
    
    // Store constructor parameter information
    context->constructor_param_count = param_count;
    context->has_constructor_params = true;
    
    if (debug_mode) {
        printf("Constructor parameters parsed: %d parameters\n", param_count);
    }
    
    return true;
}

bool parse_parenthesized_expression(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing parenthesized expression\n");
    }
    
    // Consume opening parenthesis
    if (!advance_token(context)) {
        return false;
    }
    
    // Parse inner expression
    if (!parse_expression(context)) {
        return false;
    }
    
    // Expect closing parenthesis
    if (!match_token(context, TOK_RPAREN)) {
        parser_error(context, "Expected closing parenthesis");
        return false;
    }
    
    // Consume closing parenthesis
    if (!advance_token(context)) {
        return false;
    }
    
    return true;
}

ast_node_t* parse_new_expression(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing NEW expression\n");
    }
    
    // Expect NEW token
    if (!expect_token(context, TOK_NEW)) {
        return NULL;
    }
    
    // Expect class name identifier
    if (context->lexer->token != TOK_IDENT) {
        parser_error(context, "Expected class name after NEW");
        return NULL;
    }
    
    // Create NEW expression node
    ast_node_t *new_node = ast_create_node(AST_NEW_EXPR);
    if (!new_node) {
        parser_error(context, "Failed to create NEW expression node");
        return NULL;
    }
    
    // Store class name
    char *class_name = malloc(context->lexer->toklen + 1);
    if (!class_name) {
        ast_destroy_node(new_node);
        parser_error(context, "Failed to allocate memory for class name");
        return NULL;
    }
    strncpy(class_name, context->lexer->tokstart, context->lexer->toklen);
    class_name[context->lexer->toklen] = '\0';
    
    new_node->value = class_name;
    
    if (debug_mode) {
        printf("NEW expression parsed: %s\n", class_name);
    }
    
    // Advance past class name
    if (!advance_token(context)) {
        ast_destroy_node(new_node);
        return NULL;
    }
    
    return new_node;
}

