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
    
    if (config && config->debug_mode) {
        printf("Runtime: Initializing runtime\n");
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
    
    if (!loader_load_classes_section(&runtime->loader)) {
        printf("Error: Failed to load classes section\n");
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

bool runtime_call_main_procedure(runtime_context_t *runtime)
{
    if (runtime == NULL || !runtime->initialized) {
        return false;
    }
    
    if (runtime->config.debug_mode) {
        printf("Runtime: Looking for App.Main entry point\n");
    }
    
    // Find the App class in the class manifest
    uint64_t app_class_id = 0;
    if (!vm_resolve_class_id(&runtime->vm, "App", &app_class_id)) {
        if (runtime->config.debug_mode) {
            printf("Runtime: App class not found in class manifest\n");
        }
        return false;
    }
    
    if (runtime->config.debug_mode) {
        printf("Runtime: Found App class with ID %llu\n", (unsigned long long)app_class_id);
    }
    
    // Instantiate the App class (allocate memory for the App object)
    uint64_t app_object_address = 0;
    if (!vm_instantiate_class(&runtime->vm, app_class_id, &app_object_address)) {
        if (runtime->config.debug_mode) {
            printf("Runtime: Failed to instantiate App class\n");
        }
        return false;
    }
    
    if (runtime->config.debug_mode) {
        printf("Runtime: Instantiated App object at address 0x%llx\n", (unsigned long long)app_object_address);
    }
    
    // Check module type from header flags
    bool is_library = (runtime->vm.module_header.flags & ARXMOD_FLAG_LIBRARY) != 0;
    bool is_executable = (runtime->vm.module_header.flags & ARXMOD_FLAG_EXECUTABLE) != 0;
    
    if (runtime->config.debug_mode) {
        printf("Runtime: Module flags: 0x%08x (Library: %s, Executable: %s)\n", 
               runtime->vm.module_header.flags, 
               is_library ? "YES" : "NO", 
               is_executable ? "YES" : "NO");
    }
    
    // Handle library modules
    if (is_library) {
        if (runtime->config.debug_mode) {
            printf("Runtime: This is a library module - no entry point execution\n");
            printf("Runtime: Module contains %zu classes that can be used by other modules\n", 
                   runtime->vm.class_system.class_count);
        }
        
        // For library modules, we don't execute anything
        // The classes are loaded and available for use by other modules
        printf("Library module loaded successfully with %zu classes\n", runtime->vm.class_system.class_count);
        return true;
    }
    
    // Handle executable modules - find the entry point method
    uint64_t entry_point_address = 0;
    bool has_entry_point = false;
    
    // Use manifest to find App.Main entry point (one-time lookup, not runtime)
    if (runtime->vm.class_system.method_count > 0) {
        // Find Main method in the methods array
        for (size_t i = 0; i < runtime->vm.class_system.method_count; i++) {
            if (strcmp(runtime->vm.class_system.methods[i].method_name, "Main") == 0) {
                entry_point_address = runtime->vm.class_system.methods[i].offset;
                has_entry_point = true;
                if (runtime->config.debug_mode) {
                    printf("Runtime: Found App.Main entry point at offset %llu (from manifest)\n", 
                           (unsigned long long)entry_point_address);
                }
                break;
            }
        }
    }
    
    // Check if executable module has entry point
    if (is_executable && !has_entry_point) {
        printf("Error: Executable module declared but no entry point (App.Main) found\n");
        return false;
    }
    
    if (runtime->config.debug_mode) {
        printf("Runtime: Entry point method starts at instruction %llu (address 0x%llx)\n", 
               (unsigned long long)entry_point_address, (unsigned long long)entry_point_address);
    }
    
    // Safety check for invalid entry point
    if (entry_point_address == 0) {
        printf("Error: Entry point is 0 - no valid Main method found\n");
        return false;
    }
    
    if (entry_point_address >= runtime->vm.instruction_count) {
        printf("Error: Entry point %llu exceeds instruction count %zu\n", 
               (unsigned long long)entry_point_address, runtime->vm.instruction_count);
        return false;
    }
    
    // Set up the execution context to call Main
    // Push the object address (this pointer) onto the stack
    if (!vm_push(&runtime->vm, app_object_address)) {
        if (runtime->config.debug_mode) {
            printf("Runtime: Failed to push object address onto stack\n");
        }
        return false;
    }
    
    // Set up a call to the entry point procedure
    // The VM will execute the call instruction to jump to the entry point
    if (runtime->config.debug_mode) {
        printf("Runtime: Setting up call to entry point procedure with object context\n");
    }
    
    // Set up a proper call stack frame for the Main procedure
    // This ensures the procedure can return properly when it ends
    if (!vm_call(&runtime->vm, entry_point_address, 0)) {
        printf("Error: Failed to set up call stack frame for Main procedure\n");
        return false;
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
    
    // Find and call the Main procedure as entry point
    if (!runtime_call_main_procedure(runtime)) {
        printf("Error: Failed to find or call Main procedure\n");
        return false;
    }
    
    // Execute the program
    if (runtime->config.debug_mode) {
        printf("Starting VM execution at PC=%zu\n", runtime->vm.pc);
    }
    
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
