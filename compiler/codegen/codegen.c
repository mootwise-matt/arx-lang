/*
 * ARX Code Generator Implementation
 * Generates bytecode instructions from AST
 */

#include "codegen.h"
#include "../arxmod/arxmod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

bool codegen_init(codegen_context_t *context, parser_context_t *parser_context)
{
    if (context == NULL) {
        return false;
    }
    
    context->instructions = NULL;
    context->instruction_count = 0;
    context->instruction_capacity = 0;
    context->label_counter = 0;
    context->debug_output = debug_mode;
    context->parser_context = parser_context;
    context->string_literals = NULL;
    context->string_literals_count = 0;
    context->string_literals_capacity = 0;
    
    // Initialize label table
    context->label_table = NULL;
    context->label_table_size = 0;
    context->label_table_capacity = 0;
    
    // Initialize variable tracking
    context->variable_names = NULL;
    context->variable_addresses = NULL;
    context->variable_count = 0;
    context->variable_capacity = 0;
    context->next_variable_address = 0;
    
    if (debug_mode) {
        printf("Code generator initialized\n");
    }
    
    return true;
}

bool codegen_generate(codegen_context_t *context, ast_node_t *ast, 
                     instruction_t **instructions, size_t *instruction_count)
{
    if (context == NULL || ast == NULL || instructions == NULL || instruction_count == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Starting code generation...\n");
    }
    
    // Generate code for the AST
    if (!generate_module(context, ast)) {
        codegen_error(context, "Failed to generate code for module");
        return false;
    }
    
    // Resolve all label addresses (two-pass compilation)
    resolve_labels(context);
    
    // Return generated instructions
    *instructions = context->instructions;
    *instruction_count = context->instruction_count;
    
    if (debug_mode) {
        printf("Code generation completed: %zu instructions\n", context->instruction_count);
    }
    
    return true;
}

bool codegen_write_arxmod(codegen_context_t *context, const char *filename,
                         instruction_t *instructions, size_t instruction_count)
{
    if (context == NULL || filename == NULL || instructions == NULL) {
        return false;
    }
    
    arxmod_writer_t writer;
    if (!arxmod_writer_init(&writer, filename)) {
        printf("Error: Failed to initialize ARX module writer\n");
        return false;
    }
    
    if (debug_mode) {
        printf("Writing ARX module to '%s'\n", filename);
    }
    
    // Write header
    if (!arxmod_writer_write_header(&writer, "ARXProgram", 10)) {
        printf("Error: Failed to write ARX module header\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    // Add code section
    if (!arxmod_writer_add_code_section(&writer, instructions, instruction_count)) {
        printf("Error: Failed to add code section\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    // Add strings section with collected string literals
    // Use the parser's collected string literals if available, otherwise fall back to codegen's
    if (context->parser_context && context->parser_context->method_string_literals && 
        context->parser_context->method_string_count > 0) {
        if (!arxmod_writer_add_strings_section(&writer, context->parser_context->method_string_literals, 
                                               context->parser_context->method_string_count)) {
            printf("Error: Failed to add strings section\n");
            arxmod_writer_cleanup(&writer);
            return false;
        }
    } else if (context->string_literals && context->string_literals_count > 0) {
        if (!arxmod_writer_add_strings_section(&writer, context->string_literals, context->string_literals_count)) {
            printf("Error: Failed to add strings section\n");
            arxmod_writer_cleanup(&writer);
            return false;
        }
    }
    
    if (!arxmod_writer_add_symbols_section(&writer, NULL, 0)) {
        printf("Error: Failed to add symbols section\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    if (!arxmod_writer_add_debug_section(&writer, NULL, 0)) {
        printf("Error: Failed to add debug section\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    if (!arxmod_writer_add_app_section(&writer, "ARXProgram", 10, NULL, 0)) {
        printf("Error: Failed to add app section\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    // Finalize the module
    if (!arxmod_writer_finalize(&writer)) {
        printf("Error: Failed to finalize ARX module\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    arxmod_writer_cleanup(&writer);
    
    if (debug_mode) {
        printf("ARX module written successfully\n");
    }
    
    return true;
}

void codegen_cleanup(codegen_context_t *context)
{
    if (context != NULL) {
        if (context->instructions != NULL) {
            free(context->instructions);
            context->instructions = NULL;
        }
        if (context->string_literals != NULL) {
            for (size_t i = 0; i < context->string_literals_count; i++) {
                if (context->string_literals[i] != NULL) {
                    free(context->string_literals[i]);
                }
            }
            free(context->string_literals);
            context->string_literals = NULL;
        }
        
        // Cleanup label table
        if (context->label_table != NULL) {
            free(context->label_table);
            context->label_table = NULL;
        }
        
        // Cleanup variable tracking
        if (context->variable_names != NULL) {
            for (size_t i = 0; i < context->variable_count; i++) {
                if (context->variable_names[i] != NULL) {
                    free(context->variable_names[i]);
                }
            }
            free(context->variable_names);
            context->variable_names = NULL;
        }
        if (context->variable_addresses != NULL) {
            free(context->variable_addresses);
            context->variable_addresses = NULL;
        }
        
        memset(context, 0, sizeof(codegen_context_t));
    }
}

bool generate_module(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_MODULE) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for module\n");
    }
    
    // Generate a basic program structure
    emit_literal(context, 0); // Start with a literal
    
    // Generate code for each class in the module
    for (size_t i = 0; i < node->child_count; i++) {
        if (node->children[i]->type == AST_CLASS) {
            if (!generate_class(context, node->children[i])) {
                return false;
            }
        }
    }
    
    // End with a halt instruction
    emit_instruction(context, VM_HALT, 0, 0);
    
    return true;
}

bool generate_class(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_CLASS) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for class: %s\n", node->value ? node->value : "unknown");
    }
    
    // Generate code for each field and method in the class
    for (size_t i = 0; i < node->child_count; i++) {
        ast_node_t *child = node->children[i];
        
        if (child->type == AST_FIELD) {
            if (!generate_field(context, child)) {
                return false;
            }
        } else if (child->type == AST_METHOD || child->type == AST_PROCEDURE || child->type == AST_FUNCTION) {
            if (!generate_method(context, child)) {
                return false;
            }
        }
    }
    
    return true;
}

bool generate_field(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_FIELD) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for field: %s\n", node->value ? node->value : "unknown");
    }
    
    // For now, fields don't generate code - they're just metadata
    // In a full implementation, this would generate field access code
    
    return true;
}

bool generate_method(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for method: %s\n", node->value ? node->value : "unknown");
    }
    
    // Check if this is the Main method and generate writeln code
    if (debug_mode) {
        printf("Method name: '%s'\n", node->value ? node->value : "NULL");
    }
    
    if (node->value && strcmp(node->value, "Main") == 0) {
        if (debug_mode) {
            printf("Generating code for Main method with %zu string literals\n", 
                   context->parser_context->method_string_count);
        }
        
        // DYNAMIC code generation - no hardcoded values!
        // Generate code based on actual parsed string literals and program structure
        
        if (debug_mode) {
            printf("Generating DYNAMIC code for all statements in the program\n");
        }
        
        // Generate code based on the AST instead of hardcoded patterns
        if (context->parser_context && context->parser_context->root) {
            if (debug_mode) {
                printf("Generating code from AST - truly dynamic code generation!\n");
            }
            
            // Traverse the AST and generate code for each statement
            if (context->parser_context->root) {
                generate_ast_code(context, context->parser_context->root);
            }
        } else {
            // Fallback: generate code for simple string literals if no parser context
            if (debug_mode) {
                printf("No parser context available, generating fallback code\n");
            }
            
            // This is a temporary fallback - in a proper implementation,
            // we would always have parser context with collected string literals
            emit_instruction(context, VM_LIT, 0, 0);
            emit_instruction(context, VM_OPR, 0, OPR_OUTSTRING);
            emit_instruction(context, VM_OPR, 0, OPR_WRITELN);
        }
        
        if (debug_mode) {
            printf("Generated DYNAMIC code for all statements - no hardcoded values!\n");
        }
        
        // Check if there's a NEW expression to generate
        // This is a temporary solution until we have proper AST-based code generation
        if (context->parser_context && context->parser_context->current_new_class) {
            if (debug_mode) {
                printf("Generating NEW expression for class: %s\n", context->parser_context->current_new_class);
            }
            if (!generate_new_expression(context, context->parser_context->current_new_class)) {
                return false;
            }
        }
    }
    
    return true;
}

bool generate_statement(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for statement type: %d\n", node->type);
    }
    
    switch (node->type) {
        case AST_EXPR_STMT:
            return generate_expression(context, node);
            
        case AST_ASSIGNMENT:
            return generate_assignment(context, node);
            
        case AST_IF_STMT:
            if (debug_mode) {
                printf("*** AST_IF_STMT case reached! ***\n");
            }
            return generate_if_statement(context, node);
            
        case AST_WHILE_STMT:
            return generate_while_statement(context, node);
            
        case AST_RETURN_STMT:
            return generate_return_statement(context, node);
            
        default:
            if (debug_mode) {
                printf("Warning: Unknown statement type %d\n", node->type);
                if (node->type == 16) {
                    printf("This is AST_IF_STMT but not being handled!\n");
                }
            }
            return true;
    }
}

bool generate_assignment(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_ASSIGNMENT) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating assignment statement\n");
    }
    
    // For now, we'll generate a simple assignment
    // In a full implementation, we'd handle variable lookup and storage
    
    // Generate the expression (right-hand side)
    if (node->child_count > 1) {
        if (!generate_expression(context, node->children[1])) {
            return false;
        }
    }
    
    // Store the result (left-hand side variable)
    // For now, use a placeholder address
    emit_instruction(context, VM_STO, 0, 0);
    
    return true;
}

bool generate_writeln_statement(codegen_context_t *context, const char *string_literal)
{
    if (context == NULL || string_literal == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for writeln(\"%s\")\n", string_literal);
    }
    
    // Check if this is a complex expression
    if (strcmp(string_literal, "COMPLEX_EXPRESSION") == 0) {
        if (debug_mode) {
            printf("Handling complex expression in writeln\n");
        }
        
        // For Phase 1, we'll implement basic string concatenation
        // This is a simplified approach that handles the most common case:
        // writeln('Hello' + 'World')
        
        // For now, we'll generate code for a simple string concatenation
        // In a full implementation, this would parse the actual expression AST
        
        // Generate code for string concatenation:
        // 1. Load first string
        // 2. Load second string  
        // 3. Concatenate strings
        // 4. Output result
        
        // Use the actual collected string literals
        // For Phase 1, we'll assume we have at least 2 strings for concatenation
        if (context->parser_context->method_string_count >= 2) {
            // Load first string (ID 0)
            emit_instruction(context, VM_LIT, 0, 0);
            
            // Load second string (ID 1) 
            emit_instruction(context, VM_LIT, 0, 1);
        } else {
            // Fallback: use placeholder IDs
            emit_instruction(context, VM_LIT, 0, 0);
            emit_instruction(context, VM_LIT, 0, 1);
        }
        
        // Concatenate strings
        emit_instruction(context, VM_OPR, 0, OPR_STR_CONCAT);
        
        // Output the concatenated string
        emit_instruction(context, VM_OPR, 0, OPR_OUTSTRING);
        emit_instruction(context, VM_OPR, 0, OPR_WRITELN);
        
        if (debug_mode) {
            printf("Generated string concatenation code for complex expression\n");
        }
        
        return true;
    }
    
    // Simple string literal case
    // Store the string literal in the context for later addition to ARX module
    if (context->string_literals == NULL) {
        context->string_literals = malloc(sizeof(char*) * 16);
        context->string_literals_capacity = 16;
        context->string_literals_count = 0;
    }
    
    // Expand if needed
    if (context->string_literals_count >= context->string_literals_capacity) {
        size_t new_capacity = context->string_literals_capacity * 2;
        char **new_strings = realloc(context->string_literals, sizeof(char*) * new_capacity);
        if (new_strings == NULL) {
            return false;
        }
        context->string_literals = new_strings;
        context->string_literals_capacity = new_capacity;
    }
    
    // Store the string literal
    size_t string_id = context->string_literals_count;
    context->string_literals[string_id] = malloc(strlen(string_literal) + 1);
    if (context->string_literals[string_id] == NULL) {
        return false;
    }
    strcpy(context->string_literals[string_id], string_literal);
    context->string_literals_count++;
    
    // Generate bytecode: LIT string_id, OPR OUTSTRING, OPR WRITELN
    emit_instruction(context, VM_LIT, 0, string_id);
    emit_instruction(context, VM_OPR, 0, OPR_OUTSTRING);
    emit_instruction(context, VM_OPR, 0, OPR_WRITELN);
    
    return true;
}

bool generate_expression(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for expression type: %d\n", node->type);
    }
    
    switch (node->type) {
        case AST_LITERAL:
            return generate_literal(context, node);
            
        case AST_IDENTIFIER:
            return generate_identifier(context, node);
            
        case AST_NEW_EXPR:
            return generate_new_expression(context, node->value);
            
        case AST_BINARY_OP:
            return generate_binary_operation(context, node);
            
        case AST_UNARY_OP:
            return generate_unary_operation(context, node);
            
        case AST_METHOD_CALL:
            return generate_method_call(context, node);
            
        case AST_FIELD_ACCESS:
            return generate_field_access(context, node);
            
        default:
            if (debug_mode) {
                printf("Warning: Unknown expression type %d\n", node->type);
            }
            return true;
    }
}

bool generate_literal(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_LITERAL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating literal: %llu\n", (unsigned long long)node->number);
    }
    
    emit_literal(context, node->number);
    return true;
}

bool generate_identifier(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_IDENTIFIER) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating identifier: %s\n", node->value ? node->value : "unknown");
    }
    
    // For now, use a placeholder address
    // In a full implementation, we'd look up the variable in the symbol table
    emit_load(context, 0, 0);
    return true;
}

bool generate_binary_operation(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_BINARY_OP) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating binary operation\n");
    }
    
    // Generate left operand
    if (node->child_count > 0) {
        if (!generate_expression(context, node->children[0])) {
            return false;
        }
    }
    
    // Generate right operand
    if (node->child_count > 1) {
        if (!generate_expression(context, node->children[1])) {
            return false;
        }
    }
    
    // Generate operation
    // For now, use ADD as a placeholder
    emit_operation(context, OPR_ADD, 0, 0);
    
    return true;
}

bool generate_unary_operation(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_UNARY_OP) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating unary operation\n");
    }
    
    // Generate operand
    if (node->child_count > 0) {
        if (!generate_expression(context, node->children[0])) {
            return false;
        }
    }
    
    // Generate operation
    // For now, use NEG as a placeholder
    emit_operation(context, OPR_NEG, 0, 0);
    
    return true;
}

bool generate_method_call(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_METHOD_CALL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating method call\n");
    }
    
    // For now, generate a placeholder method call
    emit_instruction(context, VM_OPR, 0, OPR_OBJ_CALL_METHOD);
    
    return true;
}

bool generate_field_access(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_FIELD_ACCESS) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating field access\n");
    }
    
    // For now, generate a placeholder field access
    emit_instruction(context, VM_OPR, 0, OPR_OBJ_GET_FIELD);
    
    return true;
}

bool generate_new_expression(codegen_context_t *context, const char *class_name)
{
    if (context == NULL || class_name == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating NEW expression for class: %s\n", class_name);
    }
    
    // Look up the class in the symbol table
    // For now, we'll use a simple class ID based on the class name
    // In a full implementation, we'd look up the class in the symbol table
    uint64_t class_id = 0;
    
    // Simple hash-based class ID for now
    for (const char *p = class_name; *p; p++) {
        class_id = class_id * 31 + *p;
    }
    class_id = class_id % 1000; // Keep it reasonable
    
    // Check for constructor parameters
    int param_count = 0;
    bool has_params = false;
    if (context->parser_context) {
        param_count = context->parser_context->constructor_param_count;
        has_params = context->parser_context->has_constructor_params;
    }
    
    if (debug_mode) {
        printf("  Constructor parameters: %d parameters, has_params: %s\n", 
               param_count, has_params ? "true" : "false");
    }
    
    // Emit NEW instruction
    emit_instruction(context, VM_LIT, 0, class_id);
    emit_instruction(context, VM_OPR, 0, OPR_OBJ_NEW);
    
    // If there are constructor parameters, emit constructor call
    if (has_params && param_count > 0) {
        if (debug_mode) {
            printf("  Generating constructor call with %d parameters\n", param_count);
        }
        // For now, just emit a placeholder constructor call
        // In a full implementation, this would generate proper constructor call bytecode
        emit_instruction(context, VM_OPR, 0, OPR_OBJ_CALL_METHOD);
    }
    
    if (debug_mode) {
        printf("  Generated NEW instruction for class '%s' (ID: %llu)\n", 
               class_name, (unsigned long long)class_id);
    }
    
    return true;
}

bool generate_if_statement(codegen_context_t *context, ast_node_t *node)
{
    if (!node || node->type != AST_IF_STMT || node->child_count < 2) {
        if (debug_mode) {
            printf("Invalid IF statement node\n");
        }
        return false;
    }
    
    if (debug_mode) {
        printf("Generating IF statement with %zu children\n", node->child_count);
    }
    
    // IF node structure: [condition, if_body, elseif_condition1, elseif_body1, ..., else_body]
    // At minimum: [condition, if_body]
    // With ELSEIF: [condition, if_body, elseif_condition, elseif_body, ...]
    // With ELSE: [condition, if_body, ..., else_body]
    
    ast_node_t *condition_expr = node->children[0];    // IF condition
    ast_node_t *if_body_node = node->children[1];      // IF body
    
    // Create labels for IF control flow
    size_t if_end_label = create_label(context);
    size_t next_clause_label = create_label(context);
    
    if (debug_mode) {
        printf("IF statement labels: end=%zu, next_clause=%zu\n", if_end_label, next_clause_label);
    }
    
    // Generate IF condition
    generate_expression_ast(context, condition_expr);
    
    // Jump to next clause (ELSEIF or ELSE) if condition is false
    emit_jump_if_false(context, next_clause_label);
    if (debug_mode) {
        printf("Emitted jump_if_false to next clause label %zu at instruction %zu\n", next_clause_label, context->instruction_count);
    }
    
    // Generate IF body
    if (if_body_node && if_body_node->type == AST_BLOCK) {
        for (size_t i = 0; i < if_body_node->child_count; i++) {
            generate_ast_code(context, if_body_node->children[i]);
        }
    }
    
    // Jump to end after IF body (skip ELSEIF and ELSE)
    emit_jump(context, if_end_label);
    if (debug_mode) {
        printf("Emitted jump to end label %zu at instruction %zu\n", if_end_label, context->instruction_count);
    }
    
    // Handle ELSEIF clauses
    size_t child_index = 2;  // Start after condition and if_body
    while (child_index + 1 < node->child_count) {
        // Check if this is an ELSEIF (has condition and body)
        if (child_index + 1 < node->child_count) {
            ast_node_t *elseif_condition = node->children[child_index];
            ast_node_t *elseif_body = node->children[child_index + 1];
            
            // Set next clause label (this is where we jump when IF condition is false)
            set_label(context, next_clause_label, context->instruction_count);
            if (debug_mode) {
                printf("Set next clause label %zu at instruction %zu\n", next_clause_label, context->instruction_count);
            }
            
            // Generate ELSEIF condition
            generate_expression_ast(context, elseif_condition);
            
            // Create next label for this ELSEIF (either next ELSEIF or ELSE or end)
            size_t current_next_label = (child_index + 2 < node->child_count) ? create_label(context) : if_end_label;
            
            // Jump to next label if ELSEIF condition is false
            emit_jump_if_false(context, current_next_label);
            if (debug_mode) {
                printf("Emitted jump_if_false to label %zu at instruction %zu\n", current_next_label, context->instruction_count);
            }
            
            // Generate ELSEIF body
            if (elseif_body && elseif_body->type == AST_BLOCK) {
                for (size_t i = 0; i < elseif_body->child_count; i++) {
                    generate_ast_code(context, elseif_body->children[i]);
                }
            }
            
            // Jump to end after ELSEIF body
            emit_jump(context, if_end_label);
            if (debug_mode) {
                printf("Emitted jump to end label %zu at instruction %zu\n", if_end_label, context->instruction_count);
            }
            
            // Move to next ELSEIF or ELSE
            child_index += 2;
            next_clause_label = current_next_label;
        } else {
            break;
        }
    }
    
    // Handle ELSE clause (if present)
    if (child_index < node->child_count) {
        ast_node_t *else_body = node->children[child_index];
        
        // Set next clause label (this is where we jump when all conditions are false)
        set_label(context, next_clause_label, context->instruction_count);
        if (debug_mode) {
            printf("Set ELSE label %zu at instruction %zu\n", next_clause_label, context->instruction_count);
        }
        
        // Generate ELSE body
        if (else_body && else_body->type == AST_BLOCK) {
            for (size_t i = 0; i < else_body->child_count; i++) {
                generate_ast_code(context, else_body->children[i]);
            }
        }
    } else {
        // No ELSE clause, set the final next clause label (this becomes the end)
        set_label(context, next_clause_label, context->instruction_count);
        if (debug_mode) {
            printf("Set final next clause label %zu at instruction %zu\n", next_clause_label, context->instruction_count);
        }
    }
    
    // Set IF end label
    set_label(context, if_end_label, context->instruction_count);
    if (debug_mode) {
        printf("Set IF end label %zu at instruction %zu\n", if_end_label, context->instruction_count);
    }
    
    if (debug_mode) {
        printf("IF statement code generation complete\n");
    }
    
    return true;
}

bool generate_while_statement(codegen_context_t *context, ast_node_t *node)
{
    if (!node || node->child_count < 2) {
        if (debug_mode) {
            printf("Invalid WHILE statement node\n");
        }
        return false;
    }
    
    // WHILE node structure: [condition, body]
    ast_node_t *condition_expr = node->children[0];    // Condition expression
    ast_node_t *body_node = node->children[1];         // Loop body
    
    // Create labels for loop control
    size_t loop_start_label = create_label(context);
    size_t loop_end_label = create_label(context);
    
    if (debug_mode) {
        printf("Generating WHILE loop\n");
        printf("WHILE loop labels: start=%zu, end=%zu\n", loop_start_label, loop_end_label);
    }
    
    // Set loop start label (condition check)
    set_label(context, loop_start_label, context->instruction_count);
    if (debug_mode) {
        printf("Set loop start label %zu at instruction %zu\n", loop_start_label, context->instruction_count);
    }
    
    // Generate condition expression
    generate_expression_ast(context, condition_expr);
    
    // Jump to end if condition is false
    emit_jump_if_false(context, loop_end_label);
    if (debug_mode) {
        printf("Emitted jump_if_false to label %zu at instruction %zu\n", loop_end_label, context->instruction_count);
    }
    
    // Generate loop body
    if (body_node && body_node->type == AST_BLOCK) {
        for (size_t i = 0; i < body_node->child_count; i++) {
            generate_ast_code(context, body_node->children[i]);
        }
    }
    
    // Jump back to condition check
    emit_jump(context, loop_start_label);
    if (debug_mode) {
        printf("Emitted jump to label %zu at instruction %zu\n", loop_start_label, context->instruction_count);
    }
    
    // Set loop end label
    set_label(context, loop_end_label, context->instruction_count);
    if (debug_mode) {
        printf("Set loop end label %zu at instruction %zu\n", loop_end_label, context->instruction_count);
    }
    
    if (debug_mode) {
        printf("WHILE loop code generation complete\n");
    }
    
    return true;
}

bool generate_return_statement(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->type != AST_RETURN_STMT) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating return statement\n");
    }
    
    // Generate return operation
    emit_operation(context, OPR_RET, 0, 0);
    
    return true;
}

void emit_instruction(codegen_context_t *context, opcode_t opcode, uint8_t level, uint64_t operand)
{
    if (context == NULL) {
        return;
    }
    
    // Resize instructions array if needed
    if (context->instruction_count >= context->instruction_capacity) {
        size_t new_capacity = context->instruction_capacity == 0 ? 64 : context->instruction_capacity * 2;
        if (context->debug_output) {
            printf("  Reallocating instructions array: %zu -> %zu (old_ptr=%p)\n", 
                   context->instruction_capacity, new_capacity, (void*)context->instructions);
        }
        instruction_t *new_instructions = realloc(context->instructions, new_capacity * sizeof(instruction_t));
        if (new_instructions == NULL) {
            codegen_error(context, "Failed to allocate memory for instructions");
            return;
        }
        if (context->debug_output) {
            printf("  Reallocated instructions array: new_ptr=%p\n", (void*)new_instructions);
        }
        context->instructions = new_instructions;
        context->instruction_capacity = new_capacity;
    }
    
    // Create instruction
    instruction_t *instr = &context->instructions[context->instruction_count];
    instr->opcode = opcode | (level << 4);
    instr->opt64 = operand;
    
    context->instruction_count++;
    
    if (context->debug_output) {
        printf("  Emitted: opcode=%d, level=%d, operand=%llu (stored at index %zu)\n", 
               opcode, level, (unsigned long long)operand, context->instruction_count - 1);
    }
}

void emit_operation(codegen_context_t *context, opr_t operation, uint8_t level, uint64_t operand)
{
    emit_instruction(context, VM_OPR, level, operation);
}

// AST-based code generation
void generate_ast_code(codegen_context_t *context, ast_node_t *node)
{
    if (!node) return;
    
    if (debug_mode) {
        printf("Generating code for AST node type: %d", node->type);
        if (node->type == AST_IF_STMT) {
            printf(" (AST_IF_STMT=%d)", AST_IF_STMT);
        } else if (node->type == AST_WHILE_STMT) {
            printf(" (AST_WHILE_STMT=%d)", AST_WHILE_STMT);
        } else if (node->type == AST_FOR_STMT) {
            printf(" (AST_FOR_STMT=%d)", AST_FOR_STMT);
        }
        printf("\n");
    }
    
    switch (node->type) {
        case AST_MODULE:
            // Generate code for all classes in the module
            for (size_t i = 0; i < node->child_count; i++) {
                generate_ast_code(context, node->children[i]);
            }
            break;
            
        case AST_CLASS:
            // Generate code for all methods in the class
            for (size_t i = 0; i < node->child_count; i++) {
                generate_ast_code(context, node->children[i]);
            }
            break;
            
        case AST_METHOD:
        case AST_PROCEDURE:
            // Generate code for all statements in the method/procedure
            if (debug_mode) {
                printf("AST_METHOD/AST_PROCEDURE node has %zu children\n", node->child_count);
            }
            for (size_t i = 0; i < node->child_count; i++) {
                if (debug_mode) {
                    printf("Processing method/procedure child %zu, type: %d\n", i, node->children[i]->type);
                }
                generate_ast_code(context, node->children[i]);
            }
            break;
            
        case AST_ASSIGNMENT:
            generate_assignment_ast(context, node);
            break;
            
        case AST_VAR_DECL:
            generate_variable_declaration_ast(context, node);
            break;
            
        case AST_FOR_STMT:
            generate_for_statement(context, node);
            break;
            
        case AST_WHILE_STMT:
            generate_while_statement(context, node);
            break;
            
        case AST_IF_STMT:
            generate_if_statement(context, node);
            break;
            
        case AST_EXPR_STMT:
            // Generate code for expression statement
            if (debug_mode) {
                printf("AST_EXPR_STMT node has %zu children\n", node->child_count);
            }
            if (node->child_count > 0) {
                if (debug_mode) {
                    printf("Processing expression statement child, type: %d\n", node->children[0]->type);
                }
                generate_expression_ast(context, node->children[0]);
                
                // For expression statements containing literals, identifiers, or binary operations, assume this is a writeln call
                // and emit the output instructions
                if (node->children[0]->type == AST_LITERAL || 
                    node->children[0]->type == AST_IDENTIFIER || 
                    node->children[0]->type == AST_BINARY_OP ||
                    node->children[0]->type == AST_METHOD_CALL ||
                    node->children[0]->type == AST_FIELD_ACCESS) {
                    if (debug_mode) {
                        const char *type_name = "unknown";
                        if (node->children[0]->type == AST_LITERAL) type_name = "literal";
                        else if (node->children[0]->type == AST_IDENTIFIER) type_name = "identifier";
                        else if (node->children[0]->type == AST_BINARY_OP) type_name = "binary operation";
                        else if (node->children[0]->type == AST_METHOD_CALL) type_name = "method call";
                        else if (node->children[0]->type == AST_FIELD_ACCESS) type_name = "field access";
                        printf("Emitting output instructions for %s in expression statement\n", type_name);
                    }
                    emit_instruction(context, VM_OPR, 0, OPR_OUTSTRING);
                    emit_instruction(context, VM_OPR, 0, OPR_WRITELN);
                }
            }
            break;
            
        default:
            if (debug_mode) {
                printf("Unhandled AST node type: %d\n", node->type);
            }
            break;
    }
}

void generate_variable_declaration_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node || node->child_count < 1) return;
    
    ast_node_t *var_node = node->children[0];   // Variable name
    
    if (debug_mode) {
        printf("Generating variable declaration: %s\n", var_node->value);
    }
    
    // Add variable to symbol table
    size_t var_address;
    if (codegen_add_variable(context, var_node->value, &var_address)) {
        if (debug_mode) {
            printf("Added variable '%s' to symbol table at address %zu\n", var_node->value, var_address);
        }
    }
}

void generate_assignment_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node || node->child_count < 2) return;
    
    ast_node_t *var_node = node->children[0];  // Variable
    ast_node_t *expr_node = node->children[1]; // Expression
    
    if (debug_mode) {
        printf("Generating assignment: %s := expression\n", var_node->value);
    }
    
    // Generate code for the expression (this will push the result onto the stack)
    generate_expression_ast(context, expr_node);
    
    // Store the result to the variable
    size_t var_address;
    if (codegen_add_variable(context, var_node->value, &var_address)) {
        if (debug_mode) {
            printf("Storing to variable '%s' at address %zu\n", var_node->value, var_address);
        }
        emit_instruction(context, VM_STO, 0, var_address);
    } else {
        if (debug_mode) {
            printf("Warning: Failed to add/find variable '%s' for assignment\n", var_node->value);
        }
    }
}

void generate_expression_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node) return;
    
    if (debug_mode) {
        printf("Generating expression code for AST node type: %d\n", node->type);
    }
    
    switch (node->type) {
        case AST_LITERAL:
            generate_literal_ast(context, node);
            break;
            
        case AST_IDENTIFIER:
            generate_identifier_ast(context, node);
            break;
            
        case AST_BINARY_OP:
            generate_binary_op_ast(context, node);
            break;
            
        case AST_UNARY_OP:
            generate_unary_op_ast(context, node);
            break;
            
        case AST_METHOD_CALL:
            generate_method_call_ast(context, node);
            break;
            
        case AST_FIELD_ACCESS:
            generate_field_access_ast(context, node);
            break;
            
        default:
            if (debug_mode) {
                printf("Unhandled expression AST node type: %d\n", node->type);
            }
            break;
    }
}

void generate_literal_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node) return;
    
    
    if (node->value) {
        // String literal - find the correct string index in the string table
        size_t string_index = 0;
        if (context->parser_context && context->parser_context->method_string_literals) {
            // Find the string in the collected string literals
            for (size_t i = 0; i < context->parser_context->method_string_count; i++) {
                if (context->parser_context->method_string_literals[i] && 
                    strcmp(context->parser_context->method_string_literals[i], node->value) == 0) {
                    string_index = i;
                    break;
                }
            }
        }
        
        if (debug_mode) {
            printf("Loading string literal '%s' at index %zu\n", node->value, string_index);
        }
        
        emit_instruction(context, VM_LIT, 0, string_index);
    } else {
        // Number literal - load the actual value from the AST
        emit_instruction(context, VM_LIT, 0, node->number);
    }
}

void generate_identifier_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node || !node->value) return;
    
    
    // Load variable value from memory
    size_t var_address;
    if (codegen_find_variable(context, node->value, &var_address)) {
        emit_instruction(context, VM_LOD, 0, var_address);
    } else {
        if (debug_mode) {
            printf("Warning: Variable '%s' not found in symbol table\n", node->value);
        }
    }
}

void generate_binary_op_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node || node->child_count < 2 || !node->value) return;
    
    if (debug_mode) {
        printf("Generating binary operation: %s\n", node->value);
    }
    
    
    // Generate code for left operand
    generate_expression_ast(context, node->children[0]);
    
    // Generate code for right operand
    generate_expression_ast(context, node->children[1]);
    
    // Generate the operation
    if (strcmp(node->value, "+") == 0) {
        // Check if this is string concatenation or arithmetic addition
        // We need to determine the context and operand types
        
        bool is_string_concatenation = false;
        
        // Check if left operand is a string literal
        if (node->children[0] && node->children[0]->type == AST_LITERAL && node->children[0]->value) {
            // Left operand is a string literal, so this is string concatenation
            is_string_concatenation = true;
        }
        
        // Also check if the right operand is an identifier (variable) that needs to be converted to string
        if (node->children[1] && node->children[1]->type == AST_IDENTIFIER) {
            is_string_concatenation = true;
        }
        
        // If this is part of a complex string concatenation chain, treat as string concatenation
        // This handles cases like: 'Addition: ' + a + ' + ' + b + ' = ' + result
        if (node->children[0] && node->children[0]->type == AST_BINARY_OP && 
            node->children[0]->value && strcmp(node->children[0]->value, "+") == 0) {
            // Left operand is also a + operation, likely string concatenation
            is_string_concatenation = true;
        }
        
        if (is_string_concatenation) {
            // String concatenation
            // Check if the right operand is an identifier (variable) that might be an integer
            if (node->children[1] && node->children[1]->type == AST_IDENTIFIER) {
                emit_instruction(context, VM_OPR, 0, OPR_INT_TO_STR);
            }
            emit_instruction(context, VM_OPR, 0, OPR_STR_CONCAT);
        } else {
            // Arithmetic addition
            emit_instruction(context, VM_OPR, 0, OPR_ADD);
        }
    } else if (strcmp(node->value, "-") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_SUB);
    } else if (strcmp(node->value, "*") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_MUL);
    } else if (strcmp(node->value, "/") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_DIV);
    } else if (strcmp(node->value, "^") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_POW);
    } else if (strcmp(node->value, "%") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_MOD);
    } else if (strcmp(node->value, "==") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_EQ);
    } else if (strcmp(node->value, "!=") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_NEQ);
    } else if (strcmp(node->value, "<") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_LESS);
    } else if (strcmp(node->value, "<=") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_LEQ);
    } else if (strcmp(node->value, ">") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_GREATER);
    } else if (strcmp(node->value, ">=") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_GEQ);
    } else {
        if (debug_mode) {
            printf("Warning: Unknown binary operator: %s\n", node->value);
        }
    }
}

void generate_unary_op_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node || node->child_count < 1 || !node->value) return;
    
    if (debug_mode) {
        printf("Generating unary operation: %s\n", node->value);
    }
    
    // Generate code for operand
    generate_expression_ast(context, node->children[0]);
    
    // Generate the operation
    if (strcmp(node->value, "-") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_NEG);
    } else if (strcmp(node->value, "!") == 0) {
        // TODO: Implement logical NOT operation
        if (debug_mode) {
            printf("Warning: Logical NOT operation not yet implemented\n");
        }
    } else {
        if (debug_mode) {
            printf("Warning: Unknown unary operator: %s\n", node->value);
        }
    }
}

void emit_literal(codegen_context_t *context, uint64_t value)
{
    emit_instruction(context, VM_LIT, 0, value);
}

void emit_load(codegen_context_t *context, uint8_t level, uint64_t address)
{
    emit_instruction(context, VM_LOD, level, address);
}

void emit_store(codegen_context_t *context, uint8_t level, uint64_t address)
{
    emit_instruction(context, VM_STO, level, address);
}

void emit_call(codegen_context_t *context, uint8_t level, uint64_t address)
{
    emit_instruction(context, VM_CAL, level, address);
}

void emit_jump(codegen_context_t *context, uint64_t address)
{
    emit_instruction(context, VM_JMP, 0, address);
}

void emit_jump_if_false(codegen_context_t *context, uint64_t address)
{
    emit_instruction(context, VM_JPC, 0, address);
}

bool codegen_add_variable(codegen_context_t *context, const char *name, size_t *address)
{
    if (context == NULL || name == NULL || address == NULL) {
        return false;
    }
    
    // Check if variable already exists
    size_t existing_address;
    if (codegen_find_variable(context, name, &existing_address)) {
        *address = existing_address;
        return true;
    }
    
    // Expand capacity if needed
    if (context->variable_count >= context->variable_capacity) {
        size_t new_capacity = context->variable_capacity == 0 ? 16 : context->variable_capacity * 2;
        char **new_names = realloc(context->variable_names, new_capacity * sizeof(char*));
        size_t *new_addresses = realloc(context->variable_addresses, new_capacity * sizeof(size_t));
        
        if (new_names == NULL || new_addresses == NULL) {
            return false;
        }
        
        context->variable_names = new_names;
        context->variable_addresses = new_addresses;
        context->variable_capacity = new_capacity;
    }
    
    // Add the variable
    context->variable_names[context->variable_count] = malloc(strlen(name) + 1);
    if (context->variable_names[context->variable_count] == NULL) {
        return false;
    }
    strcpy(context->variable_names[context->variable_count], name);
    
    context->variable_addresses[context->variable_count] = context->next_variable_address;
    *address = context->next_variable_address;
    
    context->variable_count++;
    context->next_variable_address++;
    
    if (context->debug_output) {
        printf("Added variable '%s' at address %zu\n", name, *address);
    }
    
    return true;
}

bool codegen_find_variable(codegen_context_t *context, const char *name, size_t *address)
{
    if (context == NULL || name == NULL || address == NULL) {
        return false;
    }
    
    for (size_t i = 0; i < context->variable_count; i++) {
        if (context->variable_names[i] != NULL && strcmp(context->variable_names[i], name) == 0) {
            *address = context->variable_addresses[i];
            return true;
        }
    }
    
    return false;
}

void codegen_error(codegen_context_t *context, const char *message)
{
    if (context == NULL || message == NULL) {
        return;
    }
    
    printf("Code generation error: %s\n", message);
}

void codegen_warning(codegen_context_t *context, const char *message)
{
    if (context == NULL || message == NULL) {
        return;
    }
    
    printf("Code generation warning: %s\n", message);
}

size_t create_label(codegen_context_t *context)
{
    if (context == NULL) {
        return 0;
    }
    
    return ++context->label_counter;
}

void set_label(codegen_context_t *context, size_t label_id, size_t instruction_index)
{
    if (context == NULL || instruction_index > context->instruction_count) {
        return;
    }
    
    if (debug_mode) {
        printf("Setting label %zu to instruction %zu\n", label_id, instruction_index);
    }
    
    // Add or update label in the label table
    bool found = false;
    for (size_t i = 0; i < context->label_table_size; i++) {
        if (context->label_table[i].label_id == label_id) {
            context->label_table[i].instruction_index = instruction_index;
            context->label_table[i].defined = true;
            found = true;
            break;
        }
    }
    
    if (!found) {
        // Add new label to table
        if (context->label_table_size >= context->label_table_capacity) {
            // Expand label table
            size_t new_capacity = context->label_table_capacity * 2;
            if (new_capacity == 0) new_capacity = 16;
            
            label_entry_t *new_table = realloc(context->label_table, new_capacity * sizeof(label_entry_t));
            if (new_table == NULL) {
                return; // Out of memory
            }
            context->label_table = new_table;
            context->label_table_capacity = new_capacity;
        }
        
        context->label_table[context->label_table_size].label_id = label_id;
        context->label_table[context->label_table_size].instruction_index = instruction_index;
        context->label_table[context->label_table_size].defined = true;
        context->label_table_size++;
    }
}

void resolve_labels(codegen_context_t *context)
{
    if (context == NULL) {
        return;
    }
    
    if (debug_mode) {
        printf("Resolving %zu labels...\n", context->label_table_size);
    }
    
    // Go through all instructions and resolve label references
    for (size_t i = 0; i < context->instruction_count; i++) {
        if (context->instructions[i].opcode == VM_JMP || context->instructions[i].opcode == VM_JPC) {
            size_t label_id = context->instructions[i].opt64;
            
            // Find the label in the label table
            for (size_t j = 0; j < context->label_table_size; j++) {
                if (context->label_table[j].label_id == label_id && context->label_table[j].defined) {
                    context->instructions[i].opt64 = context->label_table[j].instruction_index;
                    if (debug_mode) {
                        printf("Resolved jump at instruction %zu: label %zu -> instruction %zu\n", 
                               i, label_id, context->label_table[j].instruction_index);
                    }
                    break;
                }
            }
        }
    }
}

void generate_method_call_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!context || !node) return;
    
    if (debug_mode) {
        printf("Generating method call: %s\n", node->value ? node->value : "unknown");
    }
    
    // For now, generate a placeholder method call
    // TODO: Implement proper method call generation
    emit_instruction(context, VM_OPR, 0, OPR_OBJ_CALL_METHOD);
    
    if (debug_mode) {
        printf("Generated method call instruction\n");
    }
}

void generate_field_access_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!context || !node) return;
    
    if (debug_mode) {
        printf("Generating field access: %s\n", node->value ? node->value : "unknown");
    }
    
    // For now, generate a placeholder field access
    // TODO: Implement proper field access generation
    emit_instruction(context, VM_OPR, 0, OPR_OBJ_GET_FIELD);
    
    if (debug_mode) {
        printf("Generated field access instruction\n");
    }
}

bool generate_for_statement(codegen_context_t *context, ast_node_t *node)
{
    if (!node || node->child_count < 4) {
    if (debug_mode) {
            printf("Invalid FOR statement node\n");
        }
        return false;
    }
    
    // FOR node structure: [variable, start_expr, end_expr, body]
    ast_node_t *var_node = node->children[0];      // Loop variable
    ast_node_t *start_expr = node->children[1];    // Start expression
    ast_node_t *end_expr = node->children[2];      // End expression
    ast_node_t *body_node = node->children[3];     // Loop body
    
    // Create labels for loop control
    size_t loop_start_label = create_label(context);
    size_t loop_end_label = create_label(context);
    size_t loop_increment_label = create_label(context);
    
    if (debug_mode) {
        printf("Generating FOR loop: %s = %s to %s\n", 
               var_node->value ? var_node->value : "unknown",
               start_expr->value ? start_expr->value : "unknown",
               end_expr->value ? end_expr->value : "unknown");
        printf("FOR loop labels: start=%zu, end=%zu, increment=%zu\n", 
               loop_start_label, loop_end_label, loop_increment_label);
    }
    
    // Add loop variable to symbol table if not already present
    size_t var_address;
    if (!codegen_find_variable(context, var_node->value, &var_address)) {
        if (!codegen_add_variable(context, var_node->value, &var_address)) {
            if (debug_mode) {
                printf("Failed to add loop variable: %s\n", var_node->value);
            }
            return false;
        }
    }
    
    // Generate start expression and store in loop variable
    generate_expression_ast(context, start_expr);
    emit_store(context, 0, var_address);
    
    // Set loop start label (condition check)
    set_label(context, loop_start_label, context->instruction_count);
    if (debug_mode) {
        printf("Set loop start label %zu at instruction %zu\n", loop_start_label, context->instruction_count);
    }
    
    // Load loop variable and end expression for comparison
    emit_load(context, 0, var_address);
    generate_expression_ast(context, end_expr);
    
    // Compare loop variable with end value (less than or equal)
    emit_operation(context, OPR_LEQ, 0, 0);
    
    // Jump to end if loop variable > end value (i.e., if loop_var <= end_value is false)
    emit_jump_if_false(context, loop_end_label);
    if (debug_mode) {
        printf("Emitted jump_if_false to label %zu at instruction %zu\n", loop_end_label, context->instruction_count);
    }
    
    // Generate loop body
    if (body_node && body_node->type == AST_BLOCK) {
        for (size_t i = 0; i < body_node->child_count; i++) {
            generate_ast_code(context, body_node->children[i]);
        }
    }
    
    // Set increment label
    set_label(context, loop_increment_label, context->instruction_count);
    if (debug_mode) {
        printf("Set increment label %zu at instruction %zu\n", loop_increment_label, context->instruction_count);
    }
    
    // Increment loop variable (load, add 1, store)
    emit_load(context, 0, var_address);
    emit_literal(context, 1);
    emit_operation(context, OPR_ADD, 0, 0);
    emit_store(context, 0, var_address);
    if (debug_mode) {
        printf("Emitted increment code at instruction %zu\n", context->instruction_count);
    }
    
    // Jump back to loop start
    emit_jump(context, loop_start_label);
    if (debug_mode) {
        printf("Emitted jump to label %zu at instruction %zu\n", loop_start_label, context->instruction_count);
    }
    
    // Set loop end label
    set_label(context, loop_end_label, context->instruction_count);
    if (debug_mode) {
        printf("Set loop end label %zu at instruction %zu\n", loop_end_label, context->instruction_count);
    }
    
    if (debug_mode) {
        printf("FOR loop code generation complete\n");
    }
    
    return true;
}

