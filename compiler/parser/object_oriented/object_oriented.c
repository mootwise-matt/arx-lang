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
            printf("DEBUG: Class parsing - token: %s (%d)\n", token_to_string(context->lexer->token), context->lexer->token);
        }
        
        if (context->lexer->token == TOK_STRING || 
            context->lexer->token == TOK_INTEGER || 
            context->lexer->token == TOK_BOOLEAN || 
            context->lexer->token == TOK_CHAR || 
            context->lexer->token == TOK_IDENT ||
            context->lexer->token == TOK_ARRAY) {
            // Object variable declaration (C-style: type name;)
            if (debug_mode) {
                printf("DEBUG: Parsing object variable declaration\n");
            }
            ast_node_t *object_var = parse_object_variable(context);
            if (object_var == NULL) {
                return NULL;
            }
            ast_add_child(class_node, object_var);
        } else if (context->lexer->token == TOK_PROCEDURE) {
            // Procedure declaration
            if (debug_mode) {
                printf("DEBUG: Parsing procedure declaration - token: %s\n", token_to_string(context->lexer->token));
            }
            ast_node_t *procedure = parse_procedure(context);
            if (procedure == NULL) {
                return NULL;
            }
            ast_add_child(class_node, procedure);
            if (debug_mode) {
                printf("DEBUG: Added PROCEDURE to class: %s (type=%d)\n", procedure->value ? procedure->value : "unknown", procedure->type);
            }
        } else if (context->lexer->token == TOK_FUNCTION) {
            // Function declaration
            if (debug_mode) {
                printf("DEBUG: Parsing function declaration - token: %s\n", token_to_string(context->lexer->token));
            }
            ast_node_t *function = parse_function(context);
            if (function == NULL) {
                return NULL;
            }
            ast_add_child(class_node, function);
            if (debug_mode) {
                printf("DEBUG: Added FUNCTION to class: %s (type=%d)\n", function->value ? function->value : "unknown", function->type);
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
        if (class_node->children[i]->type == AST_OBJECT_VAR) {
            field_count++;
        } else if (class_node->children[i]->type == AST_PROCEDURE ||
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

ast_node_t* parse_object_variable(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing object variable\n");
    }
    
    // Create object variable node
    ast_node_t *object_var = ast_create_node(AST_OBJECT_VAR);
    if (object_var == NULL) {
        parser_error(context, "Failed to create object variable node");
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
    
    // Parse variable name
    if (context->lexer->token != TOK_IDENT) {
        parser_error(context, "Expected object variable name");
        free(type_name);
        return NULL;
    }
    
    // Store variable name before advancing token
    char *var_name = malloc(context->lexer->toklen + 1);
    if (var_name == NULL) {
        parser_error(context, "Failed to allocate memory for variable name");
        free(type_name);
        return NULL;
    }
    strncpy(var_name, context->lexer->tokstart, context->lexer->toklen);
    var_name[context->lexer->toklen] = '\0';
    
    if (debug_mode) {
        printf("DEBUG: Parsing object variable: %s %s\n", type_name, var_name);
    }
    
    ast_set_value(object_var, var_name);
    ast_set_number(object_var, context->lexer->toklen);
    
    free(var_name);
    
    // Advance token after capturing variable name
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
    ast_add_child(object_var, type_node);
    
    free(type_name);
    
    if (!expect_token(context, TOK_SEMICOL)) {
        return NULL;
    }
    
    if (debug_mode) {
        printf("Object variable parsed successfully\n");
    }
    
    return object_var;
}

ast_node_t* parse_procedure(parser_context_t *context)
{
    if (debug_mode) {
        printf("DEBUG: parse_procedure called - current token: %s (%d)\n", 
               token_to_string(context->lexer->token), context->lexer->token);
    }
    
    // Create procedure node
    ast_node_t *procedure = ast_create_node(AST_PROCEDURE);
    if (procedure == NULL) {
        parser_error(context, "Failed to create procedure node");
        return NULL;
    }
    
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Parse procedure name
    if (context->lexer->token != TOK_IDENT) {
        parser_error(context, "Expected procedure name");
        return NULL;
    }
    
    // Create null-terminated string for procedure name
    char *procedure_name = malloc(context->lexer->toklen + 1);
    if (procedure_name == NULL) {
        parser_error(context, "Failed to allocate memory for procedure name");
        return NULL;
    }
    strncpy(procedure_name, context->lexer->tokstart, context->lexer->toklen);
    procedure_name[context->lexer->toklen] = '\0';
    
    ast_set_value(procedure, procedure_name);
    ast_set_number(procedure, context->lexer->toklen);
    
    free(procedure_name);
    
    // Advance to next token
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Parse parameters and collect parameter types for signature
    char *param_types = NULL;
    if (context->lexer->token == TOK_LPAREN) {
        if (!advance_token(context)) {
            return NULL;
        }
        
        // Collect parameter types for signature
        size_t param_count = 0;
        size_t param_types_len = 0;
        size_t param_types_capacity = 64;
        param_types = malloc(param_types_capacity);
        if (!param_types) {
            parser_error(context, "Failed to allocate memory for parameter types");
            return false;
        }
        param_types[0] = '\0';
        
        while (context->lexer->token != TOK_RPAREN && context->lexer->token != TOK_EOF) {
            // Parse parameter type
            if (context->lexer->token == TOK_IDENT || 
                context->lexer->token == TOK_INTEGER || 
                context->lexer->token == TOK_BOOLEAN || 
                context->lexer->token == TOK_CHAR || 
                context->lexer->token == TOK_STRING || 
                context->lexer->token == TOK_ARRAY) {
                
                // Add comma separator if not first parameter
                if (param_count > 0) {
                    if (param_types_len + 1 >= param_types_capacity) {
                        param_types_capacity *= 2;
                        char *new_param_types = realloc(param_types, param_types_capacity);
                        if (!new_param_types) {
                            free(param_types);
                            parser_error(context, "Failed to reallocate memory for parameter types");
                            return false;
                        }
                        param_types = new_param_types;
                    }
                    param_types[param_types_len++] = ',';
                    param_types[param_types_len] = '\0';
                }
                
                // Add parameter type
                size_t type_len = context->lexer->toklen;
                if (param_types_len + type_len >= param_types_capacity) {
                    param_types_capacity = param_types_len + type_len + 16;
                    char *new_param_types = realloc(param_types, param_types_capacity);
                    if (!new_param_types) {
                        free(param_types);
                        parser_error(context, "Failed to reallocate memory for parameter types");
                        return false;
                    }
                    param_types = new_param_types;
                }
                
                strncpy(param_types + param_types_len, context->lexer->tokstart, type_len);
                param_types_len += type_len;
                param_types[param_types_len] = '\0';
                
                param_count++;
                
                if (!advance_token(context)) {
                    free(param_types);
                    return false;
                }
                
                // Skip parameter name
                if (context->lexer->token == TOK_IDENT) {
                    if (!advance_token(context)) {
                        free(param_types);
                        return false;
                    }
                }
                
                // Skip comma if present
                if (context->lexer->token == TOK_COMMA) {
                    if (!advance_token(context)) {
                        free(param_types);
                        return false;
                    }
                }
            } else {
                if (!advance_token(context)) {
                    free(param_types);
                    return false;
                }
            }
        }
        
        if (!expect_token(context, TOK_RPAREN)) {
            free(param_types);
            return false;
        }
    }
    
    // Procedures don't have return types
    
    // Parse procedure body
    printf("PARSE_PROCEDURE: Looking for procedure body, current token: %s (%d)\n", 
           token_to_string(context->lexer->token), context->lexer->token);
    printf("PARSE_PROCEDURE: TOK_BEGIN value: %d\n", TOK_BEGIN);
    printf("PARSE_PROCEDURE: Current lexer position: %zu\n", context->lexer->pos);
    
    if (context->lexer->token == TOK_BEGIN) {
        printf("PARSE_PROCEDURE: Found BEGIN token, entering procedure body\n");
        if (!advance_token(context)) {
            printf("PARSE_PROCEDURE: Failed to advance past BEGIN token\n");
            return NULL;
        }
        printf("PARSE_PROCEDURE: Advanced past BEGIN, current token: %s (%d)\n", 
               token_to_string(context->lexer->token), context->lexer->token);
        
        // Parse procedure body statements until we find END
        int brace_count = 1;
        printf("PARSE_PROCEDURE: Starting statement parsing loop with brace_count=%d\n", brace_count);
        while (brace_count > 0 && context->lexer->token != TOK_EOF) {
            printf("PARSE_PROCEDURE: In loop, current token: %s (%d), brace_count: %d\n", 
                   token_to_string(context->lexer->token), context->lexer->token, brace_count);
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
                // Parse statement and add to procedure AST
                printf("PARSE_PROCEDURE: About to call parse_statement_ast with token: %s\n", token_to_string(context->lexer->token));
                ast_node_t *stmt_node = parse_statement_ast(context);
                printf("PARSE_PROCEDURE: parse_statement_ast returned: %p\n", stmt_node);
                if (stmt_node) {
                    ast_add_child(procedure, stmt_node);
                    printf("PARSE_PROCEDURE: Added statement node to procedure\n");
                } else {
                    // If no statement node was created, advance token to avoid infinite loop
                    printf("PARSE_PROCEDURE: No statement node created, advancing token to avoid infinite loop\n");
                    if (!advance_token(context)) {
                        return NULL;
                    }
                }
            }
        }
        
        // Skip trailing semicolon after procedure/function body if present
        if (context->lexer->token == TOK_SEMICOL) {
            if (!advance_token(context)) {
                return NULL;
            }
        }
    } else if (context->lexer->token == TOK_SEMICOL) {
        // Procedure/function declaration without body (just skip the semicolon)
        if (!advance_token(context)) {
            return NULL;
        }
        
        // For procedures/functions without BEGIN/END, we should NOT parse a body here
        // The procedure/function declaration is complete after the semicolon
        if (debug_mode) {
            printf("Procedure/function declaration without body completed - current token: %s (%d)\n", 
                   token_to_string(context->lexer->token), context->lexer->token);
        }
    }
    
    // Store signature information in the procedure node for ID generation
    // Store parameter types as a child node
    if (param_types) {
        ast_node_t *param_node = ast_create_node(AST_IDENTIFIER);
        if (param_node) {
            ast_set_value(param_node, param_types);
            ast_add_child(procedure, param_node);
        }
        free(param_types);
    }
    
    if (debug_mode) {
        printf("DEBUG: PROCEDURE parsed successfully: %s (type=%d)\n", procedure->value ? procedure->value : "unknown", procedure->type);
        printf("DEBUG: After procedure parsing, current token: %s\n", token_to_string(context->lexer->token));
    }
    
    return procedure;
}

ast_node_t* parse_function(parser_context_t *context)
{
    if (debug_mode) {
        printf("DEBUG: parse_function called - current token: %s (%d)\n", 
               token_to_string(context->lexer->token), context->lexer->token);
    }
    
    // Create function node
    ast_node_t *function = ast_create_node(AST_FUNCTION);
    if (function == NULL) {
        parser_error(context, "Failed to create function node");
        return NULL;
    }
    
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Parse function name
    if (context->lexer->token != TOK_IDENT) {
        parser_error(context, "Expected function name");
        return NULL;
    }
    
    // Create null-terminated string for function name
    char *function_name = malloc(context->lexer->toklen + 1);
    if (function_name == NULL) {
        parser_error(context, "Failed to allocate memory for function name");
        return NULL;
    }
    strncpy(function_name, context->lexer->tokstart, context->lexer->toklen);
    function_name[context->lexer->toklen] = '\0';
    
    ast_set_value(function, function_name);
    ast_set_number(function, context->lexer->toklen);
    
    free(function_name);
    
    // Advance to next token
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Parse parameters and collect parameter types for signature
    char *param_types = NULL;
    if (context->lexer->token == TOK_LPAREN) {
        if (!advance_token(context)) {
            return NULL;
        }
        
        // Collect parameter types for signature
        size_t param_count = 0;
        size_t param_types_len = 0;
        size_t param_types_capacity = 64;
        param_types = malloc(param_types_capacity);
        if (!param_types) {
            parser_error(context, "Failed to allocate memory for parameter types");
            return NULL;
        }
        param_types[0] = '\0';
        
        while (context->lexer->token != TOK_RPAREN && context->lexer->token != TOK_EOF) {
            // Parse parameter type
            if (context->lexer->token == TOK_IDENT || 
                context->lexer->token == TOK_INTEGER || 
                context->lexer->token == TOK_BOOLEAN || 
                context->lexer->token == TOK_CHAR || 
                context->lexer->token == TOK_STRING || 
                context->lexer->token == TOK_ARRAY) {
                
                // Add comma separator if not first parameter
                if (param_count > 0) {
                    if (param_types_len + 1 >= param_types_capacity) {
                        param_types_capacity *= 2;
                        char *new_param_types = realloc(param_types, param_types_capacity);
                        if (!new_param_types) {
                            free(param_types);
                            parser_error(context, "Failed to reallocate memory for parameter types");
                            return NULL;
                        }
                        param_types = new_param_types;
                    }
                    param_types[param_types_len++] = ',';
                    param_types[param_types_len] = '\0';
                }
                
                // Add parameter type
                size_t type_len = context->lexer->toklen;
                if (param_types_len + type_len >= param_types_capacity) {
                    param_types_capacity = param_types_len + type_len + 16;
                    char *new_param_types = realloc(param_types, param_types_capacity);
                    if (!new_param_types) {
                        free(param_types);
                        parser_error(context, "Failed to reallocate memory for parameter types");
                        return NULL;
                    }
                    param_types = new_param_types;
                }
                
                strncpy(param_types + param_types_len, context->lexer->tokstart, type_len);
                param_types_len += type_len;
                param_types[param_types_len] = '\0';
                
                param_count++;
                
                if (!advance_token(context)) {
                    free(param_types);
                    return NULL;
                }
                
                // Skip parameter name
                if (context->lexer->token == TOK_IDENT) {
                    if (!advance_token(context)) {
                        free(param_types);
                        return NULL;
                    }
                }
                
                // Skip comma if present
                if (context->lexer->token == TOK_COMMA) {
                    if (!advance_token(context)) {
                        free(param_types);
                        return NULL;
                    }
                }
            } else {
                if (!advance_token(context)) {
                    free(param_types);
                    return NULL;
                }
            }
        }
        
        if (!expect_token(context, TOK_RPAREN)) {
            free(param_types);
            return NULL;
        }
    }
    
    // Check for return type - functions must have return types
    char *return_type = NULL;
    if (context->lexer->token == TOK_COLON) {
        if (!advance_token(context)) {
            free(param_types);
            return NULL;
        }
        
        // Expect return type (can be identifier or keyword)
        if (!match_token(context, TOK_IDENT) && 
            !match_token(context, TOK_INTEGER) && 
            !match_token(context, TOK_BOOLEAN) && 
            !match_token(context, TOK_CHAR) && 
            !match_token(context, TOK_STRING) && 
            !match_token(context, TOK_ARRAY)) {
            parser_error(context, "Expected return type");
            free(param_types);
            return NULL;
        }
        
        // Collect return type
        size_t return_type_len = context->lexer->toklen;
        return_type = malloc(return_type_len + 1);
        if (!return_type) {
            parser_error(context, "Failed to allocate memory for return type");
            free(param_types);
            return NULL;
        }
        strncpy(return_type, context->lexer->tokstart, return_type_len);
        return_type[return_type_len] = '\0';
        
        if (!advance_token(context)) {
            free(param_types);
            free(return_type);
            return NULL;
        }
    } else {
        parser_error(context, "Functions must have a return type");
        free(param_types);
        return NULL;
    }
    
    // Parse function body
    if (debug_mode) {
        printf("DEBUG: Looking for function body, current token: %s (%d)\n", 
               token_to_string(context->lexer->token), context->lexer->token);
        printf("DEBUG: TOK_BEGIN value: %d\n", TOK_BEGIN);
        printf("DEBUG: Current lexer position: %zu\n", context->lexer->pos);
    }
    
    if (context->lexer->token == TOK_BEGIN) {
        if (debug_mode) {
            printf("Found function body with BEGIN/END, parsing statements\n");
        }
        if (!advance_token(context)) {
            return NULL;
        }
        
        // Parse function body statements until we find END
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
                // Parse statement and add to function AST
                if (debug_mode) {
                    printf("DEBUG: About to call parse_statement_ast with token: %s\n", token_to_string(context->lexer->token));
                }
                ast_node_t *stmt_node = parse_statement_ast(context);
                if (debug_mode) {
                    printf("DEBUG: parse_statement_ast returned: %p\n", stmt_node);
                }
                if (stmt_node) {
                    ast_add_child(function, stmt_node);
                } else {
                    // If no statement node was created, advance token to avoid infinite loop
                    if (debug_mode) {
                        printf("DEBUG: No statement node created, advancing token to avoid infinite loop\n");
                    }
                    if (!advance_token(context)) {
                        return NULL;
                    }
                }
            }
        }
        
        // Skip trailing semicolon after function body if present
        if (context->lexer->token == TOK_SEMICOL) {
            if (!advance_token(context)) {
                return NULL;
            }
        }
    } else {
        // Function declaration without body (just skip the semicolon)
        if (!advance_token(context)) {
            return NULL;
        }
        
        // For functions without BEGIN/END, we should NOT parse a body here
        // The function declaration is complete after the semicolon
        if (debug_mode) {
            printf("Function declaration without body completed - current token: %s (%d)\n", 
                   token_to_string(context->lexer->token), context->lexer->token);
        }
    }
    
    // Store signature information in the function node for ID generation
    // Store parameter types as a child node
    if (param_types) {
        ast_node_t *param_node = ast_create_node(AST_IDENTIFIER);
        if (param_node) {
            ast_set_value(param_node, param_types);
            ast_add_child(function, param_node);
        }
        free(param_types);
    }
    
    // Store return type as a child node
    if (return_type) {
        ast_node_t *return_node = ast_create_node(AST_IDENTIFIER);
        if (return_node) {
            ast_set_value(return_node, return_type);
            ast_add_child(function, return_node);
        }
        free(return_type);
    }
    
    if (debug_mode) {
        printf("DEBUG: FUNCTION parsed successfully: %s (type=%d)\n", function->value ? function->value : "unknown", function->type);
        printf("DEBUG: After function parsing, current token: %s\n", token_to_string(context->lexer->token));
    }
    
    return function;
}
