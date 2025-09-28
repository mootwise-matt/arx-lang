/*
 * ARX Code Generator Implementation
 * Generates bytecode instructions from AST
 */

#include "codegen.h"
#include "../arxmod/arxmod.h"
#include "../linker/linker.h"
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
    
    // Initialize class context
    context->current_class = NULL;
    context->current_class_name = NULL;
    
    // Initialize method position tracking
    context->method_positions = NULL;
    context->method_position_count = 0;
    context->method_position_capacity = 0;
    
    // String literals will be collected directly during AST traversal
    // No need for complex transfer mechanism
    
    if (debug_mode) {
        printf("Code generator initialized with %zu string literals\n", context->string_literals_count);
    }
    
    return true;
}

// Method position tracking functions
bool codegen_start_method_tracking(codegen_context_t *context, const char *method_name)
{
    if (!context || !method_name) {
        return false;
    }
    
    // Expand method positions array if needed
    if (context->method_position_count >= context->method_position_capacity) {
        size_t new_capacity = context->method_position_capacity == 0 ? 8 : context->method_position_capacity * 2;
        void *new_positions = realloc(context->method_positions, new_capacity * sizeof(*context->method_positions));
        if (!new_positions) {
            return false;
        }
        context->method_positions = new_positions;
        context->method_position_capacity = new_capacity;
    }
    
    // Add new method position entry
    size_t index = context->method_position_count;
    context->method_positions[index].method_name = strdup(method_name);
    context->method_positions[index].start_instruction = context->instruction_count;
    context->method_positions[index].end_instruction = 0; // Will be set when method ends
    context->method_position_count++;
    
    if (debug_mode) {
        printf("Started tracking method '%s' at instruction %zu\n", method_name, context->instruction_count);
        printf("  Method position tracking: total methods tracked: %zu\n", context->method_position_count);
    }
    
    return true;
}

bool codegen_end_method_tracking(codegen_context_t *context, const char *method_name)
{
    if (!context || !method_name) {
        return false;
    }
    
    // Find the method position entry
    for (size_t i = 0; i < context->method_position_count; i++) {
        if (strcmp(context->method_positions[i].method_name, method_name) == 0) {
            context->method_positions[i].end_instruction = context->instruction_count;
            
            if (debug_mode) {
                printf("Ended tracking method '%s' at instruction %zu (started at %zu)\n", 
                       method_name, context->instruction_count, context->method_positions[i].start_instruction);
            }
            return true;
        }
    }
    
    if (debug_mode) {
        printf("Warning: Method '%s' not found in tracking table\n", method_name);
    }
    return false;
}

size_t codegen_get_method_offset(codegen_context_t *context, const char *method_name)
{
    if (!context || !method_name) {
        return 0;
    }
    
    // Find the method position entry
    for (size_t i = 0; i < context->method_position_count; i++) {
        if (strcmp(context->method_positions[i].method_name, method_name) == 0) {
            return context->method_positions[i].start_instruction;
        }
    }
    
    if (debug_mode) {
        printf("Warning: Method '%s' not found in tracking table\n", method_name);
    }
    return 0;
}

// Unique class ID generation function
uint64_t codegen_generate_unique_class_id(const char *module_name, const char *class_name)
{
    if (!module_name || !class_name) {
        return 0;
    }
    
    // Use a combination of module name and class name to generate a unique hash
    // This ensures build-time uniqueness across different modules
    uint64_t hash = 0;
    
    // Hash the module name
    for (const char *p = module_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    
    // Add a separator to distinguish module from class
    hash = hash * 31 + ':';
    
    // Hash the class name
    for (const char *p = class_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    
    if (debug_mode) {
        printf("Generated unique class ID for module '%s', class '%s': %llu\n", 
               module_name, class_name, (unsigned long long)hash);
    }
    
    return hash;
}

uint64_t codegen_generate_unique_method_id(const char *module_name, const char *class_name, 
                                          const char *method_name, const char *param_types, 
                                          const char *return_type)
{
    if (!module_name || !class_name || !method_name) {
        return 0;
    }
    
    // Create a hash from module name, class name, method name, parameter types, and return type
    uint64_t hash = 0;
    
    // Hash the module name
    for (const char *p = module_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    
    // Add separator
    hash = hash * 31 + ':';
    
    // Hash the class name
    for (const char *p = class_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    
    // Add separator
    hash = hash * 31 + '.';
    
    // Hash the method name
    for (const char *p = method_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    
    // Add parameter types if provided
    if (param_types) {
        hash = hash * 31 + '(';
        for (const char *p = param_types; *p; p++) {
            hash = hash * 31 + *p;
        }
        hash = hash * 31 + ')';
    }
    
    // Add return type if provided (for functions)
    if (return_type) {
        hash = hash * 31 + ':';
        for (const char *p = return_type; *p; p++) {
            hash = hash * 31 + *p;
        }
    }
    
    if (debug_mode) {
        printf("Generated unique method ID for module '%s', class '%s', method '%s' (params: '%s', return: '%s'): %llu\n", 
               module_name, class_name, method_name, 
               param_types ? param_types : "none", 
               return_type ? return_type : "NULL", 
               (unsigned long long)hash);
        printf("  Hash components: module='%s' (%zu), class='%s' (%zu), method='%s' (%zu), params='%s' (%zu), return='%s' (%zu)\n",
               module_name, strlen(module_name),
               class_name, strlen(class_name),
               method_name, strlen(method_name),
               param_types ? param_types : "none", param_types ? strlen(param_types) : 4,
               return_type ? return_type : "NULL", return_type ? strlen(return_type) : 4);
    }
    
    return hash;
}

bool codegen_generate(codegen_context_t *context, ast_node_t *ast, 
                     instruction_t **instructions, size_t *instruction_count)
{
    printf("CODEGEN_GENERATE: Starting code generation\n");
    printf("CODEGEN_GENERATE: AST node type=%d, value='%s', children=%zu\n", 
           ast->type, ast->value ? ast->value : "NULL", ast->child_count);
    if (context == NULL || ast == NULL || instructions == NULL || instruction_count == NULL) {
        printf("CODEGEN_GENERATE: NULL parameters, returning false\n");
        return false;
    }
    
    if (debug_mode) {
        printf("Starting code generation...\n");
    }
    
    // Generate code for the AST using AST-based approach
    printf("CODEGEN_GENERATE: Calling generate_ast_code to process AST\n");
    generate_ast_code(context, ast);
    
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
    
    // Enable debug output for arxmod writer
    writer.debug_output = true;
    
    if (debug_mode) {
        printf("Writing ARX module to '%s'\n", filename);
    }
    
    // Detect if this module has an entry point (App.Main)
    bool has_entry_point = false;
    if (context->parser_context && context->parser_context->root) {
        has_entry_point = detect_entry_point(context->parser_context->root);
    }
    
    // Set module flags based on whether it has an entry point
    uint32_t module_flags = has_entry_point ? ARXMOD_FLAG_EXECUTABLE : ARXMOD_FLAG_LIBRARY;
    if (!arxmod_writer_set_flags(&writer, module_flags)) {
        printf("Error: Failed to set module flags\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    // Set entry point if this is an executable module
    if (has_entry_point) {
        // Entry point will be set by linker after method offsets are calculated
        // For now, set to 0 - linker will update it
        if (!arxmod_writer_set_entry_point(&writer, 0)) {
            printf("Error: Failed to set entry point\n");
            arxmod_writer_cleanup(&writer);
            return false;
        }
    }
    
    if (debug_mode) {
        printf("Module type: %s (flags: 0x%08x)\n", 
               has_entry_point ? "EXECUTABLE" : "LIBRARY", module_flags);
    }
    
    // Write header
    if (!arxmod_writer_write_header(&writer, "ARXProgram", 10)) {
        printf("Error: Failed to write ARX module header\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    // Linker phase: resolve method and field addresses
    printf("DEBUG: Checking linker condition - parser_context=%p, root=%p\n", 
           (void*)context->parser_context, 
           context->parser_context ? (void*)context->parser_context->root : NULL);
    
    if (context->parser_context && context->parser_context->root) {
        printf("Running linker phase...\n");
        
        // Collect classes from AST (same as for classes section)
        class_entry_t *classes = NULL;
        size_t class_count = 0;
        method_entry_t *methods = NULL;
        size_t method_count = 0;
        field_entry_t *fields = NULL;
        size_t field_count = 0;
        if (!collect_classes_from_ast(context, context->parser_context->root, &classes, &class_count, &methods, &method_count, &fields, &field_count)) {
            printf("Error: Failed to collect classes for linker\n");
            arxmod_writer_cleanup(&writer);
            return false;
        }
        
        printf("DEBUG: class_count = %zu, method_count = %zu, field_count = %zu\n", class_count, method_count, field_count);
        
        if (class_count > 0) {
            printf("DEBUG: Running linker with %zu classes, %zu methods, %zu fields\n", class_count, method_count, field_count);
            printf("DEBUG: Bytecode size: %zu bytes, String count: %zu\n", instruction_count * sizeof(instruction_t), context->string_literals_count);
            
            // Initialize linker
            linker_context_t linker;
            if (!linker_init(&linker, classes, class_count, methods, method_count, fields, field_count)) {
                printf("Error: Failed to initialize linker\n");
                if (classes) free(classes);
                if (methods) free(methods);
                if (fields) free(fields);
                arxmod_writer_cleanup(&writer);
                return false;
            }
            
            // Patch bytecode with resolved addresses
            // Use centralized string literals
            const char **strings_to_patch = NULL;
            size_t string_count_to_patch = 0;
            
            if (context->string_literals && context->string_literals_count > 0) {
                strings_to_patch = (const char**)context->string_literals;
                string_count_to_patch = context->string_literals_count;
            }
            
            if (!linker_patch_bytecode(&linker, instructions, instruction_count, 
                                       strings_to_patch, string_count_to_patch)) {
                printf("Error: Failed to patch bytecode\n");
                linker_cleanup(&linker);
                if (classes) free(classes);
                if (methods) free(methods);
                if (fields) free(fields);
                arxmod_writer_cleanup(&writer);
                return false;
            }
            
            // Update class manifest with correct method offsets
            if (!linker_update_class_manifest(&linker, instructions, instruction_count)) {
                printf("Error: Failed to update class manifest\n");
                linker_cleanup(&linker);
                if (classes) free(classes);
                if (methods) free(methods);
                if (fields) free(fields);
                arxmod_writer_cleanup(&writer);
                return false;
            }
            
            // Set entry point if this is an executable module
            if (has_entry_point) {
                // Find the Main method offset from the linker's method list
                uint64_t main_method_offset = 0;
                for (size_t i = 0; i < linker.method_count; i++) {
                    if (strcmp(linker.methods[i].method_name, "Main") == 0) {
                        main_method_offset = linker.methods[i].offset;
                        break;
                    }
                }
                
                if (main_method_offset != 0) {
                    if (!arxmod_writer_set_entry_point(&writer, main_method_offset)) {
                        printf("Error: Failed to set entry point\n");
                        linker_cleanup(&linker);
                        if (classes) free(classes);
                        if (methods) free(methods);
                        if (fields) free(fields);
                        arxmod_writer_cleanup(&writer);
                        return false;
                    }
                    
                    if (debug_mode) {
                        printf("Linker: Set entry point to Main method at offset %llu\n", 
                               (unsigned long long)main_method_offset);
                    }
                    
                    // Update the header in the file with the correct entry point
                    if (!arxmod_writer_update_header(&writer)) {
                        printf("Error: Failed to update header with entry point\n");
                        linker_cleanup(&linker);
                        if (classes) free(classes);
                        if (methods) free(methods);
                        if (fields) free(fields);
                        arxmod_writer_cleanup(&writer);
                        return false;
                    }
                } else {
                    printf("Warning: Executable module but Main method not found\n");
                }
            }
            
            // Cleanup linker
            linker_cleanup(&linker);
            printf("Linker phase completed\n");
        }
        
        if (classes) free(classes);
        if (methods) free(methods);
        if (fields) free(fields);
    }
    
    // Add code section
    if (!arxmod_writer_add_code_section(&writer, instructions, instruction_count)) {
        printf("Error: Failed to add code section\n");
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    // Add strings section with collected string literals
    // Use the parser's collected string literals if available, otherwise fall back to codegen's
    // Merge and add strings section
    size_t total_string_count = 0;
    char **all_strings = NULL;
    
    // Use only centralized string literals (no more dual string stores)
    if (context->string_literals && context->string_literals_count > 0) {
        total_string_count = context->string_literals_count;
        all_strings = malloc(sizeof(char*) * total_string_count);
        if (!all_strings) {
            printf("Error: Failed to allocate memory for strings\n");
            arxmod_writer_cleanup(&writer);
            return false;
        }
        
        // Copy centralized strings
        for (size_t i = 0; i < context->string_literals_count; i++) {
            all_strings[i] = context->string_literals[i];
        }
        
        // Method signature information is now stored directly in method entries
        // No need to add signature strings to the string table
        
        // Write merged strings section
        if (!arxmod_writer_add_strings_section(&writer, all_strings, total_string_count)) {
            printf("Error: Failed to add strings section\n");
            free(all_strings);
            arxmod_writer_cleanup(&writer);
            return false;
        }
        
        free(all_strings);
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
    
    // Add classes section
    class_entry_t *classes = NULL;
    size_t class_count = 0;
    method_entry_t *methods = NULL;
    size_t method_count = 0;
    field_entry_t *fields = NULL;
    size_t field_count = 0;
    if (context->parser_context && context->parser_context->root) {
        if (!collect_classes_from_ast(context, context->parser_context->root, &classes, &class_count, &methods, &method_count, &fields, &field_count)) {
            printf("Error: Failed to collect classes from AST\n");
            arxmod_writer_cleanup(&writer);
            return false;
        }
    }
    
    if (!arxmod_writer_add_classes_section(&writer, classes, class_count, methods, method_count, fields, field_count)) {
        printf("Error: Failed to add classes section\n");
        if (classes) free(classes);
        arxmod_writer_cleanup(&writer);
        return false;
    }
    
    if (classes) {
        free(classes);
    }
    if (methods) {
        free(methods);
    }
    if (fields) {
        free(fields);
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
        
        // Cleanup method position tracking
        if (context->method_positions != NULL) {
            for (size_t i = 0; i < context->method_position_count; i++) {
                if (context->method_positions[i].method_name != NULL) {
                    free(context->method_positions[i].method_name);
                }
            }
            free(context->method_positions);
            context->method_positions = NULL;
        }
        
        memset(context, 0, sizeof(codegen_context_t));
    }
}

bool detect_entry_point(ast_node_t *root)
{
    if (!root || root->type != AST_MODULE) {
        return false;
    }
    
    // Look for an App class with a Main method
    for (size_t i = 0; i < root->child_count; i++) {
        ast_node_t *child = root->children[i];
        if (child->type == AST_CLASS && child->value && strcmp(child->value, "App") == 0) {
            // Found App class, check if it has a Main method
            for (size_t j = 0; j < child->child_count; j++) {
                ast_node_t *method = child->children[j];
                if ((method->type == AST_PROCEDURE || method->type == AST_FUNCTION) && 
                    method->value && strcmp(method->value, "Main") == 0) {
                    return true; // Found App.Main
                }
            }
        }
    }
    
    return false; // No App.Main found
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
    
    // Build each class separately with its own context
    for (size_t i = 0; i < node->child_count; i++) {
        if (node->children[i]->type == AST_CLASS) {
            if (!build_class_separately(context, node->children[i])) {
                return false;
            }
        }
    }
    
    // Don't add HALT instruction - let Main method return properly
    
    return true;
}

bool build_class_separately(codegen_context_t *context, ast_node_t *class_node)
{
    if (context == NULL || class_node == NULL || class_node->type != AST_CLASS) {
        return false;
    }
    
    if (debug_mode) {
        printf("Building class separately: %s\n", class_node->value ? class_node->value : "unknown");
    }
    
    // Create a separate codegen context for this class
    codegen_context_t class_context;
    codegen_init(&class_context, context->parser_context);
    
    // Set the current class context
    class_context.current_class = class_node;
    class_context.current_class_name = class_node->value;
    
    if (debug_mode) {
        printf("Created separate context for class: %s\n", class_context.current_class_name ? class_context.current_class_name : "unknown");
    }
    
    // Generate code for this class using its own context
    if (!generate_class(&class_context, class_node)) {
        codegen_cleanup(&class_context);
        return false;
    }
    
    // Merge the class bytecode into the main context
    if (debug_mode) {
        printf("Merging %zu instructions from class %s into main context\n", 
               class_context.instruction_count, class_context.current_class_name ? class_context.current_class_name : "unknown");
    }
    
    // Store the current instruction count as the base offset for this class
    size_t class_base_offset = context->instruction_count;
    
    // Append class instructions to main context
    for (size_t i = 0; i < class_context.instruction_count; i++) {
        emit_instruction(context, class_context.instructions[i].opcode, 
                        0, // level is encoded in opcode upper nibble
                        class_context.instructions[i].opt64);
    }
    
    // Merge method positions from class context to main context
    if (debug_mode) {
        printf("Merging %zu method positions from class %s into main context\n", 
               class_context.method_position_count, class_context.current_class_name ? class_context.current_class_name : "unknown");
    }
    
    for (size_t i = 0; i < class_context.method_position_count; i++) {
        // Expand method positions array if needed
        if (context->method_position_count >= context->method_position_capacity) {
            size_t new_capacity = context->method_position_capacity == 0 ? 8 : context->method_position_capacity * 2;
            void *new_positions = realloc(context->method_positions, new_capacity * sizeof(*context->method_positions));
            if (!new_positions) {
                codegen_cleanup(&class_context);
                return false;
            }
            context->method_positions = new_positions;
            context->method_position_capacity = new_capacity;
        }
        
        // Add method position with adjusted offset
        size_t index = context->method_position_count;
        context->method_positions[index].method_name = strdup(class_context.method_positions[i].method_name);
        context->method_positions[index].start_instruction = class_base_offset + class_context.method_positions[i].start_instruction;
        context->method_positions[index].end_instruction = class_base_offset + class_context.method_positions[i].end_instruction;
        context->method_position_count++;
        
        if (debug_mode) {
            printf("Merged method '%s' at offset %zu (adjusted from %zu)\n", 
                   class_context.method_positions[i].method_name, 
                   context->method_positions[index].start_instruction,
                   class_context.method_positions[i].start_instruction);
        }
    }
    
    // Merge labels from class context to main context
    if (debug_mode) {
        printf("Merging %zu labels from class %s into main context\n", 
               class_context.label_table_size, class_context.current_class_name ? class_context.current_class_name : "unknown");
    }
    
    for (size_t i = 0; i < class_context.label_table_size; i++) {
        // Add label to main context
        if (context->label_table_size >= context->label_table_capacity) {
            size_t new_capacity = context->label_table_capacity == 0 ? 16 : context->label_table_capacity * 2;
            label_entry_t *new_table = realloc(context->label_table, new_capacity * sizeof(label_entry_t));
            if (!new_table) {
                codegen_cleanup(&class_context);
                return false;
            }
            context->label_table = new_table;
            context->label_table_capacity = new_capacity;
        }
        
        context->label_table[context->label_table_size] = class_context.label_table[i];
        context->label_table[context->label_table_size].instruction_index += class_base_offset;
        context->label_table_size++;
    }
    
    // Clean up the class context
    codegen_cleanup(&class_context);
    
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
        
        if (child->type == AST_OBJECT_VAR) {
            // Object variable handling - variables are accessible only within class procedures/functions
            if (debug_mode) {
                printf("Found object variable: %s\n", child->value ? child->value : "unknown");
            }
        } else if (child->type == AST_PROCEDURE || child->type == AST_FUNCTION) {
            if (!generate_method(context, child)) {
                return false;
            }
        }
    }
    
    return true;
}

bool generate_object_variable(codegen_context_t *context, ast_node_t *node)
{
    // Object variable handling - variables are accessed directly by name within class procedures/functions
    if (context == NULL || node == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("Generating code for object variable: %s\n", node->value ? node->value : "unknown");
    }
    
    // For now, object variables don't generate code - they're just metadata
    // In a full implementation, this would generate variable access code
    
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
    
    // Method position tracking is now handled in generate_ast_code
    // when the actual method bytecode is generated
    
    // Check if this is the Main method
    if (debug_mode) {
        printf("Method name: '%s'\n", node->value ? node->value : "NULL");
    }
    
    // Start tracking method position right before generating method bytecode
    if (node->value) {
        codegen_start_method_tracking(context, node->value);
        if (debug_mode) {
            printf("Started tracking method '%s' at instruction %zu (actual bytecode position)\n", 
                   node->value, context->instruction_count);
        }
    }
    
    // Generate code for the method's body using AST-based approach
    if (debug_mode) {
        printf("Generating code for method: %s\n", node->value ? node->value : "unknown");
    }
    
    // Generate code for the method's body
    for (size_t i = 0; i < node->child_count; i++) {
        if (debug_mode) {
            printf("Processing method child %zu, type: %d\n", i, node->children[i]->type);
        }
        generate_ast_code(context, node->children[i]);
    }
    
    // End tracking method position after generating method bytecode
    if (node->value) {
        codegen_end_method_tracking(context, node->value);
        if (debug_mode) {
            printf("Ended tracking method '%s' at instruction %zu\n", 
                   node->value, context->instruction_count);
        }
    }
    
    return true;
}

// Legacy method generation - keeping for reference but not used
bool generate_method_legacy(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL) {
        return false;
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
        
        // Generate code for the Main method's body only
        if (context->parser_context && context->parser_context->root) {
            if (debug_mode) {
                printf("Generating code for Main method body only\n");
            }
            
            // Find the Main method in the AST and generate code for its body
            if (context->parser_context->root) {
                // Look for the App class and its Main method
                for (size_t i = 0; i < context->parser_context->root->child_count; i++) {
                    ast_node_t *child = context->parser_context->root->children[i];
                    if (child->type == AST_CLASS && child->value && strcmp(child->value, "App") == 0) {
                        // Found App class, look for Main method
                        for (size_t j = 0; j < child->child_count; j++) {
                            ast_node_t *method = child->children[j];
                            printf("CODEGEN: Checking method %zu, type: %d, value: '%s'\n", 
                                   j, method->type, method->value ? method->value : "NULL");
                            if ((method->type == AST_PROCEDURE || method->type == AST_FUNCTION) && 
                                method->value && strcmp(method->value, "Main") == 0) {
                                // Found Main method, generate code for its body
                                printf("CODEGEN: Found Main method, generating code for its %zu children\n", method->child_count);
                                for (size_t k = 0; k < method->child_count; k++) {
                                    printf("CODEGEN: Processing Main method child %zu, type: %d\n", k, method->children[k]->type);
                                    generate_ast_code(context, method->children[k]);
                                }
                                printf("CODEGEN: Finished processing Main method\n");
                                break;
                            }
                        }
                        break;
                    }
                }
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
            // OPR_WRITELN removed - writeln is now accessed via system.writeln()
        }
        
    if (debug_mode) {
        printf("Generated DYNAMIC code for all statements - no hardcoded values!\n");
    }
    
    // Report final string count after all processing
    printf("FINAL: Codegen completed with %zu string literals\n", context->string_literals_count);
        
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
    } else {
        // Generate bytecode for class methods (non-Main methods)
        if (debug_mode) {
            printf("Generating bytecode for class method: %s\n", node->value ? node->value : "unknown");
        }
        
        // Generate bytecode for the method body
        if (node->child_count > 0) {
            // The method body is in the first child
            ast_node_t *method_body = node->children[0];
            if (method_body) {
                if (debug_mode) {
                    printf("Generating bytecode for method body with %zu children\n", method_body->child_count);
                }
                
                // Generate bytecode for each statement in the method body
                for (size_t i = 0; i < method_body->child_count; i++) {
                    ast_node_t *statement = method_body->children[i];
                    if (statement) {
                        if (debug_mode) {
                            printf("Generating bytecode for statement %zu in method %s\n", i, node->value ? node->value : "unknown");
                        }
                        
                        // Generate bytecode for the statement
                        if (!generate_statement(context, statement)) {
                            printf("Error: Failed to generate bytecode for statement %zu in method %s\n", i, node->value ? node->value : "unknown");
                            return false;
                        }
                    }
                }
            }
        }
        
        // Add return instruction for all methods
        // The method's return type determines what to return
        if (debug_mode) {
            printf("Adding return instruction for method: %s\n", node->value ? node->value : "unknown");
        }
        
        // Generate proper method body bytecode
        // The method body statements should already be generated above
        // If the method has a return statement, it should have generated the return value
        // If not, we need to generate a default return value
        
        // Check if the method body already generated a return value
        // If not, generate a placeholder return value
        if (debug_mode) {
            printf("Method body bytecode generation completed for: %s\n", node->value ? node->value : "unknown");
        }
        
        // The method body should have already generated the return value
        // If it didn't, we'll generate a placeholder
        // This is a fallback for methods that don't have explicit return statements
        emit_instruction(context, VM_LIT, 0, 0xFFFF); // Placeholder return value
        
        emit_instruction(context, VM_OPR, 0, OPR_RET);
    }
    
    // End tracking method position
    if (node->value) {
        codegen_end_method_tracking(context, node->value);
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
            
        case AST_FIELD_ACCESS:
            return generate_field_access(context, node);
            
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
    if (node->child_count > 0 && node->children[0]->value) {
        const char *var_name = node->children[0]->value;
        
        // Check if we're in a class context (method body)
        if (context->current_class && context->current_class_name) {
            // This is a field assignment within a class method
            if (debug_mode) {
                printf("Field assignment in class '%s': %s\n", context->current_class_name, var_name);
            }
            
            // For now, use field offset 0 for all fields
            // TODO: Implement proper field offset calculation based on field name
            uint64_t field_offset = 0;
            
            // Generate field assignment: push field offset, set field
            // The VM will use the stored object address from the method call context
            emit_instruction(context, VM_OPR, 0, OPR_OBJ_SET_FIELD);
        } else {
            // This is a local variable assignment
            if (debug_mode) {
                printf("Local variable assignment: %s\n", var_name);
            }
            
            // Check if variable already exists
            bool found = false;
            size_t var_address = 0;
            for (size_t i = 0; i < context->variable_count; i++) {
                if (context->variable_names[i] && strcmp(context->variable_names[i], var_name) == 0) {
                    var_address = context->variable_addresses[i];
                    found = true;
                    break;
                }
            }
            
            // If not found, add new variable
            if (!found) {
                if (context->variable_count >= context->variable_capacity) {
                    size_t new_capacity = context->variable_capacity == 0 ? 8 : context->variable_capacity * 2;
                    char **new_names = realloc(context->variable_names, new_capacity * sizeof(char*));
                    size_t *new_addresses = realloc(context->variable_addresses, new_capacity * sizeof(size_t));
                    if (!new_names || !new_addresses) {
                        return false;
                    }
                    context->variable_names = new_names;
                    context->variable_addresses = new_addresses;
                    context->variable_capacity = new_capacity;
                }
                
                var_address = context->next_variable_address++;
                context->variable_names[context->variable_count] = strdup(var_name);
                context->variable_addresses[context->variable_count] = var_address;
                context->variable_count++;
                
                if (debug_mode) {
                    printf("Added variable '%s' at address %zu (total variables: %zu)\n", 
                           var_name, var_address, context->variable_count);
                }
            }
            
            emit_instruction(context, VM_STO, 0, var_address);
        }
    } else {
        // Fallback to address 0
        emit_instruction(context, VM_STO, 0, 0);
    }
    
    return true;
}

// Centralized string literal management with deduplication
size_t get_or_add_string_literal(codegen_context_t *context, const char *string_literal)
{
    printf("GET_OR_ADD_STRING: Called with string='%s'\n", string_literal);
    if (context == NULL || string_literal == NULL) {
        printf("GET_OR_ADD_STRING: NULL context or string, returning 0\n");
        return 0;
    }
    
    // Initialize string store if needed
    if (context->string_literals == NULL) {
        context->string_literals = malloc(sizeof(char*) * 16);
        context->string_literals_capacity = 16;
        context->string_literals_count = 0;
    }
    
    // Check if string already exists (deduplication)
    for (size_t i = 0; i < context->string_literals_count; i++) {
        if (context->string_literals[i] && strcmp(context->string_literals[i], string_literal) == 0) {
            if (debug_mode) {
                printf("String literal '%s' already exists at ID %zu (reusing)\n", string_literal, i);
            }
            return i; // Return existing ID
        }
    }
    
    // Expand if needed
    if (context->string_literals_count >= context->string_literals_capacity) {
        size_t new_capacity = context->string_literals_capacity * 2;
        char **new_strings = realloc(context->string_literals, sizeof(char*) * new_capacity);
        if (new_strings == NULL) {
            return 0;
        }
        context->string_literals = new_strings;
        context->string_literals_capacity = new_capacity;
    }
    
    // Add new string literal
    size_t string_id = context->string_literals_count;
    printf("GET_OR_ADD_STRING: Adding new string at ID %zu\n", string_id);
    context->string_literals[string_id] = malloc(strlen(string_literal) + 1);
    if (context->string_literals[string_id] == NULL) {
        printf("GET_OR_ADD_STRING: Failed to allocate memory for string, returning 0\n");
        return 0;
    }
    strcpy(context->string_literals[string_id], string_literal);
    context->string_literals_count++;
    
    printf("GET_OR_ADD_STRING: Successfully added string '%s' with ID %zu (total: %zu)\n", 
           string_literal, string_id, context->string_literals_count);
    
    return string_id;
}

// generate_writeln_statement is no longer needed - writeln is now a global procedure call

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
            if (debug_mode) {
                printf("generate_expression: Calling generate_literal for AST_LITERAL\n");
            }
            return generate_literal(context, node);
            
        case AST_IDENTIFIER:
            return generate_identifier(context, node);
            
        case AST_NEW_EXPR:
            return generate_new_expression(context, node);
            
        case AST_BINARY_OP:
            return generate_binary_operation(context, node);
            
        case AST_UNARY_OP:
            return generate_unary_operation(context, node);
            
        case AST_METHOD_CALL:
        case AST_PROCEDURE_CALL:
        case AST_FUNCTION_CALL:
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
        printf("generate_literal: Processing literal node (value=%s, number=%llu)\n", 
               node->value ? node->value : "NULL", (unsigned long long)node->number);
    }
    
    // Use the proper literal generation function that handles both strings and numbers
    generate_literal_ast(context, node);
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
    
    // Check if we're in a class context (method body)
    if (context->current_class && context->current_class_name) {
        // First check if this is a local variable
        bool found_local = false;
        for (size_t i = 0; i < context->variable_count; i++) {
            if (context->variable_names[i] && node->value && 
                strcmp(context->variable_names[i], node->value) == 0) {
                if (debug_mode) {
                    printf("In class context '%s': found local variable '%s' at address %zu\n",
                           context->current_class_name, node->value, context->variable_addresses[i]);
                }
                // Load the local variable value from memory
                emit_instruction(context, VM_LOD, 0, context->variable_addresses[i]);
                found_local = true;
                break;
            }
        }
        
        if (!found_local) {
            if (debug_mode) {
                printf("In class context '%s': treating identifier '%s' as field access\n",
                       context->current_class_name, node->value ? node->value : "unknown");
            }

            // Field access within class methods - need to load from object's field memory
            // For now, we'll generate a placeholder that the VM can handle
            if (debug_mode) {
                printf("Field access within class method - generating field load instruction\n");
            }
            
            // Generate field access instruction - this will be handled by the VM
            // The VM will use the stored object address from the method call context
            emit_instruction(context, VM_OPR, 0, OPR_OBJ_GET_FIELD);
        }
    } else {
        if (debug_mode) {
            printf("Not in class context: treating identifier '%s' as variable access\n",
                   node->value ? node->value : "unknown");
        }

        // Look up the variable in the variable table
        if (debug_mode) {
            printf("Looking up variable '%s' in table with %zu variables\n", 
                   node->value ? node->value : "unknown", context->variable_count);
            for (size_t i = 0; i < context->variable_count; i++) {
                printf("  Variable %zu: '%s' at address %zu\n", i, 
                       context->variable_names[i] ? context->variable_names[i] : "NULL",
                       context->variable_addresses[i]);
            }
        }
        
        bool found = false;
        for (size_t i = 0; i < context->variable_count; i++) {
            if (context->variable_names[i] && node->value && 
                strcmp(context->variable_names[i], node->value) == 0) {
                if (debug_mode) {
                    printf("Found variable '%s' at address %zu\n", node->value, context->variable_addresses[i]);
                }
                // Load the variable value from memory
                emit_instruction(context, VM_LOD, 0, context->variable_addresses[i]);
                found = true;
                break;
            }
        }
        
        if (!found) {
            if (debug_mode) {
                printf("Variable '%s' not found, using placeholder string ID 0\n", node->value ? node->value : "unknown");
            }
            emit_literal(context, 0); // Load placeholder string ID 0
        }
    }

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
    if (context == NULL || node == NULL || (node->type != AST_METHOD_CALL && node->type != AST_PROCEDURE_CALL && node->type != AST_FUNCTION_CALL)) {
        return false;
    }
    
    printf("*** GENERATE_METHOD_CALL CALLED ***\n");
    printf("Generating method call: %s\n", node->value ? node->value : "unknown");
    
    // Use the same logic as generate_method_call_ast
    generate_method_call_ast(context, node);
    
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
    
    // Field access - fields are accessed directly by name within class methods
    // No special bytecode needed since fields are just local variables within the class
    if (debug_mode) {
        printf("Field access - treating as local variable access\n");
    }
    
    return true;
}

bool generate_new_expression(codegen_context_t *context, ast_node_t *node)
{
    if (context == NULL || node == NULL || node->value == NULL) {
        return false;
    }
    
    const char *class_name = node->value;
    
    if (debug_mode) {
        printf("Generating NEW expression for class: %s\n", class_name);
    }
    
    // Generate unique class ID based on module and class names
    const char *module_name = context->parser_context && context->parser_context->root ? 
                              context->parser_context->root->value : "UnknownModule";
    uint64_t class_id = codegen_generate_unique_class_id(module_name, class_name);
    
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
    
    // Generate bytecode for the return expression
    if (node->child_count > 0) {
        ast_node_t *return_expr = node->children[0];
        if (return_expr) {
            if (debug_mode) {
                printf("Generating bytecode for return expression\n");
            }
            
            // Generate bytecode for the return expression
            if (!generate_expression(context, return_expr)) {
                printf("Error: Failed to generate bytecode for return expression\n");
                return false;
            }
        }
    }
    
    // Generate return operation
    if (debug_mode) {
        printf("Emitting OPR_RET instruction\n");
    }
    emit_operation(context, OPR_RET, 0, 0);
    
    if (debug_mode) {
        printf("Return statement generation completed\n");
    }
    
    return true;
}

// generate_writeln_ast_statement is no longer needed - writeln is now a global procedure call

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
    
    printf("GENERATE_AST_CODE: Processing node type=%d, value='%s', children=%zu\n", 
           node->type, node->value ? node->value : "NULL", node->child_count);
    
    switch (node->type) {
        case AST_MODULE:
            printf("GENERATE_AST_CODE: Processing AST_MODULE\n");
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
            
        case AST_PROCEDURE:
        case AST_FUNCTION:
            // Generate code for all statements in the procedure/function
            if (debug_mode) {
                printf("AST_PROCEDURE/AST_FUNCTION node has %zu children\n", node->child_count);
            }
            for (size_t i = 0; i < node->child_count; i++) {
                if (debug_mode) {
                    printf("Processing procedure/function child %zu, type: %d\n", i, node->children[i]->type);
                }
                generate_ast_code(context, node->children[i]);
            }
            
            // Add implicit return statement for procedures/functions that don't have explicit returns
            // Check if the last statement is a return statement
            bool has_explicit_return = false;
            if (node->child_count > 0) {
                ast_node_t *last_child = node->children[node->child_count - 1];
                if (last_child && last_child->type == AST_RETURN_STMT) {
                    has_explicit_return = true;
                }
            }
            
            // If no explicit return, add implicit return
            if (!has_explicit_return) {
                if (debug_mode) {
                    printf("Adding implicit return statement for %s\n", node->value ? node->value : "unknown");
                }
                emit_operation(context, OPR_RET, 0, 0);
            }
            
            // Method tracking is now handled in generate_method function
            break;
            
        case AST_ASSIGNMENT:
            generate_assignment(context, node);
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
                generate_expression(context, node->children[0]);
                
                // All expression types push their result onto the stack
                // For output statements, we need to output whatever is on the stack
                    if (debug_mode) {
                        const char *type_name = "unknown";
                        if (node->children[0]->type == AST_LITERAL) type_name = "literal";
                        else if (node->children[0]->type == AST_IDENTIFIER) type_name = "identifier";
                        else if (node->children[0]->type == AST_BINARY_OP) type_name = "binary operation";
                        else if (node->children[0]->type == AST_PROCEDURE_CALL) type_name = "procedure call";
                        else if (node->children[0]->type == AST_FUNCTION_CALL) type_name = "function call";
                        else if (node->children[0]->type == AST_FIELD_ACCESS) type_name = "field access";
                    printf("Outputting result of %s expression\n", type_name);
                    }
                
                // Output whatever the expression pushed onto the stack
                    emit_instruction(context, VM_OPR, 0, OPR_OUTSTRING);
                    // OPR_WRITELN removed - writeln is now accessed via system.writeln()
            }
            break;
            
        case AST_FIELD_ACCESS:
            // Field access within class methods - treat as identifier access
            if (debug_mode) {
                printf("AST_FIELD_ACCESS: treating as identifier access\n");
            }
            generate_identifier_ast(context, node);
            break;
            
        case AST_RETURN_STMT:
            // Return statement - generate code for return expression and return instruction
            if (debug_mode) {
                printf("AST_RETURN_STMT: processing return statement\n");
            }
            generate_return_statement(context, node);
            break;
            
        // AST_WRITELN_STMT removed - writeln is now accessed via system.writeln()
            
        default:
            printf("GENERATE_AST_CODE: Unhandled AST node type: %d\n", node->type);
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
            printf("*** AST_METHOD_CALL CASE REACHED ***\n");
            printf("GENERATE_AST_CODE: Processing AST_METHOD_CALL '%s' with %zu children\n", 
                   node->value ? node->value : "NULL", node->child_count);
            for (size_t i = 0; i < node->child_count; i++) {
                printf("GENERATE_AST_CODE: Method call child %zu: type=%d, value='%s'\n", 
                       i, node->children[i]->type, node->children[i]->value ? node->children[i]->value : "NULL");
            }
            generate_method_call_ast(context, node);
            break;
            
        case AST_PROCEDURE_CALL:
            printf("*** AST_PROCEDURE_CALL CASE REACHED ***\n");
            printf("GENERATE_AST_CODE: Processing AST_PROCEDURE_CALL '%s' with %zu children\n", 
                   node->value ? node->value : "NULL", node->child_count);
            for (size_t i = 0; i < node->child_count; i++) {
                printf("GENERATE_AST_CODE: Procedure call child %zu: type=%d, value='%s'\n", 
                       i, node->children[i]->type, node->children[i]->value ? node->children[i]->value : "NULL");
            }
            generate_method_call_ast(context, node);
            break;
            
        case AST_FUNCTION_CALL:
            printf("GENERATE_AST_CODE: Processing AST_FUNCTION_CALL '%s' with %zu children\n", 
                   node->value ? node->value : "NULL", node->child_count);
            for (size_t i = 0; i < node->child_count; i++) {
                printf("GENERATE_AST_CODE: Function call child %zu: type=%d, value='%s'\n", 
                       i, node->children[i]->type, node->children[i]->value ? node->children[i]->value : "NULL");
            }
            generate_method_call_ast(context, node);
            break;
            
        case AST_FIELD_ACCESS:
            generate_field_access_ast(context, node);
            break;
            
        case AST_NEW_EXPR:
            generate_new_expression_ast(context, node);
            break;
            
        default:
            if (debug_mode) {
                printf("Unhandled expression AST node type: %d\n", node->type);
            }
            break;
    }
}

void generate_new_expression_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node || !node->value) return;
    
    const char *class_name = node->value;
    
    if (debug_mode) {
        printf("Generating NEW expression AST for class: %s\n", class_name);
    }
    
    // Generate unique class ID based on module and class names
    const char *module_name = context->parser_context && context->parser_context->root ? 
                              context->parser_context->root->value : "UnknownModule";
    uint64_t class_id = codegen_generate_unique_class_id(module_name, class_name);
    
    // Emit NEW instruction
    emit_instruction(context, VM_LIT, 0, class_id);
    emit_instruction(context, VM_OPR, 0, OPR_OBJ_NEW);
    
    if (debug_mode) {
        printf("  Generated NEW instruction for class '%s' (ID: %llu)\n", 
               class_name, (unsigned long long)class_id);
    }
}

void generate_literal_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!node) return;
    
    printf("GENERATE_LITERAL: Processing node with value='%s', number=%llu\n", 
           node->value ? node->value : "NULL", (unsigned long long)node->number);
    
    if (node->value) {
        // String literal - use centralized string literal management
        printf("GENERATE_LITERAL: About to call get_or_add_string_literal for '%s'\n", node->value);
        size_t string_id = get_or_add_string_literal(context, node->value);
        printf("GENERATE_LITERAL: get_or_add_string_literal returned ID %zu\n", string_id);
        
        // Emit LIT instruction with the string ID
        emit_instruction(context, VM_LIT, 0, string_id);
    } else {
        // Number literal - load the actual value from the AST
        if (debug_mode) {
            printf("Loading number literal %llu\n", (unsigned long long)node->number);
        }
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
    } else if (strcmp(node->value, "&&") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_AND);
    } else if (strcmp(node->value, "||") == 0) {
        emit_instruction(context, VM_OPR, 0, OPR_OR);
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
        emit_instruction(context, VM_OPR, 0, OPR_NOT);
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
        if (debug_mode) {
            printf("set_label: Invalid parameters - context=%p, instruction_index=%zu, instruction_count=%zu\n", 
                   (void*)context, instruction_index, context ? context->instruction_count : 0);
        }
        return;
    }
    
    if (debug_mode) {
        printf("Setting label %zu to instruction %zu (table_size=%zu, capacity=%zu)\n", 
               label_id, instruction_index, context->label_table_size, context->label_table_capacity);
    }
    
    // Add or update label in the label table
    bool found = false;
    for (size_t i = 0; i < context->label_table_size; i++) {
        if (context->label_table[i].label_id == label_id) {
            context->label_table[i].instruction_index = instruction_index;
            context->label_table[i].defined = true;
            found = true;
            if (debug_mode) {
                printf("Updated existing label %zu to instruction %zu\n", label_id, instruction_index);
            }
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
                if (debug_mode) {
                    printf("set_label: Failed to allocate memory for label table\n");
                }
                return; // Out of memory
            }
            context->label_table = new_table;
            context->label_table_capacity = new_capacity;
            if (debug_mode) {
                printf("Expanded label table to capacity %zu\n", new_capacity);
            }
        }
        
        context->label_table[context->label_table_size].label_id = label_id;
        context->label_table[context->label_table_size].instruction_index = instruction_index;
        context->label_table[context->label_table_size].defined = true;
        context->label_table_size++;
        
        if (debug_mode) {
            printf("Added new label %zu at instruction %zu (new table_size=%zu)\n", 
                   label_id, instruction_index, context->label_table_size);
        }
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
            
            if (debug_mode) {
                printf("Found jump instruction %zu: opcode=%d, label_id=%zu\n", 
                       i, context->instructions[i].opcode, label_id);
            }
            
            // Find the label in the label table
            bool found = false;
            for (size_t j = 0; j < context->label_table_size; j++) {
                if (context->label_table[j].label_id == label_id && context->label_table[j].defined) {
                    context->instructions[i].opt64 = context->label_table[j].instruction_index;
                    if (debug_mode) {
                        printf("Resolved jump at instruction %zu: label %zu -> instruction %zu\n", 
                               i, label_id, context->label_table[j].instruction_index);
                    }
                    found = true;
                    break;
                }
            }
            
            if (!found && debug_mode) {
                printf("Warning: Could not resolve label %zu for jump at instruction %zu\n", label_id, i);
            }
        }
    }
}

void generate_method_call_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!context || !node) return;
    
    printf("GENERATE_METHOD_CALL_AST: Processing method call '%s' with %zu children\n", 
           node->value ? node->value : "unknown", node->child_count);
    
    // **CRITICAL FIX**: Process method call parameters (children) to collect string literals
    for (size_t i = 0; i < node->child_count; i++) {
        printf("GENERATE_METHOD_CALL_AST: Processing parameter %zu (type=%d, value='%s')\n", 
               i, node->children[i]->type, node->children[i]->value ? node->children[i]->value : "NULL");
        
        // Generate code for each parameter (this will collect string literals)
        generate_ast_code(context, node->children[i]);
    }
    
    // Check if this is a global procedure call (no dot) or object method call (with dot)
    if (node->value) {
        char *dot_pos = strrchr(node->value, '.');
        if (dot_pos) {
            // Object method call: object.method()
            // Split into object name and method name
            size_t object_name_len = dot_pos - node->value;
            char *object_name = malloc(object_name_len + 1);
            if (object_name == NULL) {
                printf("Error: Failed to allocate memory for object name\n");
                return;
            }
            strncpy(object_name, node->value, object_name_len);
            object_name[object_name_len] = '\0';
            
            char *method_name = dot_pos + 1; // Skip the '.'
            
            if (debug_mode) {
                printf("  Object name: %s, Method name: %s\n", object_name, method_name);
            }
            
            // Step 1: Push object address onto stack
            // Special handling for virtual 'system' object
            if (strcmp(object_name, "system") == 0) {
                // Virtual system object - use special address 0xFFFFFFFF
                emit_instruction(context, VM_LIT, 0, 0xFFFFFFFF);
                if (debug_mode) {
                    printf("  Loading virtual system object with special address 0xFFFFFFFF\n");
                }
            } else {
                // Look up the object variable in the symbol table
                size_t object_address;
                if (codegen_find_variable(context, object_name, &object_address)) {
                    emit_instruction(context, VM_LOD, 0, object_address);
                    if (debug_mode) {
                        printf("  Loading object '%s' from address %zu\n", object_name, object_address);
                    }
                } else {
                    if (debug_mode) {
                        printf("  Warning: Object '%s' not found in symbol table, using placeholder address 0\n", object_name);
                    }
                    // Use placeholder address if object not found
                    emit_instruction(context, VM_LIT, 0, 0);
                }
            }
            
            // Step 2: Generate method ID using the same hash function as method definition
            // We need to determine the class name and generate the proper method ID
            const char *class_name = "UnknownClass"; // TODO: Get actual class name from object type
            
            // For now, use a simple heuristic: if object name starts with "person", it's Person class
            // This is a temporary solution until we implement proper type tracking
            if (strncmp(object_name, "person", 6) == 0) {
                class_name = "Person";
            } else if (strncmp(object_name, "student", 7) == 0) {
                class_name = "Student";
            } else if (strcmp(object_name, "system") == 0) {
                class_name = "System";
            }
            
            const char *module_name = context->parser_context && context->parser_context->root ? 
                                     context->parser_context->root->value : "UnknownModule";
            
            // Use a simpler approach: store method name in the parser context's string table
            // The linker will resolve this by name instead of by hash ID
            uint64_t method_name_id = 0;
            
            // Add method name to parser context's string table if not already present
            bool found = false;
            if (context->parser_context && context->parser_context->method_string_literals) {
                for (size_t k = 0; k < context->parser_context->method_string_count; k++) {
                    if (context->parser_context->method_string_literals[k] && 
                        strcmp(context->parser_context->method_string_literals[k], method_name) == 0) {
                        method_name_id = k;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    // Add new string to parser context's string table
                    if (context->parser_context->method_string_count >= context->parser_context->method_string_capacity) {
                        size_t new_capacity = context->parser_context->method_string_capacity == 0 ? 8 : context->parser_context->method_string_capacity * 2;
                        char **new_strings = realloc(context->parser_context->method_string_literals, new_capacity * sizeof(char*));
                        if (!new_strings) {
                            method_name_id = 0; // Fallback
                        } else {
                            context->parser_context->method_string_literals = new_strings;
                            context->parser_context->method_string_capacity = new_capacity;
                        }
                    }
                    
                    if (context->parser_context->method_string_count < context->parser_context->method_string_capacity) {
                        context->parser_context->method_string_literals[context->parser_context->method_string_count] = strdup(method_name);
                        method_name_id = context->parser_context->method_string_count;
                        context->parser_context->method_string_count++;
                    }
                }
            }
            
            emit_instruction(context, VM_LIT, 0, method_name_id);
            
            if (debug_mode) {
                printf("  Pushed method name ID %llu for method '%s' (class: %s, module: %s)\n", 
                       (unsigned long long)method_name_id, method_name, class_name, module_name);
            }
            
            free(object_name);
        } else {
            // No dot found, treat whole string as method name with no object
            if (debug_mode) {
                printf("  Warning: No object found in method call '%s', using placeholder\n", node->value);
            }
            
            // Push placeholder object address
            emit_instruction(context, VM_LIT, 0, 0);
            
            // Push method name
            uint64_t method_name_id = context->string_literals_count;
            // (string literal storage logic same as above)
            emit_instruction(context, VM_LIT, 0, method_name_id);
        }
    } else {
        // No method call value, use placeholders
        emit_instruction(context, VM_LIT, 0, 0); // Object address
        emit_instruction(context, VM_LIT, 0, 0); // Method name
    }
    
    // Step 3: Generate method call instruction
    // Check if this is a global procedure call (no dot) or object method call (with dot)
    if (node->value && strchr(node->value, '.') == NULL) {
        // Global procedure call: procedure()
        const char *procedure_name = node->value;
        
        if (debug_mode) {
            printf("  Global procedure call: %s\n", procedure_name);
        }
        
        // For global procedures like writeln, we need to handle them specially
        if (strcmp(procedure_name, "writeln") == 0) {
            // writeln is a procedure of the virtual 'system' object
            // The argument should already be on the stack from the expression parsing
            // We just need to emit the output instruction
            emit_instruction(context, VM_OPR, 0, OPR_OUTSTRING);
            
            if (debug_mode) {
                printf("Generated system.writeln procedure call instruction\n");
            }
        } else {
            // Other system procedures - for now, just emit a placeholder
            if (debug_mode) {
                printf("  Warning: System procedure '%s' not implemented, using placeholder\n", procedure_name);
            }
            emit_instruction(context, VM_OPR, 0, OPR_OUTSTRING); // Placeholder
        }
    } else {
        // Object method call
        emit_instruction(context, VM_OPR, 0, OPR_OBJ_CALL_METHOD);
        
        if (debug_mode) {
            printf("Generated object method call instruction for method: %s\n", node->value ? node->value : "unknown");
        }
    }
}

void generate_field_access_ast(codegen_context_t *context, ast_node_t *node)
{
    if (!context || !node) return;
    
    if (debug_mode) {
        printf("Generating field access: %s\n", node->value ? node->value : "unknown");
    }
    
    // Field access - fields are accessed directly by name within class methods
    // No special bytecode needed since fields are just local variables within the class
    if (debug_mode) {
        printf("Field access - treating as local variable access\n");
    }
    
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
    size_t loop_end_label = create_label(context);
    size_t loop_condition_label = create_label(context);
    
    if (debug_mode) {
        printf("Generating FOR loop: %s = %s to %s\n", 
               var_node->value ? var_node->value : "unknown",
               start_expr->value ? start_expr->value : "unknown",
               end_expr->value ? end_expr->value : "unknown");
        printf("FOR loop labels: condition=%zu, end=%zu\n", 
               loop_condition_label, loop_end_label);
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
    
    // Generate start expression and store in loop variable (INITIALIZATION - outside loop)
    generate_expression_ast(context, start_expr);
    emit_store(context, 0, var_address);
    if (debug_mode) {
        printf("Emitted initialization code at instruction %zu\n", context->instruction_count);
    }
    
    // Set condition check label (start of loop)
    set_label(context, loop_condition_label, context->instruction_count);
    if (debug_mode) {
        printf("Set condition check label %zu at instruction %zu\n", loop_condition_label, context->instruction_count);
    }
    
    // Load loop variable and end expression for comparison
    emit_load(context, 0, var_address);
    generate_expression_ast(context, end_expr);
    
    // Compare loop variable with end value (less than or equal)
    emit_operation(context, OPR_LEQ, 0, 0);
    
    // Jump to end if condition is false (i.e., if loop_var > end_value)
    emit_jump_if_false(context, loop_end_label);
    if (debug_mode) {
        printf("Emitted condition check and jump to end if false at label %zu\n", loop_end_label);
    }
    
    // Body execution starts here (no label needed)
    
    // Generate loop body
    if (body_node && body_node->type == AST_BLOCK) {
        for (size_t i = 0; i < body_node->child_count; i++) {
            generate_ast_code(context, body_node->children[i]);
        }
    }
    
    // Increment loop variable (load, add 1, store)
    emit_load(context, 0, var_address);
    emit_literal(context, 1);
    emit_operation(context, OPR_ADD, 0, 0);
    emit_store(context, 0, var_address);
    if (debug_mode) {
        printf("Emitted increment code at instruction %zu\n", context->instruction_count);
    }
    
    // Jump back to condition check
    emit_jump(context, loop_condition_label);
    if (debug_mode) {
        printf("Emitted jump back to condition check at label %zu\n", loop_condition_label);
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

// Class collection functions
bool collect_classes_from_ast(codegen_context_t *context, ast_node_t *ast, class_entry_t **classes, size_t *class_count, method_entry_t **methods, size_t *method_count, field_entry_t **fields, size_t *field_count)
{
    if (context == NULL || ast == NULL || classes == NULL || class_count == NULL || 
        methods == NULL || method_count == NULL || fields == NULL || field_count == NULL) {
        return false;
    }
    
    // Count classes, methods, and fields in the AST
    size_t class_count_total = 0;
    size_t method_count_total = 0;
    size_t field_count_total = 0;
    
    if (ast->type == AST_MODULE) {
        for (size_t i = 0; i < ast->child_count; i++) {
            if (ast->children[i]->type == AST_CLASS) {
                ast_node_t *class_node = ast->children[i];
                class_count_total++;
                
                // Count methods and fields in this class
                printf("DEBUG: Class %s has %zu children\n", class_node->value ? class_node->value : "unknown", class_node->child_count);
                for (size_t j = 0; j < class_node->child_count; j++) {
                    ast_node_t *child = class_node->children[j];
                    printf("DEBUG: Class child %zu: type=%d, value='%s'\n", j, child->type, child->value ? child->value : "NULL");
                    
                    if (child->type == AST_OBJECT_VAR) {
                        field_count_total++;
                        printf("DEBUG: Found object variable: %s (type=%d)\n", child->value ? child->value : "unknown", child->type);
                    } else if (child->type == AST_PROCEDURE || 
                        child->type == AST_FUNCTION) {
                        method_count_total++;
                        printf("DEBUG: Found method: %s (type=%d)\n", child->value ? child->value : "unknown", child->type);
                    } else {
                        printf("DEBUG: Unknown child type: %d\n", child->type);
                    }
                }
            }
        }
    }
    
    if (class_count_total == 0) {
        *classes = NULL;
        *class_count = 0;
        *methods = NULL;
        *method_count = 0;
        *fields = NULL;
        *field_count = 0;
        return true;
    }
    
    // Allocate arrays
    *classes = malloc(class_count_total * sizeof(class_entry_t));
    if (*classes == NULL) return false;
    
    *methods = malloc(method_count_total * sizeof(method_entry_t));
    if (*methods == NULL) {
        free(*classes);
        return false;
    }
    
    *fields = malloc(field_count_total * sizeof(field_entry_t));
    if (*fields == NULL) {
        free(*classes);
        free(*methods);
        return false;
    }
    
    // Fill class entries and collect methods/fields
    size_t class_index = 0;
    size_t method_index = 0;
    size_t field_index = 0;
    
    if (ast->type == AST_MODULE) {
        for (size_t i = 0; i < ast->child_count; i++) {
            if (ast->children[i]->type == AST_CLASS) {
                ast_node_t *class_node = ast->children[i];
                class_entry_t *class_entry = &(*classes)[class_index];
                
                // Initialize class entry
                memset(class_entry, 0, sizeof(class_entry_t));
                
                // Set class name
                if (class_node->value) {
                    strncpy(class_entry->class_name, class_node->value, 31);
                    class_entry->class_name[31] = '\0';
                }
                
                // Generate unique class ID based on module and class names
                const char *module_name = ast->value ? ast->value : "UnknownModule";
                class_entry->class_id = codegen_generate_unique_class_id(module_name, class_entry->class_name);
                
                // Process fields and methods with offset calculation
                uint64_t field_offset = 0;
                uint64_t method_offset = 0;
                
                for (size_t j = 0; j < class_node->child_count; j++) {
                    ast_node_t *child = class_node->children[j];
                    
                    if (child->type == AST_OBJECT_VAR) {
                        field_entry_t *field_entry = &(*fields)[field_index];
                        memset(field_entry, 0, sizeof(field_entry_t));
                        
                        // Set field name
                        if (child->value) {
                            strncpy(field_entry->field_name, child->value, 31);
                            field_entry->field_name[31] = '\0';
                        }
                        
                        // Set field offset (sequential for now)
                        field_entry->offset = field_offset;
                        field_offset += sizeof(uint64_t); // Each field is 8 bytes
                        
                        // Set field type (for now, assume all fields are strings)
                        field_entry->type_id = 1; // String type
                        
                        field_index++;
                        class_entry->field_count++;
                    } else if (child->type == AST_PROCEDURE || 
                        child->type == AST_FUNCTION) {
                        method_entry_t *method_entry = &(*methods)[method_index];
                        memset(method_entry, 0, sizeof(method_entry_t));
                        
                        // Set method name
                        if (child->value) {
                            strncpy(method_entry->method_name, child->value, 31);
                            method_entry->method_name[31] = '\0';
                        }
                        
                        // Extract signature information from method node
                        char *param_types = NULL;
                        char *return_type = NULL;
                        
                        // Look for parameter types and return type in child nodes
                        for (size_t k = 0; k < child->child_count; k++) {
                            ast_node_t *child_node = child->children[k];
                            if (child_node->type == AST_IDENTIFIER) {
                                // First identifier child is parameter types, second is return type
                                if (param_types == NULL) {
                                    param_types = child_node->value;
                                } else if (return_type == NULL) {
                                    return_type = child_node->value;
                                }
                            }
                        }
                        
                        // Generate signature-based method ID
                        const char *module_name = ast->value ? ast->value : "UnknownModule";
                        
                        if (debug_mode) {
                            printf("Method definition: module='%s', class='%s', method='%s', params='%s', return='%s'\n",
                                   module_name, class_entry->class_name, method_entry->method_name,
                                   param_types ? param_types : "NULL", return_type ? return_type : "NULL");
                        }
                        
                        method_entry->method_id = codegen_generate_unique_method_id(
                            module_name, 
                            class_entry->class_name, 
                            method_entry->method_name, 
                            param_types, 
                            return_type
                        );
                        
                        // Store signature information directly in method entry
                        if (param_types) {
                            strncpy(method_entry->param_types, param_types, 63);
                            method_entry->param_types[63] = '\0';
                            
                            // Count parameters
                            method_entry->parameter_count = 1; // At least one parameter
                            for (const char *p = param_types; *p; p++) {
                                if (*p == ',') {
                                    method_entry->parameter_count++;
                                }
                            }
                        } else {
                            method_entry->param_types[0] = '\0';
                            method_entry->parameter_count = 0;
                        }
                        
                        if (return_type) {
                            strncpy(method_entry->return_type, return_type, 31);
                            method_entry->return_type[31] = '\0';
                        } else {
                            method_entry->return_type[0] = '\0';
                        }
                        
                        // Set method offset using actual bytecode position
                        method_entry->offset = codegen_get_method_offset(context, child->value);
                        
                        // If method offset is 0 (not found), set it to 1 for Main method
                        if (method_entry->offset == 0 && strcmp(child->value, "Main") == 0) {
                            method_entry->offset = 1;
                        }
                        if (debug_mode) {
                            printf("Set method '%s' offset to %zu (actual bytecode position)\n", 
                                   child->value, method_entry->offset);
                            printf("  Method position tracking: found %zu methods in tracking table\n", context->method_position_count);
                            for (size_t k = 0; k < context->method_position_count; k++) {
                                printf("    Method %zu: '%s' at instruction %zu\n", k, 
                                       context->method_positions[k].method_name ? context->method_positions[k].method_name : "NULL",
                                       context->method_positions[k].start_instruction);
                            }
                        }
                        
                        method_entry->parameter_count = 0; // Count parameters from AST
                        method_entry->flags = 0;
                        
                        method_index++;
                        class_entry->method_count++;
                    }
                }
                
                class_entry->parent_class_id = 0;
                class_entry->flags = 0;
                class_entry->reserved = 0;
    
    if (debug_mode) {
                    printf("Collected class: %s (ID: %llu, fields: %u, methods: %u)\n", 
                           class_entry->class_name, (unsigned long long)class_entry->class_id, 
                           class_entry->field_count, class_entry->method_count);
                }
                
                class_index++;
            }
        }
    }
    
    *class_count = class_count_total;
    *method_count = method_count_total;
    *field_count = field_count_total;
    
    if (debug_mode) {
        printf("Collected %zu classes, %zu methods, %zu fields\n", 
               class_count_total, method_count_total, field_count_total);
    }
    
    return true;
}


