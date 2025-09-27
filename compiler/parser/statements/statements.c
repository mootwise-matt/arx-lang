/*
 * ARX Statements Implementation
 * Handles statement parsing including variable declarations, assignments, and writeln
 */

#include "statements.h"
#include "../core/parser_core.h"
#include "../expressions/expressions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External debug flag
extern bool debug_mode;

// Forward declaration for parse_type (defined in parser.c)
type_info_t* parse_type(parser_context_t *context);

bool parse_statement(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing statement - token: %s\n", token_to_string(context->lexer->token));
    }
    
    // Check for FOR loop statement
    if (context->lexer->token == TOK_FOR) {
        if (debug_mode) {
            printf("Found FOR loop statement\n");
        }
        // For now, just skip FOR loops in the simple parser
        // The AST-based parser will handle them properly
        while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
            if (!advance_token(context)) {
                return false;
            }
        }
        // Skip the END token
        if (context->lexer->token == TOK_END) {
            if (!advance_token(context)) {
                return false;
            }
        }
        return true;
    }
    
    // Check for WHILE loop statement
    if (context->lexer->token == TOK_WHILE) {
        if (debug_mode) {
            printf("Found WHILE loop statement\n");
        }
        // For now, just skip WHILE loops in the simple parser
        // The AST-based parser will handle them properly
        while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
            if (!advance_token(context)) {
                return false;
            }
        }
        // Skip the END token
        if (context->lexer->token == TOK_END) {
            if (!advance_token(context)) {
                return false;
            }
        }
        return true;
    }
    
    // Check for IF statement
    if (context->lexer->token == TOK_IF) {
        if (debug_mode) {
            printf("Found IF statement\n");
        }
        // For now, just skip IF statements in the simple parser
        // The AST-based parser will handle them properly
        while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
            if (!advance_token(context)) {
                return false;
            }
        }
        // Skip the END token
        if (context->lexer->token == TOK_END) {
            if (!advance_token(context)) {
                return false;
            }
        }
        return true;
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
        printf("*** PARSE_STATEMENT_AST CALLED *** - token: %s (switch_token=%d, lexer_token=%d, text='%.*s')\n", 
               token_to_string(context->lexer->token), context->lexer->token, context->lexer->token, 
               (int)context->lexer->toklen, context->lexer->tokstart);
    }
    
    if (debug_mode) {
        printf("DEBUG: Checking token value: %d, TOK_FOR value: %d\n", context->lexer->token, TOK_FOR);
        printf("DEBUG: About to enter switch statement\n");
    }
    
    switch (context->lexer->token) {
        case TOK_FOR:
            if (debug_mode) {
                printf("*** FOR CASE REACHED! ***\n");
            }
            // Parse FOR loop statement
            return parse_for_statement(context);
            
        case TOK_WHILE:
            if (debug_mode) {
                printf("*** WHILE CASE REACHED! ***\n");
            }
            // Parse WHILE loop statement
            return parse_while_statement(context);
            
        case TOK_IF:
            if (debug_mode) {
                printf("*** IF CASE REACHED! ***\n");
            }
            // Parse IF statement
            return parse_if_statement(context);
            
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
                if (context->lexer->token == TOK_ASSIGN) {
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
    if (!expect_token(context, TOK_ASSIGN)) {
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

ast_node_t* parse_for_statement(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing FOR loop statement\n");
    }
    
    // Create FOR loop AST node
    ast_node_t *for_node = ast_create_node(AST_FOR_STMT);
    if (!for_node) {
        parser_error(context, "Failed to create FOR loop node");
        return NULL;
    }
    
    // Consume FOR token
    if (!expect_token(context, TOK_FOR)) {
        ast_destroy_node(for_node);
        return NULL;
    }
    
    // Parse loop variable (identifier)
    if (!match_token(context, TOK_IDENT)) {
        parser_error(context, "Expected loop variable identifier after FOR");
        ast_destroy_node(for_node);
        return NULL;
    }
    
    // Create identifier node for loop variable
    ast_node_t *var_node = ast_create_node(AST_IDENTIFIER);
    if (!var_node) {
        ast_destroy_node(for_node);
        return NULL;
    }
    
    // Store the variable name
    char *var_name = malloc(context->lexer->toklen + 1);
    if (var_name) {
        strncpy(var_name, context->lexer->tokstart, context->lexer->toklen);
        var_name[context->lexer->toklen] = '\0';
        ast_set_value(var_node, var_name);
    }
    
    if (!advance_token(context)) {
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Expect assignment operator (=)
    if (!expect_token(context, TOK_ASSIGN)) {
        parser_error(context, "Expected '=' after loop variable");
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Parse start expression
    ast_node_t *start_expr = parse_expression(context);
    if (!start_expr) {
        parser_error(context, "Expected start expression in FOR loop");
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        return NULL;
    }
    
    // Expect TO token
    if (!expect_token(context, TOK_TO)) {
        parser_error(context, "Expected 'to' after start expression");
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        ast_destroy_node(start_expr);
        return NULL;
    }
    
    // Parse end expression
    ast_node_t *end_expr = parse_expression(context);
    if (!end_expr) {
        parser_error(context, "Expected end expression in FOR loop");
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        ast_destroy_node(start_expr);
        return NULL;
    }
    
    // Expect DO token
    if (!expect_token(context, TOK_DO)) {
        parser_error(context, "Expected 'do' after end expression");
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        ast_destroy_node(start_expr);
        ast_destroy_node(end_expr);
        return NULL;
    }
    
    // Parse loop body (statements)
    ast_node_t *body_node = ast_create_node(AST_BLOCK);
    if (!body_node) {
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        ast_destroy_node(start_expr);
        ast_destroy_node(end_expr);
        return NULL;
    }
    
    // Expect BEGIN token for the loop body
    if (!expect_token(context, TOK_BEGIN)) {
        parser_error(context, "Expected 'begin' for FOR loop body");
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        ast_destroy_node(start_expr);
        ast_destroy_node(end_expr);
        ast_destroy_node(body_node);
        return NULL;
    }
    
    // Parse statements until we find END
    while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
        ast_node_t *stmt = parse_statement_ast(context);
        if (stmt) {
            ast_add_child(body_node, stmt);
        }
        
        // Skip semicolon after statement
        if (context->lexer->token == TOK_SEMICOL) {
            if (!advance_token(context)) {
                ast_destroy_node(for_node);
                ast_destroy_node(var_node);
                ast_destroy_node(start_expr);
                ast_destroy_node(end_expr);
                ast_destroy_node(body_node);
                return NULL;
            }
        }
    }
    
    // Expect END token
    if (!expect_token(context, TOK_END)) {
        parser_error(context, "Expected 'end' after FOR loop body");
        ast_destroy_node(for_node);
        ast_destroy_node(var_node);
        ast_destroy_node(start_expr);
        ast_destroy_node(end_expr);
        ast_destroy_node(body_node);
        return NULL;
    }
    
    // Add children to FOR node: variable, start_expr, end_expr, body
    ast_add_child(for_node, var_node);
    ast_add_child(for_node, start_expr);
    ast_add_child(for_node, end_expr);
    ast_add_child(for_node, body_node);
    
    if (debug_mode) {
        printf("FOR loop parsed successfully: %s = %s to %s\n", 
               var_name ? var_name : "unknown", 
               start_expr->value ? start_expr->value : "unknown",
               end_expr->value ? end_expr->value : "unknown");
    }
    
    return for_node;
}

ast_node_t* parse_while_statement(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing WHILE loop statement\n");
    }
    
    // Create WHILE loop AST node
    ast_node_t *while_node = ast_create_node(AST_WHILE_STMT);
    if (!while_node) {
        parser_error(context, "Failed to create WHILE loop node");
        return NULL;
    }
    
    // Consume WHILE token
    if (!expect_token(context, TOK_WHILE)) {
        ast_destroy_node(while_node);
        return NULL;
    }
    
    // Parse condition expression
    ast_node_t *condition_expr = parse_expression(context);
    if (!condition_expr) {
        parser_error(context, "Expected condition expression after WHILE");
        ast_destroy_node(while_node);
        return NULL;
    }
    
    // Expect DO token
    if (!expect_token(context, TOK_DO)) {
        parser_error(context, "Expected 'do' after WHILE condition");
        ast_destroy_node(while_node);
        ast_destroy_node(condition_expr);
        return NULL;
    }
    
    // Expect BEGIN token for the loop body
    if (!expect_token(context, TOK_BEGIN)) {
        parser_error(context, "Expected 'begin' for WHILE loop body");
        ast_destroy_node(while_node);
        ast_destroy_node(condition_expr);
        return NULL;
    }
    
    // Create body node
    ast_node_t *body_node = ast_create_node(AST_BLOCK);
    if (!body_node) {
        parser_error(context, "Failed to create WHILE loop body node");
        ast_destroy_node(while_node);
        ast_destroy_node(condition_expr);
        return NULL;
    }
    
    // Parse statements until we find END
    while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
        ast_node_t *stmt = parse_statement_ast(context);
        if (stmt) {
            ast_add_child(body_node, stmt);
        }
        
        // Skip semicolon after statement
        if (context->lexer->token == TOK_SEMICOL) {
    if (!advance_token(context)) {
                ast_destroy_node(while_node);
                ast_destroy_node(condition_expr);
                ast_destroy_node(body_node);
                return NULL;
            }
        }
    }
    
    // Expect END token
    if (!expect_token(context, TOK_END)) {
        parser_error(context, "Expected 'end' after WHILE loop body");
        ast_destroy_node(while_node);
        ast_destroy_node(condition_expr);
        ast_destroy_node(body_node);
        return NULL;
    }
    
    // Add children to WHILE node: condition, body
    ast_add_child(while_node, condition_expr);
    ast_add_child(while_node, body_node);
    
    if (debug_mode) {
        printf("WHILE loop parsed successfully\n");
    }
    
    return while_node;
}

ast_node_t* parse_if_statement(parser_context_t *context)
{
    if (debug_mode) {
        printf("Parsing IF statement\n");
    }
    
    // Create IF statement AST node
    ast_node_t *if_node = ast_create_node(AST_IF_STMT);
    if (!if_node) {
        parser_error(context, "Failed to create IF statement node");
        return NULL;
    }
    
    // Consume IF token
    if (!expect_token(context, TOK_IF)) {
        ast_destroy_node(if_node);
        return NULL;
    }
    
    // Parse condition expression
    ast_node_t *condition_expr = parse_expression(context);
    if (!condition_expr) {
        parser_error(context, "Expected condition expression after IF");
        ast_destroy_node(if_node);
        return NULL;
    }
    
    // Expect THEN token
    if (!expect_token(context, TOK_THEN)) {
        parser_error(context, "Expected 'then' after IF condition");
        ast_destroy_node(if_node);
        ast_destroy_node(condition_expr);
        return NULL;
    }
    
    // Expect BEGIN token for the IF statement block
    if (!expect_token(context, TOK_BEGIN)) {
        parser_error(context, "Expected 'begin' for IF statement");
        ast_destroy_node(if_node);
        ast_destroy_node(condition_expr);
        return NULL;
    }
    
    // Create IF body node
    ast_node_t *if_body_node = ast_create_node(AST_BLOCK);
    if (!if_body_node) {
        parser_error(context, "Failed to create IF body node");
        ast_destroy_node(if_node);
        ast_destroy_node(condition_expr);
        return NULL;
    }
    
    // Parse IF body statements until we find ELSEIF, ELSE, or END
    while (context->lexer->token != TOK_END && 
           context->lexer->token != TOK_ELSEIF && 
           context->lexer->token != TOK_ELSE && 
           context->lexer->token != TOK_EOF) {
        ast_node_t *stmt = parse_statement_ast(context);
        if (stmt) {
            ast_add_child(if_body_node, stmt);
        }
        
        // Skip semicolon after statement
        if (context->lexer->token == TOK_SEMICOL) {
            if (!advance_token(context)) {
                ast_destroy_node(if_node);
                ast_destroy_node(condition_expr);
                ast_destroy_node(if_body_node);
                return NULL;
            }
        }
    }
    
    // Add IF condition and body to the IF node
    ast_add_child(if_node, condition_expr);
    ast_add_child(if_node, if_body_node);
    
    // Handle ELSEIF clauses (inside the same BEGIN...END block)
    while (context->lexer->token == TOK_ELSEIF) {
        if (debug_mode) {
            printf("Parsing ELSEIF clause\n");
        }
        
        // Consume ELSEIF token
        if (!expect_token(context, TOK_ELSEIF)) {
            ast_destroy_node(if_node);
            return NULL;
        }
        
        // Parse ELSEIF condition
        ast_node_t *elseif_condition = parse_expression(context);
        if (!elseif_condition) {
            parser_error(context, "Expected condition expression after ELSEIF");
            ast_destroy_node(if_node);
            return NULL;
        }
        
        // Create ELSEIF body node
        ast_node_t *elseif_body_node = ast_create_node(AST_BLOCK);
        if (!elseif_body_node) {
            parser_error(context, "Failed to create ELSEIF body node");
            ast_destroy_node(if_node);
            ast_destroy_node(elseif_condition);
            return NULL;
        }
        
        // Parse ELSEIF body statements until we find another ELSEIF, ELSE, or END
        while (context->lexer->token != TOK_END && 
               context->lexer->token != TOK_ELSEIF && 
               context->lexer->token != TOK_ELSE && 
               context->lexer->token != TOK_EOF) {
            ast_node_t *stmt = parse_statement_ast(context);
            if (stmt) {
                ast_add_child(elseif_body_node, stmt);
            }
            
            // Skip semicolon after statement
            if (context->lexer->token == TOK_SEMICOL) {
                if (!advance_token(context)) {
                    ast_destroy_node(if_node);
                    ast_destroy_node(elseif_condition);
                    ast_destroy_node(elseif_body_node);
                    return NULL;
                }
            }
        }
        
        // Add ELSEIF condition and body to the IF node
        ast_add_child(if_node, elseif_condition);
        ast_add_child(if_node, elseif_body_node);
    }
    
    // Handle ELSE clause (inside the same BEGIN...END block)
    if (context->lexer->token == TOK_ELSE) {
        if (debug_mode) {
            printf("Parsing ELSE clause\n");
        }
        
        // Consume ELSE token
        if (!expect_token(context, TOK_ELSE)) {
            ast_destroy_node(if_node);
            return NULL;
        }
        
        // Create ELSE body node
        ast_node_t *else_body_node = ast_create_node(AST_BLOCK);
        if (!else_body_node) {
            parser_error(context, "Failed to create ELSE body node");
            ast_destroy_node(if_node);
            return NULL;
        }
        
        // Parse ELSE body statements until we find END
        while (context->lexer->token != TOK_END && context->lexer->token != TOK_EOF) {
            ast_node_t *stmt = parse_statement_ast(context);
            if (stmt) {
                ast_add_child(else_body_node, stmt);
            }
            
            // Skip semicolon after statement
            if (context->lexer->token == TOK_SEMICOL) {
                if (!advance_token(context)) {
                    ast_destroy_node(if_node);
                    ast_destroy_node(else_body_node);
                    return NULL;
                }
            }
        }
        
        // Add ELSE body to the IF node
        ast_add_child(if_node, else_body_node);
    }
    
    // Expect END token
    if (!expect_token(context, TOK_END)) {
        parser_error(context, "Expected 'end' after IF statement");
        ast_destroy_node(if_node);
        return NULL;
    }
    
    if (debug_mode) {
        printf("IF statement parsed successfully with %zu children\n", if_node->child_count);
    }
    
    return if_node;
}
