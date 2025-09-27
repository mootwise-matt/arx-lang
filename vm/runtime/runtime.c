/*
 * ARX Virtual Machine Runtime Implementation
 * Main runtime and execution environment
 */

#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

// Default runtime configuration
const runtime_config_t RUNTIME_CONFIG_DEFAULT = {
    .stack_size = 16384,           // 16K stack entries
    .memory_size = 65536,          // 64K memory entries
    .debug_mode = false,           // Debug output disabled
    .trace_execution = false,      // Trace execution disabled
    .dump_state_on_error = true    // Dump state on error
};

bool runtime_init(runtime_context_t *runtime, const runtime_config_t *config)
{
    if (runtime == NULL) {
        return false;
    }
    
    memset(runtime, 0, sizeof(runtime_context_t));
    
    // Use default config if none provided
    if (config != NULL) {
        runtime->config = *config;
    } else {
        runtime->config = RUNTIME_CONFIG_DEFAULT;
    }
    
    // Override debug mode from global flag
    runtime->config.debug_mode = debug_mode;
    
    // Initialize VM
    if (!vm_init(&runtime->vm, runtime->config.stack_size, runtime->config.memory_size)) {
        printf("Error: Failed to initialize VM\n");
        return false;
    }
    
    // Initialize loader
    if (!loader_init(&runtime->loader, &runtime->vm)) {
        printf("Error: Failed to initialize loader\n");
        vm_cleanup(&runtime->vm);
        return false;
    }
    
    runtime->initialized = true;
    
    if (runtime->config.debug_mode) {
        printf("ARX VM runtime initialized\n");
        printf("  Stack size: %zu\n", runtime->config.stack_size);
        printf("  Memory size: %zu\n", runtime->config.memory_size);
        printf("  Debug mode: %s\n", runtime->config.debug_mode ? "enabled" : "disabled");
        printf("  Trace execution: %s\n", runtime->config.trace_execution ? "enabled" : "disabled");
    }
    
    return true;
}

void runtime_cleanup(runtime_context_t *runtime)
{
    if (runtime != NULL) {
        loader_cleanup(&runtime->loader);
        vm_cleanup(&runtime->vm);
        memset(runtime, 0, sizeof(runtime_context_t));
    }
}

bool runtime_load_program(runtime_context_t *runtime, const char *filename)
{
    if (runtime == NULL || filename == NULL || !runtime->initialized) {
        return false;
    }
    
    if (runtime->config.debug_mode) {
        printf("Loading program: %s\n", filename);
    }
    
    // Load ARX module
    if (!loader_load_module(&runtime->loader, filename)) {
        printf("Error: Failed to load ARX module\n");
        return false;
    }
    
    // Validate module
    if (!loader_validate_module(&runtime->loader)) {
        printf("Error: Module validation failed\n");
        return false;
    }
    
    // Load all sections
    if (!loader_load_code_section(&runtime->loader)) {
        printf("Error: Failed to load code section\n");
        return false;
    }
    
    if (!loader_load_strings_section(&runtime->loader)) {
        printf("Error: Failed to load strings section\n");
        return false;
    }
    
    if (!loader_load_symbols_section(&runtime->loader)) {
        printf("Error: Failed to load symbols section\n");
        return false;
    }
    
    if (!loader_load_debug_section(&runtime->loader)) {
        printf("Error: Failed to load debug section\n");
        return false;
    }
    
    if (runtime->config.debug_mode) {
        printf("Program loaded successfully\n");
    }
    
    return true;
}

bool runtime_execute(runtime_context_t *runtime)
{
    if (runtime == NULL || !runtime->initialized) {
        return false;
    }
    
    if (runtime->config.debug_mode) {
        printf("Starting program execution\n");
    }
    
    // Execute the program
    bool success = vm_execute(&runtime->vm);
    
    if (runtime->config.debug_mode) {
        if (success) {
            printf("Program execution completed successfully\n");
        } else {
            printf("Program execution failed: %s\n", 
                   vm_error_to_string(vm_get_last_error(&runtime->vm)));
        }
    }
    
    // Dump state on error if configured
    if (!success && runtime->config.dump_state_on_error) {
        runtime_dump_state(runtime);
    }
    
    return success;
}

bool runtime_step(runtime_context_t *runtime)
{
    if (runtime == NULL || !runtime->initialized) {
        return false;
    }
    
    bool success = vm_step(&runtime->vm);
    
    if (runtime->config.trace_execution) {
        if (success) {
            printf("Step executed successfully\n");
        } else {
            printf("Step execution failed: %s\n", 
                   vm_error_to_string(vm_get_last_error(&runtime->vm)));
        }
    }
    
    return success;
}

void runtime_set_config(runtime_context_t *runtime, const runtime_config_t *config)
{
    if (runtime != NULL && config != NULL) {
        runtime->config = *config;
    }
}

void runtime_set_debug_mode(runtime_context_t *runtime, bool debug_mode)
{
    if (runtime != NULL) {
        runtime->config.debug_mode = debug_mode;
        runtime->vm.debug_mode = debug_mode;
        runtime->loader.debug_output = debug_mode;
    }
}

void runtime_set_trace_execution(runtime_context_t *runtime, bool trace)
{
    if (runtime != NULL) {
        runtime->config.trace_execution = trace;
    }
}

void runtime_dump_state(runtime_context_t *runtime)
{
    if (runtime == NULL || !runtime->initialized) {
        printf("Runtime not initialized\n");
        return;
    }
    
    printf("\n=== ARX VM Runtime State ===\n");
    vm_dump_state(&runtime->vm);
    
    if (runtime->config.debug_mode) {
        loader_dump_module_info(&runtime->loader);
    }
}

void runtime_dump_stack(runtime_context_t *runtime, size_t count)
{
    if (runtime != NULL && runtime->initialized) {
        vm_dump_stack(&runtime->vm, count);
    }
}

void runtime_dump_memory(runtime_context_t *runtime, size_t start, size_t count)
{
    if (runtime != NULL && runtime->initialized) {
        vm_dump_memory(&runtime->vm, start, count);
    }
}

void runtime_dump_instructions(runtime_context_t *runtime, size_t start, size_t count)
{
    if (runtime != NULL && runtime->initialized) {
        vm_dump_instructions(&runtime->vm, start, count);
    }
}

vm_error_t runtime_get_last_error(runtime_context_t *runtime)
{
    if (runtime != NULL && runtime->initialized) {
        return vm_get_last_error(&runtime->vm);
    }
    return VM_ERROR_INVALID_ADDRESS;
}

const char* runtime_get_error_string(runtime_context_t *runtime)
{
    if (runtime != NULL && runtime->initialized) {
        return vm_error_to_string(vm_get_last_error(&runtime->vm));
    }
    return "Runtime not initialized";
}
