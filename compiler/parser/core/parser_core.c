/*
 * ARX Core Parser Implementation
 * Handles core parser functionality, initialization, and utility functions
 */

#include "parser_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External debug flag
extern bool debug_mode;

// Forward declaration for parse_class (defined in parser.c)
ast_node_t* parse_class(parser_context_t *context);

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
    
    // Initialize method signature storage
    context->method_signatures = NULL;
    context->method_signature_count = 0;
    context->method_signature_capacity = 0;
    
    // Initialize type system
    types_init();
    
    // Initialize symbol table
    if (!symbol_table_init(&context->symbol_table)) {
        printf("Error: Failed to initialize symbol table\n");
        return false;
    }
    
    // Method signatures disabled for now to fix parsing error
    // parser_add_method_signature(context, "system", "writeln", NULL, NULL, 0); // system.writeln is a procedure
    
    if (debug_mode) {
        printf("Parser initialized with type system, symbol table, and method signatures\n");
    }
    
    return true;
}

ast_node_t* parser_parse(parser_context_t *context)
{
    if (context == NULL || context->lexer == NULL) {
        return NULL;
    }
    
    if (debug_mode) {
        printf("DEBUG: Starting parsing...\n");
    }
    
    // Get first token
    if (!lexer_next(context->lexer)) {
        parser_error(context, "Failed to get first token");
        return NULL;
    }
    
    if (debug_mode) {
        printf("DEBUG: First token: %s (%d)\n", token_to_string(context->lexer->token), context->lexer->token);
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

bool parse_module(parser_context_t *context)
{
    if (debug_mode) {
        printf("DEBUG: parse_module called - current token: %s (%d)\n", token_to_string(context->lexer->token), context->lexer->token);
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
    
    // Check for identifier token and read module name before advancing
    if (!match_token(context, TOK_IDENT)) {
        parser_error(context, "Expected module name identifier");
        return false;
    }
    
    // Set module name (before advancing to next token)
    ast_set_value_from_token(module, context->lexer->tokstart, context->lexer->toklen);
    ast_set_number(module, context->lexer->toklen);
    
    // Now advance past the identifier
    if (!advance_token(context)) {
        return false;
    }
    
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
    
    // Parse class declarations and other module-level constructs
    while (context->lexer->token != TOK_EOF) {
        if (debug_mode) {
            printf("DEBUG: Module parsing - token: %s (%d)\n", token_to_string(context->lexer->token), context->lexer->token);
        }
        if (context->lexer->token == TOK_CLASS) {
            if (debug_mode) {
                printf("DEBUG: Found class declaration\n");
            }
            ast_node_t *class_node = parse_class(context);
            if (class_node == NULL) {
                return false;
            }
            ast_add_child(module, class_node);
        } else if (context->lexer->token == TOK_SEMICOL) {
            // Skip empty statements
            if (!advance_token(context)) {
                return false;
            }
        } else if (context->lexer->token == TOK_EOL) {
            // Skip end-of-line tokens
            if (!advance_token(context)) {
                return false;
            }
        } else {
            // Unexpected token - try to advance and continue parsing
            if (!advance_token(context)) {
                break;
            }
        }
    }
    
    if (debug_mode) {
        printf("Module parsed successfully\n");
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

// Method Signature Management Functions

bool parser_add_method_signature(parser_context_t *context, const char *class_name, 
                                const char *method_name, const char *return_type, 
                                char **param_types, size_t param_count)
{
    if (context == NULL || class_name == NULL || method_name == NULL) {
        return false;
    }
    
    // Expand array if needed
    if (context->method_signature_count >= context->method_signature_capacity) {
        size_t new_capacity = context->method_signature_capacity == 0 ? 8 : context->method_signature_capacity * 2;
        method_signature_t *new_signatures = realloc(context->method_signatures, 
                                                   sizeof(method_signature_t) * new_capacity);
        if (new_signatures == NULL) {
            return false;
        }
        context->method_signatures = new_signatures;
        context->method_signature_capacity = new_capacity;
    }
    
    // Add new method signature
    method_signature_t *signature = &context->method_signatures[context->method_signature_count];
    signature->class_name = strdup(class_name);
    signature->method_name = strdup(method_name);
    signature->return_type = return_type ? strdup(return_type) : NULL;
    signature->param_count = param_count;
    signature->is_procedure = (return_type == NULL);
    
    // Copy parameter types
    if (param_count > 0 && param_types != NULL) {
        signature->param_types = malloc(sizeof(char*) * param_count);
        if (signature->param_types == NULL) {
            free(signature->class_name);
            free(signature->method_name);
            if (signature->return_type) free(signature->return_type);
            return false;
        }
        for (size_t i = 0; i < param_count; i++) {
            signature->param_types[i] = param_types[i] ? strdup(param_types[i]) : NULL;
        }
    } else {
        signature->param_types = NULL;
    }
    
    context->method_signature_count++;
    
    if (debug_mode) {
        printf("Added method signature: %s.%s() -> %s\n", 
               class_name, method_name, signature->is_procedure ? "procedure" : "function");
    }
    
    return true;
}

method_signature_t* parser_lookup_method_signature(parser_context_t *context, 
                                                  const char *class_name, const char *method_name)
{
    if (context == NULL || class_name == NULL || method_name == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < context->method_signature_count; i++) {
        method_signature_t *signature = &context->method_signatures[i];
        if (strcmp(signature->class_name, class_name) == 0 && 
            strcmp(signature->method_name, method_name) == 0) {
            return signature;
        }
    }
    
    return NULL;
}

bool parser_is_procedure(parser_context_t *context, const char *class_name, const char *method_name)
{
    method_signature_t *signature = parser_lookup_method_signature(context, class_name, method_name);
    if (signature == NULL) {
        // Default to procedure if not found
        return true;
    }
    return signature->is_procedure;
}
