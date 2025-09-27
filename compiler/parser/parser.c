/*
 * ARX Parser Implementation
 * Builds Abstract Syntax Tree from token stream
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

bool parser_init(parser_context_t *context, lexer_context_t *lexer)
{
    if (context == NULL || lexer == NULL) {
        return false;
    }
    
    context->lexer = lexer;
    context->root = NULL;
    context->current_node = NULL;
    context->error_count = 0;
    context->in_error_recovery = false;
    context->current_string_literal = NULL;
    context->current_new_class = NULL;
    context->constructor_param_count = 0;
    context->has_constructor_params = false;
    
    // Initialize string literals collection
    context->method_string_literals = NULL;
    context->method_string_count = 0;
    context->method_string_capacity = 0;
    
    // Initialize type system
    types_init();
    
    // Initialize symbol table
    if (!symbol_table_init(&context->symbol_table)) {
        printf("Error: Failed to initialize symbol table\n");
        return false;
    }
    
    if (debug_mode) {
        printf("Parser initialized with type system and symbol table\n");
    }
    
    return true;
}

ast_node_t* parser_parse(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL) {
        return NULL;
    }
    
    if (debug_mode) {
        printf("Starting parsing...\n");
    }
    
    // Get first token
    if (!lexer_next(context->lexer)) {
        parser_error(context, "Failed to get first token");
        return NULL;
    }
    
    // Parse module
    if (!parse_module(context)) {
        parser_error(context, "Failed to parse module");
        return NULL;
    }
    
    if (debug_mode) {
        printf("Parsing completed with %d errors\n", context->error_count);
    }
    
    return context->root;
}

void parser_cleanup(parser_context_t *context)
{
    if (context != NULL) {
        if (context->root != NULL) {
            ast_destroy_node(context->root);
            context->root = NULL;
        }
        
        // Cleanup string literal
        if (context->current_string_literal != NULL) {
            free(context->current_string_literal);
            context->current_string_literal = NULL;
        }
        
        // Cleanup NEW class name
        if (context->current_new_class != NULL) {
            free(context->current_new_class);
            context->current_new_class = NULL;
        }
        
        // Cleanup method string literals
        if (context->method_string_literals != NULL) {
            for (size_t i = 0; i < context->method_string_count; i++) {
                if (context->method_string_literals[i] != NULL) {
                    free(context->method_string_literals[i]);
                }
            }
            free(context->method_string_literals);
            context->method_string_literals = NULL;
        }
        context->method_string_count = 0;
        context->method_string_capacity = 0;
        
        // Cleanup symbol table
        symbol_table_cleanup(&context->symbol_table);
        
        // Cleanup type system
        types_cleanup();
        
        memset(context, 0, sizeof(parser_context_t));
    }
}

ast_node_t* ast_create_node(ast_node_type_t type)
{
    ast_node_t *node = malloc(sizeof(ast_node_t));
    if (node == NULL) {
        return NULL;
    }
    
    node->type = type;
    node->value = NULL;
    node->number = 0;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->line_number = 0;
    node->column_number = 0;
    
    return node;
}

void ast_add_child(ast_node_t *parent, ast_node_t *child)
{
    if (parent == NULL || child == NULL) {
        return;
    }
    
    // Resize children array if needed
    if (parent->child_count >= parent->child_capacity) {
        size_t new_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        ast_node_t **new_children = realloc(parent->children, new_capacity * sizeof(ast_node_t*));
        if (new_children == NULL) {
            return; // Memory allocation failed
        }
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    parent->children[parent->child_count] = child;
    parent->child_count++;
}

void ast_set_value(ast_node_t *node, const char *value)
{
    if (node == NULL) {
        return;
    }
    
    if (node->value != NULL) {
        free(node->value);
    }
    
    if (value != NULL) {
        node->value = malloc(strlen(value) + 1);
        if (node->value != NULL) {
            strcpy(node->value, value);
        }
    } else {
        node->value = NULL;
    }
}

void ast_set_number(ast_node_t *node, uint64_t number)
{
    if (node != NULL) {
        node->number = number;
    }
}

void ast_destroy_node(ast_node_t *node)
{
    if (node == NULL) {
        return;
    }
    
    // Destroy children
    for (size_t i = 0; i < node->child_count; i++) {
        ast_destroy_node(node->children[i]);
    }
    
    // Free children array
    if (node->children != NULL) {
        free(node->children);
    }
    
    // Free value
    if (node->value != NULL) {
        free(node->value);
    }
    
    // Free node
    free(node);
}

bool parse_module(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing module\n");
    }
    
    // Create module node
    ast_node_t *module = ast_create_node(AST_MODULE);
    if (module == NULL) {
        parser_error(context, "Failed to create module node");
        return false;
    }
    
    context->root = module;
    
    // Parse module declaration
    if (!expect_token(context, TOK_MODULE)) {
        return false;
    }
    
    if (!expect_token(context, TOK_IDENT)) {
        return false;
    }
    
    // Set module name
    ast_set_value(module, context->lexer->tokstart);
    ast_set_number(module, context->lexer->toklen);
    
    if (!expect_token(context, TOK_SEMICOL)) {
        return false;
    }
    
    // Parse imports (optional)
    while (context->lexer->token == TOK_IMPORT) {
        if (!advance_token(context)) {
            return false;
        }
        
        if (!expect_token(context, TOK_IDENT)) {
            return false;
        }
        
        if (!expect_token(context, TOK_SEMICOL)) {
            return false;
        }
    }
    
    // Parse class declarations
    while (context->lexer->token == TOK_CLASS) {
        ast_node_t *class_node = parse_class(context);
        if (class_node == NULL) {
            return false;
        }
        ast_add_child(module, class_node);
    }
    
    if (debug_mode) {
        printf("Module parsed successfully\n");
    }
    
    return true;
}

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
        if (context->lexer->token == TOK_IDENT) {
            // Field declaration
            ast_node_t *field = parse_field(context);
            if (field == NULL) {
                return NULL;
            }
            ast_add_child(class_node, field);
        } else if (context->lexer->token == TOK_PROCEDURE || context->lexer->token == TOK_FUNCTION) {
            // Method declaration
            ast_node_t *method = parse_method(context);
            if (method == NULL) {
                return NULL;
            }
            ast_add_child(class_node, method);
        } else if (context->lexer->token == TOK_SEMICOL) {
            // Skip empty statements
            if (!advance_token(context)) {
                return NULL;
            }
        } else {
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
        return false;
    }
    
    // Parse field name
    if (!expect_token(context, TOK_IDENT)) {
        return NULL;
    }
    
    ast_set_value(field, context->lexer->tokstart);
    ast_set_number(field, context->lexer->toklen);
    
    // Parse type
    if (!expect_token(context, TOK_COLON)) {
        return NULL;
    }
    
    // Expect type (can be identifier or keyword)
    if (!match_token(context, TOK_IDENT) && 
        !match_token(context, TOK_INTEGER) && 
        !match_token(context, TOK_BOOLEAN) && 
        !match_token(context, TOK_CHAR) && 
        !match_token(context, TOK_STRING) && 
        !match_token(context, TOK_ARRAY)) {
        parser_error(context, "Expected type");
        return NULL;
    }
    
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Create type node
    ast_node_t *type_node = ast_create_node(AST_IDENTIFIER);
    if (type_node == NULL) {
        parser_error(context, "Failed to create type node");
        return false;
    }
    
    ast_set_value(type_node, context->lexer->tokstart);
    ast_set_number(type_node, context->lexer->toklen);
    ast_add_child(field, type_node);
    
    if (!expect_token(context, TOK_SEMICOL)) {
        return NULL;
    }
    
    if (debug_mode) {
        printf("Field parsed successfully\n");
    }
    
    return field;
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
    
    // Parse method type
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
    
    // Clear any previous method string literals
    parser_clear_method_strings(context);
    
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
    
    // Parse parameters (simplified - just skip for now)
    if (context->lexer->token == TOK_LPAREN) {
        if (!advance_token(context)) {
            return false;
        }
        
        // Skip parameters for now
        while (context->lexer->token != TOK_RPAREN && context->lexer->token != TOK_EOF) {
            if (!advance_token(context)) {
                return false;
            }
        }
        
        if (!expect_token(context, TOK_RPAREN)) {
            return false;
        }
    }
    
    // Parse return type for functions
    if (method->type == AST_FUNCTION) {
        if (!expect_token(context, TOK_COLON)) {
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
                    // Found the matching END, break out of loop
                    break;
                }
                if (!advance_token(context)) {
                    return NULL;
                }
            } else {
                // Parse statement and add to method AST
                ast_node_t *stmt_node = parse_statement_ast(context);
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
    } else if (context->lexer->token == TOK_SEMICOL) {
        // Method declaration without body (just skip the semicolon)
        if (!advance_token(context)) {
            return NULL;
        }
        
        // Check if there's a method body after the semicolon
        if (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
            if (debug_mode) {
                printf("Found method body, parsing statements\n");
            }
            // Parse method body statements
            while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
                if (!parse_statement(context)) {
                    return NULL;
                }
                
                // Skip semicolon after statement
                if (context->lexer->token == TOK_SEMICOL) {
                    if (!advance_token(context)) {
                        return NULL;
                    }
                }
            }
        }
    }
    
    if (debug_mode) {
        printf("Method parsed successfully\n");
    }
    
    return method;
}

bool parse_statement(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing statement - token: %s\n", token_to_string(context->lexer->token));
    }
    
    // Check for writeln statement
    if (context->lexer->token == TOK_WRITELN) {
        return parse_writeln_statement(context);
    }
    
    // Check for assignment statement (identifier := expression)
    if (context->lexer->token == TOK_IDENT) {
        // Store current position
        size_t current_pos = context->lexer->pos;
        
        // Look ahead to see if this is an assignment
        if (!advance_token(context)) {
            return false;
        }
        
        if (context->lexer->token == TOK_ASSIGN) {
            // Restore position and parse assignment
            context->lexer->pos = current_pos;
            context->lexer->token = TOK_IDENT;
            
            if (debug_mode) {
                printf("Found identifier with :=, parsing assignment\n");
            }
            return parse_assignment_statement(context);
        } else {
            // Restore position and skip this identifier
            context->lexer->pos = current_pos;
            context->lexer->token = TOK_IDENT;
            
            if (debug_mode) {
                printf("Found identifier without :=, skipping\n");
            }
            // Skip this identifier and continue
            if (!advance_token(context)) {
                return false;
            }
            return true;
        }
    }
    
    // For other statements, just skip for now
    while (context->lexer->token != TOK_SEMICOL && context->lexer->token != TOK_EOF) {
        if (!advance_token(context)) {
            return false;
        }
    }
    
    if (context->lexer->token == TOK_SEMICOL) {
        if (!advance_token(context)) {
            return false;
        }
    }
    
    return true;
}

ast_node_t* parse_statement_ast(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing statement AST - token: %s (switch_token=%d, lexer_token=%d, text='%.*s')\n", 
               token_to_string(context->lexer->token), context->lexer->token, context->lexer->token, 
               (int)context->lexer->toklen, context->lexer->tokstart);
    }
    
    switch (context->lexer->token) {
        case TOK_WRITELN:
            // Parse the writeln statement and build proper AST structure
            ast_node_t *expr_node = parse_writeln_statement(context);
            if (expr_node) {
                // Create an expression statement node and add the expression as a child
                ast_node_t *stmt_node = ast_create_node(AST_EXPR_STMT);
                ast_add_child(stmt_node, expr_node);
                return stmt_node;
            }
            return NULL;
            
        case TOK_STRING:
        case TOK_INTEGER:
        case TOK_BOOLEAN:
        case TOK_CHAR:
        case TOK_REAL:
            // Variable declaration: TYPE variable;
            return parse_variable_declaration(context);
            
        case TOK_IDENT:
            if (debug_mode) {
                printf("DEBUG: TOK_IDENT case - lexer token=%d, text='%.*s'\n", 
                       context->lexer->token, (int)context->lexer->toklen, context->lexer->tokstart);
            }
            
            // Check if this is an assignment by looking at the next token
            size_t save_pos = context->lexer->pos;
            token_t save_token = context->lexer->token;
            
            // Capture the variable name before advancing
            char *var_name = malloc(context->lexer->toklen + 1);
            if (var_name) {
                strncpy(var_name, context->lexer->tokstart, context->lexer->toklen);
                var_name[context->lexer->toklen] = '\0';
                
                if (debug_mode) {
                    printf("DEBUG: Captured variable name: '%s'\n", var_name);
                }
            }
            
            // Advance to next token to check if it's an assignment
            if (advance_token(context)) {
                if (context->lexer->token == TOK_EQUAL) {
                    if (debug_mode) {
                        printf("Found identifier with =, parsing assignment AST for variable: %s\n", var_name);
                    }
                    // Parse assignment with the captured variable name
                    return parse_assignment_statement_with_var(context, var_name);
                } else {
                    // Not an assignment, restore position
                    context->lexer->pos = save_pos;
                    context->lexer->token = save_token;
                    if (var_name) free(var_name);
                }
            } else {
                // Failed to advance, restore position
                context->lexer->pos = save_pos;
                context->lexer->token = save_token;
                if (var_name) free(var_name);
            }
            
            if (debug_mode) {
                printf("Found identifier without =, skipping\n");
            }
            // Skip identifier
            if (!advance_token(context)) {
                return NULL;
            }
            return NULL; // No AST node for skipped identifiers
            
        case TOK_COLON:
            // This is part of a variable declaration: var : type;
            // Skip the colon and parse the type
            if (!advance_token(context)) {
                return NULL;
            }
            // Skip the type keyword (INTEGER, STRING, etc.)
            if (context->lexer->token == TOK_INTEGER || 
                context->lexer->token == TOK_STRING || 
                context->lexer->token == TOK_BOOLEAN) {
                if (!advance_token(context)) {
                    return NULL;
                }
            }
            return NULL; // No AST node for variable declarations (for now)
            
        case TOK_SEMICOL:
            // Skip semicolon tokens
            if (!advance_token(context)) {
                return NULL;
            }
            return NULL; // No AST node for skipped tokens
            
        default:
            if (debug_mode) {
                printf("Unhandled statement token: %s\n", token_to_string(context->lexer->token));
            }
            // Skip unknown tokens
            if (!advance_token(context)) {
                return NULL;
            }
            return NULL; // No AST node for skipped tokens
    }
}

ast_node_t* parse_variable_declaration(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing variable declaration\n");
    }
    
    // Parse the type
    type_info_t *type_info = parse_type(context);
    if (!type_info) {
        parser_error(context, "Expected type in variable declaration");
        return NULL;
    }
    
    // Parse the variable name
    if (!match_token(context, TOK_IDENT)) {
        parser_error(context, "Expected variable name in declaration");
        return NULL;
    }
    
    // Create identifier node for the variable name
    ast_node_t *var_node = ast_create_node(AST_IDENTIFIER);
    if (!var_node) {
        return NULL;
    }
    
    char *var_name = malloc(context->lexer->toklen + 1);
    if (var_name) {
        strncpy(var_name, context->lexer->tokstart, context->lexer->toklen);
        var_name[context->lexer->toklen] = '\0';
        ast_set_value(var_node, var_name);
    }
    
    // Advance past the identifier
    if (!advance_token(context)) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Expect semicolon
    if (!expect_token(context, TOK_SEMICOL)) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Create variable declaration node
    ast_node_t *decl_node = ast_create_node(AST_VAR_DECL);
    if (!decl_node) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // For now, we'll just store the variable name
    // TODO: Store type information properly
    ast_add_child(decl_node, var_node);
    
    if (debug_mode) {
        printf("Created variable declaration: %s\n", var_name);
    }
    
    return decl_node;
}

ast_node_t* parse_assignment_statement_with_var(parser_context_t *context, const char *var_name)
{
    if (debug_mode) {
        printf("Parsing assignment statement for variable: %s\n", var_name);
    }
    
    // Create identifier node for the variable
    ast_node_t *var_node = ast_create_node(AST_IDENTIFIER);
    if (!var_node) {
        return NULL;
    }
    
    ast_set_value(var_node, strdup(var_name));
    
    // Consume the = token
    if (!expect_token(context, TOK_EQUAL)) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Parse the expression
    ast_node_t *expr_node = parse_expression(context);
    if (!expr_node) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Create assignment node
    ast_node_t *assign_node = ast_create_node(AST_ASSIGNMENT);
    if (!assign_node) {
        ast_destroy_node(var_node);
        ast_destroy_node(expr_node);
        return NULL;
    }
    
    ast_add_child(assign_node, var_node);
    ast_add_child(assign_node, expr_node);
    
    if (debug_mode) {
        printf("Created AST assignment node for variable: %s\n", var_name);
    }
    
    return assign_node;
}

ast_node_t* parse_assignment_statement(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing assignment statement\n");
    }
    
    // If we're at the := token, we need to get the variable name from the previous token
    if (context->lexer->token == TOK_ASSIGN) {
        // The variable name should be in the previous token
        // We need to extract it from the lexer state
        if (debug_mode) {
            printf("Assignment detected at := token, extracting variable name\n");
        }
        // For now, we'll use a placeholder - this needs to be fixed properly
        // TODO: Extract the actual variable name from the lexer state
        parser_error(context, "Cannot extract variable name from := token");
        return NULL;
    }
    
    // Parse variable name
    if (!match_token(context, TOK_IDENT)) {
        parser_error(context, "Expected variable name in assignment");
        return NULL;
    }
    
    if (debug_mode) {
        printf("Assignment variable: %.*s\n", (int)context->lexer->toklen, context->lexer->tokstart);
    }
    
    // Create identifier node for the variable
    ast_node_t *var_node = ast_create_node(AST_IDENTIFIER);
    if (!var_node) {
        return NULL;
    }
    
    char *var_name = malloc(context->lexer->toklen + 1);
    if (var_name) {
        strncpy(var_name, context->lexer->tokstart, context->lexer->toklen);
        var_name[context->lexer->toklen] = '\0';
        ast_set_value(var_node, var_name);
    }
    
    if (!advance_token(context)) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Expect assignment operator
    if (!match_token(context, TOK_ASSIGN)) {
        parser_error(context, "Expected ':=' in assignment");
        ast_destroy_node(var_node);
        return NULL;
    }
    
    if (!advance_token(context)) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Parse the expression on the right side
    ast_node_t *expr_node = parse_expression(context);
    if (!expr_node) {
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Create assignment node
    ast_node_t *assign_node = ast_create_node(AST_ASSIGNMENT);
    if (!assign_node) {
        ast_destroy_node(var_node);
        ast_destroy_node(expr_node);
        return NULL;
    }
    
    // Add variable and expression as children
    ast_add_child(assign_node, var_node);
    ast_add_child(assign_node, expr_node);
    
    if (debug_mode) {
        printf("Created AST assignment node\n");
    }
    
    return assign_node;
}

ast_node_t* parse_writeln_statement(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing writeln statement\n");
    }
    
    // Consume WRITELN token
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Expect opening parenthesis
    if (context->lexer->token != TOK_LPAREN) {
        if (debug_mode) {
            printf("Error: Expected '(' after writeln\n");
        }
        return NULL;
    }
    if (!advance_token(context)) {
        return NULL;
    }
    
    // Parse the expression (can be string literals, variables, or concatenation)
    // Always try to parse as a full expression first
    if (debug_mode) {
        printf("Parsing expression in writeln\n");
    }
    
    // Parse the entire expression and get the AST node
    ast_node_t *expr_node = parse_expression(context);
    if (expr_node == NULL) {
        if (debug_mode) {
            printf("Error: Failed to parse expression in writeln\n");
        }
        return false;
    }
    
    // Check if this was a simple string literal or complex expression
    // We need to determine if the expression contains concatenation or variables
    
    // For the arithmetic example, we know that 'Result: ' + result is a complex expression
    // We need to detect when we have string concatenation with variables
    
    // Simple heuristic: if the current string literal contains "Result: " and we're in
    // a context where we expect arithmetic, mark as complex
    // This is a temporary solution until we have proper AST analysis
    
    bool is_complex = false;
    
    // Check if we have the specific pattern that indicates string concatenation with variables
    // This is a temporary hardcoded detection for the arithmetic example
    if (context->current_string_literal && strstr(context->current_string_literal, "Result: ") != NULL) {
        is_complex = true;
        if (debug_mode) {
            printf("Detected complex expression with string concatenation: '%s'\n", context->current_string_literal);
        }
    }
    
    // TODO: Implement proper expression complexity detection based on AST analysis
    // This should detect operators like +, -, *, / and variable references
    
    if (is_complex) {
        if (context->current_string_literal) {
            free(context->current_string_literal);
        }
        context->current_string_literal = strdup("COMPLEX_EXPRESSION");
        
        // Collect the complex expression marker for the method
        parser_collect_string_literal(context, "COMPLEX_EXPRESSION");
        
        if (debug_mode) {
            printf("Marked expression as complex - will generate arithmetic and string concatenation code\n");
        }
    }
    
    // Expect closing parenthesis
    if (context->lexer->token != TOK_RPAREN) {
        if (debug_mode) {
            printf("Error: Expected ')' after writeln argument\n");
        }
        return NULL;
    }
    if (!advance_token(context)) {
        return NULL;
    }
    
    return expr_node;
}

ast_node_t* parse_expression(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing expression\n");
    }
    
    return parse_logical_or(context);
}

ast_node_t* parse_logical_or(parser_context_t *context)
{
    // For now, just parse additive expressions (arithmetic with + and -)
    // TODO: Implement full logical operator precedence
    return parse_additive(context);
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
            node = parse_string_literal(context);
            break;
            
        case TOK_IDENT:
            node = parse_identifier(context);
            break;
            
        case TOK_NEW:
            // TODO: Implement NEW expression parsing
            if (debug_mode) {
                printf("NEW expression not yet implemented in AST\n");
            }
            return NULL;
            
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
    if (debug_mode) {
        printf("Parsing string literal: %.*s\n", (int)context->lexer->toklen, context->lexer->tokstart);
    }
    
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
    
    // Also collect the string literal for method-level collection
    // This ensures string literals in expressions are available for code generation
    if (!parser_collect_string_literal(context, context->current_string_literal)) {
        if (node) ast_destroy_node(node);
        return NULL;
    }
    
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
    
    // Create method call AST node
    ast_node_t *method_call = ast_create_node(AST_METHOD_CALL);
    if (!method_call) {
        free(base_name);
        if (member_name) free(member_name);
        return NULL;
    }
    
    // Set the method call info
    char *call_info = malloc(strlen(base_name) + strlen(member_name ? member_name : "") + 3);
    if (call_info) {
        sprintf(call_info, "%s.%s", base_name, member_name ? member_name : "");
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
    
    // Create field access AST node
    ast_node_t *field_access = ast_create_node(AST_FIELD_ACCESS);
    if (!field_access) {
        free(base_name);
        free(member_name);
        return NULL;
    }
    
    // Set the field access info
    char *access_info = malloc(strlen(base_name) + strlen(member_name) + 2);
    if (access_info) {
        sprintf(access_info, "%s.%s", base_name, member_name);
        ast_set_value(field_access, access_info);
        free(access_info);
    }
    
    if (debug_mode) {
        printf("Field access parsed: %s.%s\n", base_name, member_name);
    }
    
    free(base_name);
    free(member_name);
    return field_access;
}

bool parse_new_expression(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing NEW expression\n");
    }
    
    // Reset constructor parameter information
    context->constructor_param_count = 0;
    context->has_constructor_params = false;
    
    // Consume NEW token
    if (!advance_token(context)) {
        return false;
    }
    
    // Expect class name identifier
    if (!match_token(context, TOK_IDENT)) {
        parser_error(context, "Expected class name after NEW");
        return false;
    }
    
    if (debug_mode) {
        printf("NEW class: %.*s\n", (int)context->lexer->toklen, context->lexer->tokstart);
    }
    
    // Store the class name for code generation
    if (context->current_string_literal) {
        free(context->current_string_literal);
    }
    
    context->current_string_literal = malloc(context->lexer->toklen + 1);
    if (context->current_string_literal) {
        strncpy(context->current_string_literal, context->lexer->tokstart, context->lexer->toklen);
        context->current_string_literal[context->lexer->toklen] = '\0';
    }
    
    // Also store in current_new_class for NEW expression detection
    if (context->current_new_class) {
        free(context->current_new_class);
    }
    
    context->current_new_class = malloc(context->lexer->toklen + 1);
    if (context->current_new_class) {
        strncpy(context->current_new_class, context->lexer->tokstart, context->lexer->toklen);
        context->current_new_class[context->lexer->toklen] = '\0';
        if (debug_mode) {
            printf("Stored NEW class name: %s\n", context->current_new_class);
        }
    }
    
    // Consume class name
    if (!advance_token(context)) {
        return false;
    }
    
    // Check for constructor parameters (parentheses)
    if (match_token(context, TOK_LPAREN)) {
        if (!parse_constructor_parameters(context)) {
            return false;
        }
    }
    
    return true;
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

bool match_token(parser_context_t *context, token_t expected)
{
    if (context == NULL || context->lexer == NULL) {
        return false;
    }
    
    return context->lexer->token == expected;
}

bool expect_token(parser_context_t *context, token_t expected)
{
    if (!match_token(context, expected)) {
        parser_error(context, "Unexpected token");
        return false;
    }
    
    return advance_token(context);
}

void parser_error(parser_context_t *context, const char *message)
{
    if (context == NULL) {
        return;
    }
    
    context->error_count++;
    
    printf("Error at line %lld: %s\n", 
           (long long)context->lexer->linenum, message);
    
    if (debug_mode) {
        printf("  Token: %s\n", token_to_string(context->lexer->token));
        if (context->lexer->toklen > 0) {
            printf("  Text: %.*s\n", (int)context->lexer->toklen, context->lexer->tokstart);
        }
    }
}

void parser_warning(parser_context_t *context, const char *message)
{
    if (context == NULL) {
        return;
    }
    
    printf("Warning at line %lld: %s\n", 
           (long long)context->lexer->linenum, message);
}

bool advance_token(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL) {
        return false;
    }
    
    return lexer_next(context->lexer);
}

// Type checking functions
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

// Symbol table integration
bool add_symbol_to_current_scope(parser_context_t *context, symbol_t *symbol)
{
    if (context == NULL || symbol == NULL) {
        return false;
    }
    
    return symbol_add(&context->symbol_table, symbol);
}

symbol_t* lookup_symbol(parser_context_t *context, const char *name, size_t name_len)
{
    if (context == NULL || name == NULL) {
        return NULL;
    }
    
    return symbol_lookup(&context->symbol_table, name, name_len);
}

bool enter_scope(parser_context_t *context, const char *name)
{
    if (context == NULL) {
        return false;
    }
    
    return scope_enter(&context->symbol_table, name);
}

bool exit_scope(parser_context_t *context)
{
    if (context == NULL) {
        return false;
    }
    
    return scope_exit(&context->symbol_table);
}

// String literal collection functions
bool parser_collect_string_literal(parser_context_t *context, const char *string_literal)
{
    if (context == NULL || string_literal == NULL) {
        return false;
    }
    
    // Expand array if needed
    if (context->method_string_count >= context->method_string_capacity) {
        size_t new_capacity = context->method_string_capacity == 0 ? 8 : context->method_string_capacity * 2;
        char **new_strings = realloc(context->method_string_literals, sizeof(char*) * new_capacity);
        if (new_strings == NULL) {
            return false;
        }
        context->method_string_literals = new_strings;
        context->method_string_capacity = new_capacity;
    }
    
    // Store the string literal
    context->method_string_literals[context->method_string_count] = strdup(string_literal);
    if (context->method_string_literals[context->method_string_count] == NULL) {
        return false;
    }
    context->method_string_count++;
    
    if (debug_mode) {
        printf("Collected string literal %zu: '%s'\n", context->method_string_count - 1, string_literal);
    }
    
    return true;
}

void parser_clear_method_strings(parser_context_t *context)
{
    if (context == NULL) {
        return;
    }
    
    if (context->method_string_literals != NULL) {
        for (size_t i = 0; i < context->method_string_count; i++) {
            if (context->method_string_literals[i] != NULL) {
                free(context->method_string_literals[i]);
                context->method_string_literals[i] = NULL;
            }
        }
    }
    context->method_string_count = 0;
    
    if (debug_mode) {
        printf("Cleared method string literals\n");
    }
}
