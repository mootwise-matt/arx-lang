/*
 * ARX Module Loader Implementation
 * Loads and validates .arxmod files for VM execution
 */

#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

bool loader_init(loader_context_t *loader, arx_vm_context_t *vm)
{
    if (loader == NULL || vm == NULL) {
        return false;
    }
    
    memset(loader, 0, sizeof(loader_context_t));
    loader->vm = vm;
    loader->debug_output = debug_mode;
    
    if (loader->debug_output) {
        printf("ARX module loader initialized\n");
    }
    
    return true;
}

void loader_cleanup(loader_context_t *loader)
{
    if (loader != NULL) {
        arxmod_reader_cleanup(&loader->reader);
        memset(loader, 0, sizeof(loader_context_t));
    }
}

bool loader_load_module(loader_context_t *loader, const char *filename)
{
    if (loader == NULL || filename == NULL) {
        return false;
    }
    
    if (loader->debug_output) {
        printf("Loading ARX module: %s\n", filename);
    }
    
    // Initialize ARX module reader
    if (!arxmod_reader_init(&loader->reader, filename)) {
        printf("Error: Failed to initialize ARX module reader\n");
        return false;
    }
    
    // Validate module format
    if (!arxmod_reader_validate(&loader->reader)) {
        printf("Error: Invalid ARX module format\n");
        arxmod_reader_cleanup(&loader->reader);
        return false;
    }
    
    // Load table of contents
    if (!arxmod_reader_load_toc(&loader->reader)) {
        printf("Error: Failed to load table of contents\n");
        arxmod_reader_cleanup(&loader->reader);
        return false;
    }
    
    if (loader->debug_output) {
        printf("ARX module loaded successfully\n");
        arxmod_dump_info(&loader->reader);
    }
    
    return true;
}

bool loader_validate_module(loader_context_t *loader)
{
    if (loader == NULL) {
        return false;
    }
    
    // Check if module is loaded
    if (loader->reader.file == NULL) {
        printf("Error: No module loaded\n");
        return false;
    }
    
    // Validate header
    if (strncmp(loader->reader.header.magic, ARXMOD_MAGIC, 8) != 0) {
        printf("Error: Invalid magic number\n");
        return false;
    }
    
    if (loader->reader.header.version != ARXMOD_VERSION) {
        printf("Error: Unsupported version %u (expected %d)\n", 
               loader->reader.header.version, ARXMOD_VERSION);
        return false;
    }
    
    if (loader->reader.header.header_size != ARXMOD_HEADER_SIZE) {
        printf("Error: Invalid header size %llu (expected %d)\n", 
               (unsigned long long)loader->reader.header.header_size, ARXMOD_HEADER_SIZE);
        return false;
    }
    
    if (loader->debug_output) {
        printf("Module validation successful\n");
    }
    
    // Load module header into VM
    if (!vm_load_module_header(loader->vm, &loader->reader.header)) {
        printf("Error: Failed to load module header into VM\n");
        return false;
    }
    
    return true;
}

bool loader_load_code_section(loader_context_t *loader)
{
    if (loader == NULL || loader->vm == NULL) {
        return false;
    }
    
    instruction_t *instructions = NULL;
    size_t instruction_count = 0;
    
    if (!arxmod_reader_load_code_section(&loader->reader, &instructions, &instruction_count)) {
        printf("Error: Failed to load code section\n");
        return false;
    }
    
    if (instruction_count == 0) {
        printf("Warning: No code section found\n");
        return true;
    }
    
    if (!vm_load_program(loader->vm, instructions, instruction_count)) {
        printf("Error: Failed to load program into VM\n");
        return false;
    }
    
    if (loader->debug_output) {
        printf("Code section loaded: %zu instructions\n", instruction_count);
    }
    
    return true;
}

bool loader_load_classes_section(loader_context_t *loader)
{
    if (loader == NULL || loader->vm == NULL) {
        return false;
    }
    
    if (loader->debug_output) {
        printf("DEBUG: loader_load_classes_section called\n");
    }
    
    class_entry_t *classes = NULL;
    size_t class_count = 0;
    method_entry_t *methods = NULL;
    size_t method_count = 0;
    field_entry_t *fields = NULL;
    size_t field_count = 0;
    
    if (!arxmod_reader_load_classes_section(&loader->reader, &classes, &class_count, &methods, &method_count, &fields, &field_count)) {
        printf("Error: Failed to load classes section\n");
        return false;
    }
    
    if (class_count == 0) {
        if (loader->debug_output) {
            printf("No classes section found\n");
        }
        return true;
    }
    
    if (loader->debug_output) {
        printf("Loader: About to load %zu classes into VM (methods and fields not loaded separately yet)\n", 
               class_count);
    }
    
    if (!vm_load_classes(loader->vm, classes, class_count, methods, method_count, fields, field_count)) {
        printf("Error: Failed to load classes into VM\n");
        if (classes) {
            free(classes);
        }
        if (methods) {
            free(methods);
        }
        if (fields) {
            free(fields);
        }
        return false;
    }
    
    if (loader->debug_output) {
        printf("Loaded %zu classes into VM\n", class_count);
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
    
    return true;
}

bool loader_load_strings_section(loader_context_t *loader)
{
    if (loader == NULL || loader->vm == NULL) {
        return false;
    }
    
    char **strings = NULL;
    size_t string_count = 0;
    
    if (!arxmod_reader_load_strings_section(&loader->reader, &strings, &string_count)) {
        printf("Error: Failed to load strings section\n");
        return false;
    }
    
    if (string_count > 0) {
        if (!vm_load_strings(loader->vm, strings, string_count)) {
            printf("Error: Failed to load strings into VM\n");
            return false;
        }
        
        if (loader->debug_output) {
            printf("Strings section loaded: %zu strings\n", string_count);
        }
    }
    
    return true;
}

bool loader_load_symbols_section(loader_context_t *loader)
{
    if (loader == NULL) {
        return false;
    }
    
    symbol_entry_t *symbols = NULL;
    size_t symbol_count = 0;
    
    if (!arxmod_reader_load_symbols_section(&loader->reader, &symbols, &symbol_count)) {
        printf("Error: Failed to load symbols section\n");
        return false;
    }
    
    if (symbol_count > 0) {
        if (loader->debug_output) {
            printf("Symbols section loaded: %zu symbols\n", symbol_count);
        }
        
        // Free symbols (not used by VM yet)
        free(symbols);
    }
    
    return true;
}

bool loader_load_debug_section(loader_context_t *loader)
{
    if (loader == NULL) {
        return false;
    }
    
    debug_entry_t *debug_info = NULL;
    size_t debug_count = 0;
    
    if (!arxmod_reader_load_debug_section(&loader->reader, &debug_info, &debug_count)) {
        printf("Error: Failed to load debug section\n");
        return false;
    }
    
    if (debug_count > 0) {
        if (loader->debug_output) {
            printf("Debug section loaded: %zu entries\n", debug_count);
        }
        
        // Free debug info (not used by VM yet)
        free(debug_info);
    }
    
    return true;
}

void loader_dump_module_info(loader_context_t *loader)
{
    if (loader == NULL) {
        printf("Loader context is NULL\n");
        return;
    }
    
    if (loader->reader.file == NULL) {
        printf("No module loaded\n");
        return;
    }
    
    arxmod_dump_info(&loader->reader);
    arxmod_dump_sections(&loader->reader);
}

bool loader_is_valid_arxmod(const char *filename)
{
    return arxmod_validate_file(filename);
}
