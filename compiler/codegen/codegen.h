/*
 * ARX Code Generator - Generates bytecode from AST
 * Fresh implementation with modern C practices
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../parser/parser.h"
#include "../common/opcodes.h"
#include "../arxmod/arxmod.h"

// Forward declaration
typedef struct parser_context parser_context_t;

// Label table entry
typedef struct {
    size_t label_id;               // Label identifier
    size_t instruction_index;      // Instruction index where label is defined
    bool defined;                  // Whether label has been defined
} label_entry_t;

// Code generator context
typedef struct {
    instruction_t *instructions;    // Generated instructions
    size_t instruction_count;      // Number of instructions
    size_t instruction_capacity;   // Capacity of instructions array
    size_t label_counter;          // Label counter for jumps
    bool debug_output;             // Debug output flag
    parser_context_t *parser_context; // Reference to parser context
    char **string_literals;        // String literals for ARX module
    size_t string_literals_count;  // Number of string literals
    size_t string_literals_capacity; // Capacity of string literals array
    
    // Label table for two-pass compilation
    label_entry_t *label_table;    // Label table
    size_t label_table_size;       // Number of labels in table
    size_t label_table_capacity;   // Capacity of label table
    
    // Variable tracking for Phase 2
    char **variable_names;         // Variable names
    size_t *variable_addresses;    // Variable memory addresses
    size_t variable_count;         // Number of variables
    size_t variable_capacity;      // Capacity of variables array
    size_t next_variable_address;  // Next available memory address
    
    // Class context for separate class compilation
    ast_node_t *current_class;     // Current class being compiled
    char *current_class_name;      // Name of current class
    
    // Method position tracking for accurate offset calculation
    struct {
        char *method_name;         // Method name
        size_t start_instruction;  // Instruction index where method bytecode starts
        size_t end_instruction;    // Instruction index where method bytecode ends
    } *method_positions;           // Array of method positions
    size_t method_position_count;  // Number of methods tracked
    size_t method_position_capacity; // Capacity of method positions array
} codegen_context_t;

// Function prototypes
bool codegen_init(codegen_context_t *context, parser_context_t *parser_context);
bool codegen_generate(codegen_context_t *context, ast_node_t *ast, 
                     instruction_t **instructions, size_t *instruction_count);
bool codegen_write_arxmod(codegen_context_t *context, const char *filename,
                         instruction_t *instructions, size_t instruction_count);
void codegen_cleanup(codegen_context_t *context);

// Code generation functions
bool detect_entry_point(ast_node_t *root);
bool generate_module(codegen_context_t *context, ast_node_t *node);
bool build_class_separately(codegen_context_t *context, ast_node_t *class_node);

// Method position tracking functions
bool codegen_start_method_tracking(codegen_context_t *context, const char *method_name);
bool codegen_end_method_tracking(codegen_context_t *context, const char *method_name);
size_t codegen_get_method_offset(codegen_context_t *context, const char *method_name);

// Unique class ID generation functions
uint64_t codegen_generate_unique_class_id(const char *module_name, const char *class_name);

bool generate_class(codegen_context_t *context, ast_node_t *node);
bool generate_field(codegen_context_t *context, ast_node_t *node);
bool generate_method(codegen_context_t *context, ast_node_t *node);
bool generate_statement(codegen_context_t *context, ast_node_t *node);
bool generate_assignment(codegen_context_t *context, ast_node_t *node);
bool generate_writeln_statement(codegen_context_t *context, const char *string_literal);
bool generate_expression(codegen_context_t *context, ast_node_t *node);
bool generate_literal(codegen_context_t *context, ast_node_t *node);
bool generate_identifier(codegen_context_t *context, ast_node_t *node);
bool generate_binary_operation(codegen_context_t *context, ast_node_t *node);
bool generate_unary_operation(codegen_context_t *context, ast_node_t *node);
bool generate_method_call(codegen_context_t *context, ast_node_t *node);
bool generate_field_access(codegen_context_t *context, ast_node_t *node);
bool generate_new_expression(codegen_context_t *context, ast_node_t *node);
void generate_new_expression_ast(codegen_context_t *context, ast_node_t *node);
bool generate_if_statement(codegen_context_t *context, ast_node_t *node);
bool generate_while_statement(codegen_context_t *context, ast_node_t *node);
bool generate_for_statement(codegen_context_t *context, ast_node_t *node);
bool generate_return_statement(codegen_context_t *context, ast_node_t *node);

// Instruction generation
void emit_instruction(codegen_context_t *context, opcode_t opcode, uint8_t level, uint64_t operand);
void emit_operation(codegen_context_t *context, opr_t operation, uint8_t level, uint64_t operand);
void emit_literal(codegen_context_t *context, uint64_t value);
void emit_load(codegen_context_t *context, uint8_t level, uint64_t address);
void emit_store(codegen_context_t *context, uint8_t level, uint64_t address);
void emit_call(codegen_context_t *context, uint8_t level, uint64_t address);
void emit_jump(codegen_context_t *context, uint64_t address);
void emit_jump_if_false(codegen_context_t *context, uint64_t address);

// Variable management functions
bool codegen_add_variable(codegen_context_t *context, const char *name, size_t *address);
bool codegen_find_variable(codegen_context_t *context, const char *name, size_t *address);

// AST-based code generation
void generate_ast_code(codegen_context_t *context, ast_node_t *node);
void generate_assignment_ast(codegen_context_t *context, ast_node_t *node);
void generate_variable_declaration_ast(codegen_context_t *context, ast_node_t *node);
void generate_expression_ast(codegen_context_t *context, ast_node_t *node);
void generate_literal_ast(codegen_context_t *context, ast_node_t *node);
void generate_identifier_ast(codegen_context_t *context, ast_node_t *node);
void generate_binary_op_ast(codegen_context_t *context, ast_node_t *node);
void generate_unary_op_ast(codegen_context_t *context, ast_node_t *node);
void generate_method_call_ast(codegen_context_t *context, ast_node_t *node);
void generate_field_access_ast(codegen_context_t *context, ast_node_t *node);

// Utility functions
void codegen_error(codegen_context_t *context, const char *message);
void codegen_warning(codegen_context_t *context, const char *message);
size_t create_label(codegen_context_t *context);
void set_label(codegen_context_t *context, size_t label_id, size_t instruction_index);
void resolve_labels(codegen_context_t *context);

// Class collection functions
bool collect_classes_from_ast(codegen_context_t *context, ast_node_t *ast, class_entry_t **classes, size_t *class_count, method_entry_t **methods, size_t *method_count, field_entry_t **fields, size_t *field_count);

// AST placeholder resolution
void resolve_ast_placeholders(ast_node_t *ast, class_entry_t *classes, size_t class_count, method_entry_t *methods, size_t method_count, field_entry_t *fields, size_t field_count);
