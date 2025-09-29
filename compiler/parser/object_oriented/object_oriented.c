/*
 * ARX Object-Oriented Implementation
 * Handles object-oriented parsing including classes, methods, and fields
 */

#include "object_oriented.h"
#include "../core/parser_core.h"
#include "../statements/statements.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External debug flag
extern bool debug_mode;

ast_node_t* parse_class(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing class\n");
    }
    
    // Create class node
    ast_node_t *class_node = ast_create_node(AST_CLASS);
    if (class_node == NULL) {
        parser_error(context, "Failed to create class node");
        return false;
    }
    
    // Parse class keyword
    if (!expect_token(context, TOK_CLASS)) {
        return NULL;
    }
    
    // Parse class name
    if (!match_token(context, TOK_IDENT)) {
        parser_error(context, "Expected class name");
        return NULL;
    }
    
    // Store class name (before advancing token)
    char *class_name = malloc(context->lexer->toklen + 1);
    if (class_name) {
        strncpy(class_name, context->lexer->tokstart, context->lexer->toklen);
        class_name[context->lexer->toklen] = '\0';
    }
    
    ast_set_value(class_node, class_name);
    ast_set_number(class_node, context->lexer->toklen);
    
    // Advance token
    if (!advance_token(context)) {
        free(class_name);
        return NULL;
    }
    
    // Check for inheritance (extends keyword)
    char *parent_class = NULL;
    if (match_token(context, TOK_EXTENDS)) {
        if (!advance_token(context)) {
            free(class_name);
            return NULL;
        }
        
        // Parse parent class name
        if (!match_token(context, TOK_IDENT)) {
            parser_error(context, "Expected parent class name after extends");
            free(class_name);
            return NULL;
        }
        
        // Store parent class name
        parent_class = malloc(context->lexer->toklen + 1);
        if (parent_class) {
            strncpy(parent_class, context->lexer->tokstart, context->lexer->toklen);
            parent_class[context->lexer->toklen] = '\0';
        }
        
        if (debug_mode) {
            printf("Class %s extends %s\n", class_name, parent_class ? parent_class : "NULL");
        }
        
        // Advance token
        if (!advance_token(context)) {
            free(class_name);
            free(parent_class);
            return NULL;
        }
    }
    
    // Parse class body (optional BEGIN keyword)
    if (context->lexer->token == TOK_BEGIN) {
        if (!advance_token(context)) {
            return NULL;
        }
    }
    
    // Parse fields and methods
    while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
        if (debug_mode) {
            printf("DEBUG: Class parsing - token: %s\n", token_to_string(context->lexer->token));
        }
        
        if (context->lexer->token == TOK_STRING || 
            context->lexer->token == TOK_INTEGER || 
            context->lexer->token == TOK_BOOLEAN || 
            context->lexer->token == TOK_CHAR || 
            context->lexer->token == TOK_IDENT ||
            context->lexer->token == TOK_ARRAY) {
            // Field declaration (C-style: type name;)
            if (debug_mode) {
                printf("DEBUG: Parsing field declaration\n");
            }
            ast_node_t *field = parse_field(context);
            if (field == NULL) {
                return NULL;
            }
            ast_add_child(class_node, field);
        } else if (context->lexer->token == TOK_PROCEDURE || context->lexer->token == TOK_FUNCTION) {
            // Method declaration
            if (debug_mode) {
                printf("DEBUG: Parsing method declaration - token: %s\n", token_to_string(context->lexer->token));
            }
            ast_node_t *method = parse_method(context);
            if (method == NULL) {
                return NULL;
            }
            ast_add_child(class_node, method);
            if (debug_mode) {
                printf("DEBUG: Added method to class: %s (type=%d)\n", method->value ? method->value : "unknown", method->type);
            }
        } else if (context->lexer->token == TOK_SEMICOL) {
            // Skip empty statements
            if (!advance_token(context)) {
                return NULL;
            }
        } else {
            if (debug_mode) {
                printf("DEBUG: Unexpected token in class body: %s\n", token_to_string(context->lexer->token));
            }
            parser_error(context, "Unexpected token in class body");
            return NULL;
        }
    }
    
    if (!expect_token(context, TOK_END)) {
        return NULL;
    }
    
    if (!expect_token(context, TOK_SEMICOL)) {
        free(class_name);
        return NULL;
    }
    
    // Count fields and methods
    size_t field_count = 0;
    size_t method_count = 0;
    for (size_t i = 0; i < class_node->child_count; i++) {
        if (class_node->children[i]->type == AST_FIELD) {
            field_count++;
        } else if (class_node->children[i]->type == AST_METHOD || 
                   class_node->children[i]->type == AST_PROCEDURE ||
                   class_node->children[i]->type == AST_FUNCTION) {
            method_count++;
        }
    }
    
    // Add class to symbol table
    symbol_t *class_symbol = symbol_create_class(class_name, strlen(class_name), field_count, method_count, parent_class);
    if (class_symbol) {
        class_symbol->data.class_info.instance_size = field_count * 8; // 8 bytes per field for now
        if (!symbol_add(&context->symbol_table, class_symbol)) {
            symbol_destroy(class_symbol);
            if (debug_mode) {
                printf("Warning: Failed to add class '%s' to symbol table\n", class_name);
            }
        } else {
            if (debug_mode) {
                printf("Added class '%s' to symbol table (fields: %zu, methods: %zu, parent: %s)\n", 
                        class_name, field_count, method_count, parent_class ? parent_class : "none");
            }
        }
    }
    
    if (debug_mode) {
        printf("Class parsed successfully\n");
    }
    
    free(class_name);
    if (parent_class) {
        free(parent_class);
    }
    return class_node;
}

ast_node_t* parse_field(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing field\n");
    }
    
    // Create field node
    ast_node_t *field = ast_create_node(AST_FIELD);
    if (field == NULL) {
        parser_error(context, "Failed to create field node");
        return NULL;
    }
    
    // Parse type first (C-style: type name;)
    if (!match_token(context, TOK_IDENT) && 
        !match_token(context, TOK_INTEGER) && 
        !match_token(context, TOK_BOOLEAN) && 
        !match_token(context, TOK_CHAR) && 
        !match_token(context, TOK_STRING) && 
        !match_token(context, TOK_ARRAY)) {
        parser_error(context, "Expected type");
        return NULL;
    }
    
    // Store type name before advancing token
    char *type_name = malloc(context->lexer->toklen + 1);
    if (type_name == NULL) {
        parser_error(context, "Failed to allocate memory for type name");
        return NULL;
    }
    strncpy(type_name, context->lexer->tokstart, context->lexer->toklen);
    type_name[context->lexer->toklen] = '\0';
    
    if (debug_mode) {
        printf("DEBUG: Parsing field type: %s\n", type_name);
    }
    
    // Advance token after capturing type
    if (!advance_token(context)) {
        free(type_name);
        return NULL;
    }
    
    // Parse field name
    if (context->lexer->token != TOK_IDENT) {
        parser_error(context, "Expected field name");
        free(type_name);
        return NULL;
    }
    
    // Store field name before advancing token
    char *field_name = malloc(context->lexer->toklen + 1);
    if (field_name == NULL) {
        parser_error(context, "Failed to allocate memory for field name");
        free(type_name);
        return NULL;
    }
    strncpy(field_name, context->lexer->tokstart, context->lexer->toklen);
    field_name[context->lexer->toklen] = '\0';
    
    if (debug_mode) {
        printf("DEBUG: Parsing field: %s %s\n", type_name, field_name);
    }
    
    ast_set_value(field, field_name);
    ast_set_number(field, context->lexer->toklen);
    
    free(field_name);
    
    // Advance token after capturing field name
    if (!advance_token(context)) {
        free(type_name);
        return NULL;
    }
    
    // Create type node
    ast_node_t *type_node = ast_create_node(AST_IDENTIFIER);
    if (type_node == NULL) {
        parser_error(context, "Failed to create type node");
        free(type_name);
        return NULL;
    }
    
    ast_set_value(type_node, type_name);
    ast_set_number(type_node, strlen(type_name));
    ast_add_child(field, type_node);
    
    free(type_name);
    
    if (!expect_token(context, TOK_SEMICOL)) {
        return NULL;
    }
    
    if (debug_mode) {
        printf("Field parsed successfully\n");
    }
    
    return field;
}

ast_node_t* parse_parameter(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing parameter\n");
    }
    
    // Create parameter node
    ast_node_t *param = ast_create_node(AST_PARAMETER);
    if (param == NULL) {
        parser_error(context, "Failed to create parameter node");
        return NULL;
    }
    
    // Parse parameter type
    if (context->lexer->token != TOK_IDENT && 
        context->lexer->token != TOK_INTEGER && 
        context->lexer->token != TOK_BOOLEAN && 
        context->lexer->token != TOK_CHAR && 
        context->lexer->token != TOK_STRING && 
        context->lexer->token != TOK_ARRAY) {
        parser_error(context, "Expected parameter type");
        ast_destroy_node(param);
        return NULL;
    }
    
    // Store parameter type
    char *type_name = malloc(context->lexer->toklen + 1);
    if (type_name == NULL) {
        parser_error(context, "Failed to allocate memory for parameter type");
        ast_destroy_node(param);
        return NULL;
    }
    strncpy(type_name, context->lexer->tokstart, context->lexer->toklen);
    type_name[context->lexer->toklen] = '\0';
    
    // Set type as first child
    ast_node_t *type_node = ast_create_node(AST_IDENTIFIER);
    if (type_node == NULL) {
        free(type_name);
        ast_destroy_node(param);
        return NULL;
    }
    ast_set_value(type_node, type_name);
    ast_add_child(param, type_node);
    free(type_name);
    
    if (!advance_token(context)) {
        ast_destroy_node(param);
        return NULL;
    }
    
    // Parse parameter name
    if (context->lexer->token != TOK_IDENT) {
        parser_error(context, "Expected parameter name");
        ast_destroy_node(param);
        return NULL;
    }
    
    // Store parameter name
    char *param_name = malloc(context->lexer->toklen + 1);
    if (param_name == NULL) {
        parser_error(context, "Failed to allocate memory for parameter name");
        ast_destroy_node(param);
        return NULL;
    }
    strncpy(param_name, context->lexer->tokstart, context->lexer->toklen);
    param_name[context->lexer->toklen] = '\0';
    
    // Set parameter name as value
    ast_set_value(param, param_name);
    free(param_name);
    
    if (!advance_token(context)) {
        ast_destroy_node(param);
        return NULL;
    }
    
    if (debug_mode) {
        printf("Parameter parsed: %s %s\n", type_name, param_name);
    }
    
    return param;
}

ast_node_t* parse_method(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing method\n");
    }
    
    // Create method node
    ast_node_t *method = ast_create_node(AST_METHOD);
    if (method == NULL) {
        parser_error(context, "Failed to create method node");
        return false;
    }
    
    // Parse method type - initially set as PROCEDURE, will be changed to FUNCTION if return type is found
    if (context->lexer->token == TOK_PROCEDURE) {
        method->type = AST_PROCEDURE;
    } else if (context->lexer->token == TOK_FUNCTION) {
        method->type = AST_FUNCTION;
    } else {
        parser_error(context, "Expected PROCEDURE or FUNCTION");
        return false;
    }
    
    if (!advance_token(context)) {
        return false;
    }
    
    // Parse method name
    if (context->lexer->token != TOK_IDENT) {
        parser_error(context, "Expected method name");
        return NULL;
    }
    
    // Don't clear method string literals - they should be collected globally
    // parser_clear_method_strings(context);
    
    // Create null-terminated string for method name
    char *method_name = malloc(context->lexer->toklen + 1);
    if (method_name == NULL) {
        parser_error(context, "Failed to allocate memory for method name");
        return NULL;
    }
    strncpy(method_name, context->lexer->tokstart, context->lexer->toklen);
    method_name[context->lexer->toklen] = '\0';
    
    ast_set_value(method, method_name);
    ast_set_number(method, context->lexer->toklen);
    
    free(method_name);
    
    // Advance to next token
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Parse parameters
    if (context->lexer->token == TOK_LPAREN) {
        if (!advance_token(context)) {
            return false;
        }
        
        // Parse parameter list
        while (context->lexer->token != TOK_RPAREN && context->lexer->token != TOK_EOF) {
            ast_node_t *param = parse_parameter(context);
            if (param == NULL) {
                return NULL;
            }
            ast_add_child(method, param);
            
            // Check for comma separator
            if (context->lexer->token == TOK_COMMA) {
                if (!advance_token(context)) {
                    return false;
                }
            }
        }
        
        if (!expect_token(context, TOK_RPAREN)) {
            return false;
        }
    }
    
    // Check for return type - if found, change method type to FUNCTION
    if (context->lexer->token == TOK_COLON) {
        // This is a procedure with return type, so it's actually a function
        method->type = AST_FUNCTION;
        
        if (!advance_token(context)) {
            return false;
        }
        
        // Expect return type (can be identifier or keyword)
        if (!match_token(context, TOK_IDENT) && 
            !match_token(context, TOK_INTEGER) && 
            !match_token(context, TOK_BOOLEAN) && 
            !match_token(context, TOK_CHAR) && 
            !match_token(context, TOK_STRING) && 
            !match_token(context, TOK_ARRAY)) {
            parser_error(context, "Expected return type");
            return false;
        }
        
        if (!advance_token(context)) {
            return false;
        }
    }
    
    // Parse method body
    if (context->lexer->token == TOK_BEGIN) {
        if (!advance_token(context)) {
            return NULL;
        }
        
        if (debug_mode) {
            printf("Found method body with BEGIN/END, parsing statements\n");
        }
        
        // Parse method body statements until we find END
        int brace_count = 1;
        while (brace_count > 0 && context->lexer->token != TOK_EOF) {
            if (context->lexer->token == TOK_BEGIN) {
                brace_count++;
                if (!advance_token(context)) {
                    return NULL;
                }
            } else if (context->lexer->token == TOK_END) {
                brace_count--;
                if (brace_count == 0) {
                    // Found the matching END, advance past it and break
                    if (!advance_token(context)) {
                        return NULL;
                    }
                    break;
                }
                if (!advance_token(context)) {
                    return NULL;
                }
            } else {
                // Parse statement and add to method AST
                if (debug_mode) {
                    printf("DEBUG: About to call parse_statement_ast with token: %s\n", token_to_string(context->lexer->token));
                }
                ast_node_t *stmt_node = parse_statement_ast(context);
                if (debug_mode) {
                    printf("DEBUG: parse_statement_ast returned: %p\n", stmt_node);
                }
                if (stmt_node) {
                    ast_add_child(method, stmt_node);
                } else {
                    // If no statement node was created, just continue
                    // This handles cases where tokens are skipped
                }
                
                // Skip semicolon after statement
                if (context->lexer->token == TOK_SEMICOL) {
                    if (!advance_token(context)) {
                        return NULL;
                    }
                }
            }
        }
        
        // Skip trailing semicolon after method body if present
        if (context->lexer->token == TOK_SEMICOL) {
            if (!advance_token(context)) {
                return NULL;
            }
        }
    } else if (context->lexer->token == TOK_SEMICOL) {
        // Method declaration without body (just skip the semicolon)
        if (!advance_token(context)) {
            return NULL;
        }
        
        // For methods without BEGIN/END, we should NOT parse a body here
        // The method declaration is complete after the semicolon
        if (debug_mode) {
            printf("Method declaration without body completed\n");
        }
    }
    
    if (debug_mode) {
        printf("Method parsed successfully: %s (type=%d)\n", method->value ? method->value : "unknown", method->type);
        printf("DEBUG: After method parsing, current token: %s\n", token_to_string(context->lexer->token));
    }
    
    return method;
}
