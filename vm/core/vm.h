/*
 * ARX Virtual Machine Core
 * Executes ARX bytecode instructions
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../compiler/common/opcodes.h"

// VM execution context
typedef struct arx_vm_context {
    // Instruction execution
    instruction_t *instructions;    // Program instructions
    size_t instruction_count;      // Number of instructions
    size_t pc;                     // Program counter
    
    // Memory management
    uint64_t *stack;               // Data stack
    size_t stack_size;             // Stack size
    size_t stack_top;              // Stack top pointer
    
    uint64_t *memory;              // Global memory
    size_t memory_size;            // Memory size
    
    // Call stack for procedures
    struct {
        uint64_t *frames;          // Call frames
        size_t frame_count;        // Number of frames
        size_t frame_capacity;     // Frame capacity
        size_t current_frame;      // Current frame index
    } call_stack;
    
    // String management (UTF-8 support)
    struct {
        char **strings;            // String table (UTF-8 encoded)
        size_t string_count;       // Number of strings
        size_t string_capacity;    // String capacity
        bool utf8_enabled;         // UTF-8 support enabled
    } string_table;
    
    // Debug information
    bool debug_mode;               // Debug output
    size_t instruction_count_executed; // Instructions executed
    bool halted;                   // VM halted flag
} arx_vm_context_t;

// VM initialization and cleanup
bool vm_init(arx_vm_context_t *vm, size_t stack_size, size_t memory_size);
void vm_cleanup(arx_vm_context_t *vm);

// Program loading
bool vm_load_program(arx_vm_context_t *vm, instruction_t *instructions, size_t instruction_count);
bool vm_load_strings(arx_vm_context_t *vm, char **strings, size_t string_count);

// Execution
bool vm_execute(arx_vm_context_t *vm);
bool vm_step(arx_vm_context_t *vm);
void vm_halt(arx_vm_context_t *vm);

// Stack operations
bool vm_push(arx_vm_context_t *vm, uint64_t value);
bool vm_pop(arx_vm_context_t *vm, uint64_t *value);
bool vm_peek(arx_vm_context_t *vm, size_t offset, uint64_t *value);
bool vm_poke(arx_vm_context_t *vm, size_t offset, uint64_t value);

// Memory operations
bool vm_load(arx_vm_context_t *vm, uint64_t address, uint64_t *value);
bool vm_store(arx_vm_context_t *vm, uint64_t address, uint64_t value);

// String operations
bool vm_load_string(arx_vm_context_t *vm, uint64_t string_id, const char **string);
bool vm_store_string(arx_vm_context_t *vm, const char *string, uint64_t *string_id);

// Call stack operations
bool vm_call(arx_vm_context_t *vm, uint64_t address, uint64_t level);
bool vm_return(arx_vm_context_t *vm);
bool vm_get_base(arx_vm_context_t *vm, uint64_t level, uint64_t *base);

// Instruction execution helpers
bool vm_execute_operation(arx_vm_context_t *vm, opr_t operation, uint8_t level, uint64_t operand);
bool vm_execute_load(arx_vm_context_t *vm, uint8_t level, uint64_t offset);
bool vm_execute_store(arx_vm_context_t *vm, uint8_t level, uint64_t offset);
bool vm_execute_call(arx_vm_context_t *vm, uint8_t level, uint64_t address);
bool vm_execute_int(arx_vm_context_t *vm, uint64_t size);
bool vm_execute_loadx(arx_vm_context_t *vm, uint8_t level, uint64_t offset);
bool vm_execute_storex(arx_vm_context_t *vm, uint8_t level, uint64_t offset);

// Debug and inspection
void vm_dump_state(arx_vm_context_t *vm);
void vm_dump_stack(arx_vm_context_t *vm, size_t count);
void vm_dump_memory(arx_vm_context_t *vm, size_t start, size_t count);
void vm_dump_instructions(arx_vm_context_t *vm, size_t start, size_t count);

// Error handling
typedef enum {
    VM_ERROR_NONE = 0,
    VM_ERROR_STACK_OVERFLOW,
    VM_ERROR_STACK_UNDERFLOW,
    VM_ERROR_MEMORY_ACCESS,
    VM_ERROR_INVALID_INSTRUCTION,
    VM_ERROR_CALL_STACK_OVERFLOW,
    VM_ERROR_CALL_STACK_UNDERFLOW,
    VM_ERROR_STRING_TABLE_FULL,
    VM_ERROR_INVALID_ADDRESS
} vm_error_t;

vm_error_t vm_get_last_error(arx_vm_context_t *vm);
const char* vm_error_to_string(vm_error_t error);
