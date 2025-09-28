/*
 * ARX Virtual Machine Core
 * Executes ARX bytecode instructions
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../compiler/common/opcodes.h"
#include "../../compiler/common/arxmod_constants.h"
#include "../../compiler/arxmod/arxmod.h"

// Memory management and garbage collection types
typedef struct {
    uint64_t object_id;           // Unique object identifier
    uint64_t class_id;            // Class this object belongs to
    uint64_t memory_address;      // Memory address of the object
    size_t object_size;           // Size of the object in bytes
    uint32_t reference_count;     // Number of references to this object
    bool is_alive;                // Whether object is still alive
    uint64_t creation_time;       // When object was created (for debugging)
} object_entry_t;

typedef struct {
    object_entry_t *objects;      // Array of object entries
    size_t object_count;          // Number of objects
    size_t object_capacity;       // Capacity of objects array
    uint64_t next_object_id;      // Next available object ID
    uint64_t total_allocated;     // Total memory allocated
    uint64_t total_freed;         // Total memory freed
} memory_manager_t;

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
    
    // Module information
    arxmod_header_t module_header; // Module header with flags
    
    // Class and method resolution
    struct {
        class_entry_t *classes;    // Class manifest
        size_t class_count;        // Number of classes
        size_t class_capacity;     // Class capacity
        method_entry_t *methods;   // Method manifest
        size_t method_count;       // Number of methods
        size_t method_capacity;    // Method capacity
        field_entry_t *fields;     // Field manifest
        size_t field_count;        // Number of fields
        size_t field_capacity;     // Field capacity
        uint64_t *method_addresses; // Method address table (pre-calculated by linker)
    } class_system;
    
    // Memory management and garbage collection
    memory_manager_t memory_manager;
    
    // Object context for method calls
    uint64_t current_object_address; // Current object address for field access
    
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
bool vm_load_module_header(arx_vm_context_t *vm, arxmod_header_t *header);

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

// Class and method resolution
bool vm_load_classes(arx_vm_context_t *vm, class_entry_t *classes, size_t class_count, method_entry_t *methods, size_t method_count, field_entry_t *fields, size_t field_count);
bool vm_resolve_method_address(arx_vm_context_t *vm, uint64_t class_id, const char *method_name, uint64_t *address);
bool vm_resolve_class_id(arx_vm_context_t *vm, const char *class_name, uint64_t *class_id);
bool vm_instantiate_class(arx_vm_context_t *vm, uint64_t class_id, uint64_t *object_address);

// Field access and method resolution
bool vm_get_field_offset(arx_vm_context_t *vm, uint64_t class_id, const char *field_name, uint64_t *offset);
bool vm_get_method_offset(arx_vm_context_t *vm, uint64_t class_id, const char *method_name, uint64_t *offset);
bool vm_access_field(arx_vm_context_t *vm, uint64_t object_address, uint64_t field_offset, uint64_t *value);
bool vm_get_field(arx_vm_context_t *vm, uint64_t object_address, uint64_t field_offset, uint64_t *value);
bool vm_set_field(arx_vm_context_t *vm, uint64_t object_address, uint64_t field_offset, uint64_t value);
bool vm_call_method(arx_vm_context_t *vm, uint64_t object_address, uint64_t class_id, const char *method_name, uint64_t *return_value);

// Memory management and garbage collection

bool vm_memory_manager_init(memory_manager_t *mm);
void vm_memory_manager_cleanup(memory_manager_t *mm);
uint64_t vm_allocate_object(arx_vm_context_t *vm, uint64_t class_id, size_t object_size);
bool vm_reference_object(arx_vm_context_t *vm, uint64_t object_id);
bool vm_release_object(arx_vm_context_t *vm, uint64_t object_id);
bool vm_get_object_info(arx_vm_context_t *vm, uint64_t object_id, object_entry_t **entry);
void vm_garbage_collect(arx_vm_context_t *vm);
void vm_dump_memory_manager(arx_vm_context_t *vm);

// Object access functions
typedef struct {
    uint64_t object_id;
    uint64_t class_id;
    uint64_t memory_address;
    size_t size;
    uint32_t reference_count;
} memory_object_t;

memory_object_t* memory_manager_get_object(memory_manager_t *mm, uint64_t object_address);

// Call stack functions
bool vm_push_call_stack(arx_vm_context_t *vm, uint64_t return_address);

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
    VM_ERROR_INVALID_ADDRESS,
    VM_ERROR_INVALID_STRING_ID,
    VM_ERROR_INVALID_OBJECT_ADDRESS,
    VM_ERROR_INVALID_CLASS_ID,
    VM_ERROR_METHOD_NOT_FOUND
} vm_error_t;

vm_error_t vm_get_last_error(arx_vm_context_t *vm);
const char* vm_error_to_string(vm_error_t error);
