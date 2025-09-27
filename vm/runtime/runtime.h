/*
 * ARX Virtual Machine Runtime
 * Main runtime and execution environment
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../core/vm.h"
#include "../loader/loader.h"

// Runtime configuration
typedef struct {
    size_t stack_size;             // VM stack size
    size_t memory_size;            // VM memory size
    bool debug_mode;               // Debug output
    bool trace_execution;          // Trace instruction execution
    bool dump_state_on_error;      // Dump state on error
} runtime_config_t;

// Runtime context
typedef struct {
    arx_vm_context_t vm;           // VM context
    loader_context_t loader;       // Module loader
    runtime_config_t config;       // Runtime configuration
    bool initialized;              // Runtime initialized flag
} runtime_context_t;

// Runtime functions
bool runtime_init(runtime_context_t *runtime, const runtime_config_t *config);
void runtime_cleanup(runtime_context_t *runtime);

// Program execution
bool runtime_load_program(runtime_context_t *runtime, const char *filename);
bool runtime_execute(runtime_context_t *runtime);
bool runtime_step(runtime_context_t *runtime);

// Configuration
void runtime_set_config(runtime_context_t *runtime, const runtime_config_t *config);
void runtime_set_debug_mode(runtime_context_t *runtime, bool debug_mode);
void runtime_set_trace_execution(runtime_context_t *runtime, bool trace);

// Inspection and debugging
void runtime_dump_state(runtime_context_t *runtime);
void runtime_dump_stack(runtime_context_t *runtime, size_t count);
void runtime_dump_memory(runtime_context_t *runtime, size_t start, size_t count);
void runtime_dump_instructions(runtime_context_t *runtime, size_t start, size_t count);

// Error handling
vm_error_t runtime_get_last_error(runtime_context_t *runtime);
const char* runtime_get_error_string(runtime_context_t *runtime);

// Default configuration
extern const runtime_config_t RUNTIME_CONFIG_DEFAULT;
