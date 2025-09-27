/*
 * ARX Module Loader
 * Loads and validates .arxmod files for VM execution
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../compiler/arxmod/arxmod.h"
#include "../core/vm.h"

// Loader context
typedef struct {
    arxmod_reader_t reader;         // ARX module reader
    arx_vm_context_t *vm;           // VM context to load into
    bool debug_output;              // Debug output flag
} loader_context_t;

// Loader functions
bool loader_init(loader_context_t *loader, arx_vm_context_t *vm);
void loader_cleanup(loader_context_t *loader);

// Module loading
bool loader_load_module(loader_context_t *loader, const char *filename);
bool loader_validate_module(loader_context_t *loader);

// Section loading
bool loader_load_code_section(loader_context_t *loader);
bool loader_load_strings_section(loader_context_t *loader);
bool loader_load_symbols_section(loader_context_t *loader);
bool loader_load_debug_section(loader_context_t *loader);

// Utility functions
void loader_dump_module_info(loader_context_t *loader);
bool loader_is_valid_arxmod(const char *filename);
