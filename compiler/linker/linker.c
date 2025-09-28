#include "linker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to resolve method name to offset within a class
static uint64_t resolve_method_offset(linker_context_t *linker, const char *class_name, const char *method_name)
{
    if (!linker || !class_name || !method_name) {
        return 0;
    }
    
    // Find the class
    for (size_t i = 0; i < linker->class_count; i++) {
        if (strcmp(linker->classes[i].class_name, class_name) == 0) {
            // Methods are stored inline after class_entry_t
            method_entry_t *current_method = (method_entry_t *)(linker->classes + i + 1);
            for (size_t j = 0; j < linker->classes[i].method_count; j++) {
                if (strcmp(current_method->method_name, method_name) == 0) {
                    return current_method->offset;
                }
                current_method++;
            }
            break;
        }
    }
    
    // Method not found, return 0 (will be handled by VM)
    return 0;
}

// Helper function to get method name from string ID (requires string table access)
static const char* get_method_name_from_id(linker_context_t *linker, uint64_t string_id, const char **string_table, size_t string_count)
{
    if (!linker || !string_table || string_id >= string_count) {
        return NULL;
    }
    
    return string_table[string_id];
}

bool linker_init(linker_context_t *linker, class_entry_t *classes, size_t class_count, method_entry_t *methods, size_t method_count, field_entry_t *fields, size_t field_count)
{
    if (!linker || !classes) {
        return false;
    }
    
    linker->classes = classes;
    linker->class_count = class_count;
    linker->methods = methods;
    linker->method_count = method_count;
    linker->fields = fields;
    linker->field_count = field_count;
    linker->base_address = 0x1000; // Start at address 0x1000
    linker->current_offset = linker->base_address;
    
    printf("Linker initialized with %zu classes, %zu methods, %zu fields\n", 
           class_count, method_count, field_count);
    return true;
}

void linker_cleanup(linker_context_t *linker)
{
    if (linker) {
        // Note: We don't free the arrays here as they're owned by the caller
        // (the code generator that collected them from the AST)
    }
}

bool linker_resolve_method_address(linker_context_t *linker, uint64_t class_id, const char *method_name, uint64_t *address)
{
    if (!linker || !method_name || !address) {
        return false;
    }
    
    // Find the class
    class_entry_t *class_entry = NULL;
    for (size_t i = 0; i < linker->class_count; i++) {
        if (linker->classes[i].class_id == class_id) {
            class_entry = &linker->classes[i];
            break;
        }
    }
    
    if (!class_entry) {
        printf("Linker: Class ID %llu not found\n", (unsigned long long)class_id);
        return false;
    }
    
    // Find the method in the method manifest
    for (size_t i = 0; i < linker->method_count; i++) {
        if (strcmp(linker->methods[i].method_name, method_name) == 0) {
            // Calculate address based on class base and method offset
            *address = linker->base_address + linker->methods[i].offset;
            
            printf("Linker: Resolved method '%s' for class %llu to address 0x%llx (offset 0x%llx)\n", 
                   method_name, (unsigned long long)class_id, (unsigned long long)*address, 
                   (unsigned long long)linker->methods[i].offset);
            return true;
        }
    }
    
    printf("Linker: Method '%s' not found for class %llu\n", method_name, (unsigned long long)class_id);
    return false;
}

bool linker_calculate_class_layout(linker_context_t *linker, uint64_t class_id, uint64_t *instance_size, field_layout_t **fields, size_t *field_count)
{
    if (!linker || !instance_size || !fields || !field_count) {
        return false;
    }
    
    // Find the class
    class_entry_t *class_entry = NULL;
    for (size_t i = 0; i < linker->class_count; i++) {
        if (linker->classes[i].class_id == class_id) {
            class_entry = &linker->classes[i];
            break;
        }
    }
    
    if (!class_entry) {
        printf("Linker: Class ID %llu not found for layout calculation\n", (unsigned long long)class_id);
        return false;
    }
    
    // Calculate instance size based on field count
    // Each field takes 8 bytes (64-bit)
    *instance_size = class_entry->field_count * 8;
    
    // Allocate field layout array
    *field_count = class_entry->field_count;
    *fields = calloc(*field_count, sizeof(field_layout_t));
    if (!*fields) {
        return false;
    }
    
    // Set up field layouts using field manifest
    size_t field_index = 0;
    for (size_t i = 0; i < linker->field_count && field_index < *field_count; i++) {
        // Find fields that belong to this class
        // Note: In a real implementation, we'd need to track which fields belong to which class
        if (field_index < *field_count) {
            (*fields)[field_index].class_id = class_id;
            (*fields)[field_index].offset = linker->fields[i].offset;
            (*fields)[field_index].size = 8; // Assume 8 bytes per field for now
            (*fields)[field_index].field_name = strdup(linker->fields[i].field_name);
            field_index++;
        }
    }
    
    printf("Linker: Class %llu layout: %llu bytes, %zu fields\n", 
           (unsigned long long)class_id, (unsigned long long)*instance_size, *field_count);
    
    return true;
}

bool linker_patch_bytecode(linker_context_t *linker, instruction_t *instructions, size_t instruction_count, const char **string_table, size_t string_count)
{
    printf("DEBUG: linker_patch_bytecode called with instruction_count=%zu, string_count=%zu\n", instruction_count, string_count);
    
    if (!linker || !instructions) {
        printf("DEBUG: linker_patch_bytecode failed - null parameters\n");
        return false;
    }
    
    printf("Linker: Patching %zu instructions\n", instruction_count);
    printf("Linker: String table has %zu strings\n", string_count);
    
    // Scan through instructions looking for method calls that need address resolution
    size_t patched_count = 0;
    
    for (size_t i = 0; i < instruction_count; i++) {
        instruction_t *inst = &instructions[i];
        uint8_t opcode = inst->opcode;
        uint64_t operand = inst->opt64;
        
        // Check if this instruction needs address resolution
        bool needs_patching = false;
        uint64_t new_address = 0;
        
        // Debug: Print instruction details
        if (i < 20) { // Only print first 20 instructions
            printf("Linker: Instruction %zu: opcode=%d, operand=%llu\n", i, opcode, (unsigned long long)operand);
        }
        
        // Skip jump instructions - they should already be resolved by resolve_labels()
        if (opcode == VM_JMP || opcode == VM_JPC) {
            if (i < 20) {
                printf("Linker: Skipping jump instruction %zu (already resolved by codegen)\n", i);
            }
            continue;
        }
        
        if (opcode == VM_CAL && operand == 0) {
            // Method call with placeholder address - this should be resolved by the compiler
            // For now, we'll leave these as placeholders since the VM handles them
            needs_patching = false;
        } else if (opcode == VM_LIT) {
            // Check if this is a string literal that needs patching
            // String literals are identified by having operand 0 (placeholder)
            // and being followed by OPR_OUTSTRING
            if (operand == 0 && i + 1 < instruction_count) {
                instruction_t *next_inst = &instructions[i + 1];
                if (next_inst->opcode == VM_OPR && next_inst->opt64 == OPR_OUTSTRING) {
                    // This is a string literal - we need to determine the correct string ID
                    // For now, we'll use a simple approach: string literals are in order
                    // TODO: Implement proper string literal resolution
                    needs_patching = true;
                    new_address = 0; // First string literal gets ID 0
                }
            }
        } else if (opcode == VM_OPR) {
            // Check for object operations that might need patching
            if (operand == OPR_OBJ_NEW) {
                // NEW object creation - operand should be class ID (already correct)
                needs_patching = false;
            } else if (operand == OPR_OBJ_CALL_METHOD) {
                // Method call - check if the previous instruction is a method name ID
                if (i > 0 && instructions[i-1].opcode == VM_LIT) {
                    uint64_t method_name_id = instructions[i-1].opt64;
                    
                    // Look up method name from string table
                    const char *method_name = NULL;
                    if (method_name_id < string_count && string_table && string_table[method_name_id]) {
                        method_name = string_table[method_name_id];
                    }
                    
                    if (method_name) {
                        // Look for method by name
                        bool found_method = false;
                        printf("Linker: Searching for method '%s' (ID: %llu) among %zu methods\n", 
                               method_name, (unsigned long long)method_name_id, linker->method_count);
                        for (size_t j = 0; j < linker->method_count; j++) {
                            if (strcmp(linker->methods[j].method_name, method_name) == 0) {
                                instructions[i-1].opt64 = linker->methods[j].offset;
                                printf("Linker: Patched method call at instruction %zu with actual offset %llu\n",
                                       i-1, (unsigned long long)linker->methods[j].offset);
                                printf("  Method: %s (ID: %llu, params: %s, return: %s)\n",
                                       linker->methods[j].method_name,
                                       (unsigned long long)linker->methods[j].method_id,
                                       linker->methods[j].param_types,
                                       linker->methods[j].return_type);
                                found_method = true;
                                break;
                            }
                        }
                        
                        if (!found_method) {
                            printf("Linker: Warning - method '%s' not found, keeping original ID\n", method_name);
                        }
                    } else {
                        printf("Linker: Warning - method name ID %llu not found in string table\n", 
                               (unsigned long long)method_name_id);
                    }
                }
                // Don't patch the OPR_OBJ_CALL_METHOD instruction itself
                needs_patching = false;
            // Field access opcodes removed - fields are accessed directly by name within class methods
            }
        }
        
        if (needs_patching) {
            // Update the instruction operand
            inst->opt64 = new_address;
            patched_count++;
            
            printf("Linker: Patched instruction %zu: opcode=%d, new_address=0x%llx\n",
                   i, opcode, (unsigned long long)new_address);
        }
    }
    
    printf("Linker: Patched %zu instructions\n", patched_count);
    return true;
}

bool linker_update_class_manifest(linker_context_t *linker, instruction_t *instructions, size_t instruction_count)
{
    if (!linker || !instructions) {
        printf("DEBUG: linker_update_class_manifest failed - null parameters\n");
        return false;
    }
    
    printf("Linker: Updating class manifest with correct method offsets\n");
    
    // Method offsets are already set correctly during method collection
    // No need to update them here as they were set to the correct instruction positions
    printf("Linker: Method offsets already set correctly during collection\n");
    return true;
}
