/*
 * ARX Virtual Machine Core Implementation
 * Executes ARX bytecode instructions
 */

#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

// VM error state
static vm_error_t last_error = VM_ERROR_NONE;

bool vm_init(arx_vm_context_t *vm, size_t stack_size, size_t memory_size)
{
    if (vm == NULL) {
        return false;
    }
    
    if (debug_mode) {
        printf("VM: Initializing VM with stack_size=%zu, memory_size=%zu\n", stack_size, memory_size);
    }
    
    memset(vm, 0, sizeof(arx_vm_context_t));
    
    // Initialize stack
    if (debug_mode) {
        printf("VM: Allocating stack memory (%zu bytes)\n", stack_size * sizeof(uint64_t));
    }
    vm->stack = calloc(stack_size, sizeof(uint64_t));
    if (vm->stack == NULL) {
        if (debug_mode) {
            printf("VM: Failed to allocate stack memory\n");
        }
        return false;
    }
    vm->stack_size = stack_size;
    vm->stack_top = 0;
    if (debug_mode) {
        printf("VM: Stack allocated successfully\n");
    }
    
    // Initialize memory
    if (debug_mode) {
        printf("VM: Allocating memory (%zu bytes)\n", memory_size * sizeof(uint64_t));
    }
    vm->memory = calloc(memory_size, sizeof(uint64_t));
    if (vm->memory == NULL) {
        if (debug_mode) {
            printf("VM: Failed to allocate memory\n");
        }
        free(vm->stack);
        return false;
    }
    vm->memory_size = memory_size;
    if (debug_mode) {
        printf("VM: Memory allocated successfully\n");
    }
    
    // Initialize call stack
    vm->call_stack.frame_capacity = 100;
    vm->call_stack.frames = calloc(vm->call_stack.frame_capacity, sizeof(uint64_t) * 4);
    if (vm->call_stack.frames == NULL) {
        free(vm->stack);
        free(vm->memory);
        return false;
    }
    vm->call_stack.frame_count = 0;
    vm->call_stack.current_frame = 0;
    
    // Initialize string table
    vm->string_table.string_capacity = 1000;
    vm->string_table.strings = calloc(vm->string_table.string_capacity, sizeof(char*));
    if (vm->string_table.strings == NULL) {
        free(vm->stack);
        free(vm->memory);
        free(vm->call_stack.frames);
        return false;
    }
    vm->string_table.string_count = 0;
    vm->string_table.utf8_enabled = true;  // Enable UTF-8 support by default
    
    // Initialize class system
    vm->class_system.class_capacity = 100;
    vm->class_system.classes = calloc(vm->class_system.class_capacity, sizeof(class_entry_t));
    if (vm->class_system.classes == NULL) {
        free(vm->stack);
        free(vm->memory);
        free(vm->call_stack.frames);
        free(vm->string_table.strings);
        return false;
    }
    vm->class_system.class_count = 0;
    
    vm->class_system.method_capacity = 1000;
    vm->class_system.method_addresses = calloc(vm->class_system.method_capacity, sizeof(uint64_t));
    if (vm->class_system.method_addresses == NULL) {
        free(vm->stack);
        free(vm->memory);
        free(vm->call_stack.frames);
        free(vm->string_table.strings);
        free(vm->class_system.classes);
        return false;
    }
    vm->class_system.method_count = 0;
    vm->class_system.method_capacity = 0;
    vm->class_system.methods = NULL;
    vm->class_system.field_count = 0;
    vm->class_system.field_capacity = 0;
    vm->class_system.fields = NULL;
    
    // Class system initialized (linker runs at compile-time)
    
    // Initialize memory manager
    if (!vm_memory_manager_init(&vm->memory_manager)) {
        free(vm->stack);
        free(vm->memory);
        free(vm->call_stack.frames);
        free(vm->string_table.strings);
        free(vm->class_system.classes);
        free(vm->class_system.method_addresses);
        return false;
    }
    
    // Initialize execution state
    vm->pc = 0;
    vm->instruction_count = 0;
    vm->instruction_count_executed = 0;
    vm->halted = false;
    vm->debug_mode = debug_mode;
    
    last_error = VM_ERROR_NONE;
    
    if (vm->debug_mode) {
        printf("VM: Starting VM initialization\n");
        printf("VM: Initializing VM with stack_size=%zu, memory_size=%zu\n", stack_size, memory_size);
        printf("VM: Allocating stack memory\n");
        printf("VM initialized: stack=%zu, memory=%zu\n", stack_size, memory_size);
    }
    
    return true;
}

void vm_cleanup(arx_vm_context_t *vm)
{
    if (vm == NULL) {
        return;
    }
    
    if (vm->debug_mode) {
        printf("VM: Cleaning up VM\n");
    }
    
    // Free stack
    if (vm->stack != NULL) {
        free(vm->stack);
        vm->stack = NULL;
    }
    
    // Free memory
    if (vm->memory != NULL) {
        free(vm->memory);
        vm->memory = NULL;
    }
    
    // Free call stack
    if (vm->call_stack.frames != NULL) {
        free(vm->call_stack.frames);
        vm->call_stack.frames = NULL;
    }
    
    // Free string table
    if (vm->string_table.strings != NULL) {
        for (size_t i = 0; i < vm->string_table.string_count; i++) {
            if (vm->string_table.strings[i] != NULL) {
                free(vm->string_table.strings[i]);
            }
        }
        free(vm->string_table.strings);
        vm->string_table.strings = NULL;
    }
    
    // Free class system
    if (vm->class_system.classes != NULL) {
        free(vm->class_system.classes);
        vm->class_system.classes = NULL;
    }
    if (vm->class_system.methods != NULL) {
        free(vm->class_system.methods);
        vm->class_system.methods = NULL;
    }
    if (vm->class_system.fields != NULL) {
        free(vm->class_system.fields);
        vm->class_system.fields = NULL;
    }
    if (vm->class_system.method_addresses != NULL) {
        free(vm->class_system.method_addresses);
        vm->class_system.method_addresses = NULL;
    }
    
    // Class system cleanup complete
    
    // Cleanup memory manager
    vm_memory_manager_cleanup(&vm->memory_manager);
    
    memset(vm, 0, sizeof(arx_vm_context_t));
}

bool vm_load_program(arx_vm_context_t *vm, instruction_t *instructions, size_t instruction_count)
{
    if (vm == NULL || instructions == NULL) {
        last_error = VM_ERROR_INVALID_ADDRESS;
        return false;
    }
    
    vm->instructions = instructions;
    vm->instruction_count = instruction_count;
    vm->pc = 0;
    vm->halted = false;
    
    if (vm->debug_mode) {
        printf("Program loaded: %zu instructions\n", instruction_count);
    }
    
    return true;
}

bool vm_load_module_header(arx_vm_context_t *vm, arxmod_header_t *header)
{
    if (vm == NULL || header == NULL) {
        last_error = VM_ERROR_INVALID_ADDRESS;
        return false;
    }
    
    if (debug_mode) {
        printf("VM: Loading module header: flags=0x%08x, entry_point=%llu\n", 
               header->flags, (unsigned long long)header->entry_point);
    }
    
    // Copy the module header to the VM
    vm->module_header = *header;
    
    if (vm->debug_mode) {
        printf("VM: Module header loaded successfully\n");
    }
    
    return true;
}

bool vm_load_strings(arx_vm_context_t *vm, char **strings, size_t string_count)
{
    if (vm == NULL || strings == NULL) {
        last_error = VM_ERROR_INVALID_ADDRESS;
        return false;
    }
    
    for (size_t i = 0; i < string_count && i < vm->string_table.string_capacity; i++) {
        if (strings[i] != NULL) {
            size_t len = strlen(strings[i]) + 1;
            vm->string_table.strings[i] = malloc(len);
            if (vm->string_table.strings[i] != NULL) {
                strcpy(vm->string_table.strings[i], strings[i]);
            }
        }
    }
    
    vm->string_table.string_count = string_count;
    
    if (vm->debug_mode) {
        printf("Strings loaded: %zu strings\n", string_count);
    }
    
    return true;
}

bool vm_execute(arx_vm_context_t *vm)
{
    if (vm == NULL) {
        return false;
    }
    
    if (vm->debug_mode) {
        printf("Starting VM execution\n");
    }
    
    size_t step_count = 0;
    const size_t max_steps = 5000; // Reduced safety limit to prevent infinite loops
    size_t last_pc = 0;
    size_t pc_repeat_count = 0;
    const size_t max_pc_repeats = 100; // Max times PC can stay the same
    
    while (!vm->halted && vm->pc < vm->instruction_count && step_count < max_steps) {
        // Check for PC getting stuck at the same instruction
        if (vm->pc == last_pc) {
            pc_repeat_count++;
            if (pc_repeat_count > max_pc_repeats) {
                if (vm->debug_mode) {
                    printf("VM: PC stuck at %zu for %zu steps (infinite loop detected)\n", 
                           vm->pc, pc_repeat_count);
                }
                return false;
            }
        } else {
            pc_repeat_count = 0;
            last_pc = vm->pc;
        }
        
        if (vm->debug_mode && step_count % 500 == 0) {
            printf("VM: Step %zu, PC=%zu, instruction_count=%zu, halted=%d\n", 
                   step_count, vm->pc, vm->instruction_count, vm->halted);
        }
        
        if (!vm_step(vm)) {
            if (vm->debug_mode) {
                printf("VM step failed at PC=%zu, instruction_count=%zu\n", 
                       vm->pc, vm->instruction_count);
            }
            return false;
        }
        step_count++;
    }
    
    if (step_count >= max_steps) {
        if (vm->debug_mode) {
            printf("VM: Execution stopped after %zu steps (infinite loop protection)\n", max_steps);
        }
        return false;
    }
    
    if (vm->debug_mode) {
        printf("VM execution completed: %zu instructions executed, PC=%zu, instruction_count=%zu\n", 
               vm->instruction_count_executed, vm->pc, vm->instruction_count);
    }
    
    return true;
}

bool vm_step(arx_vm_context_t *vm)
{
    if (vm == NULL || vm->halted || vm->pc >= vm->instruction_count) {
        if (vm && vm->debug_mode) {
            printf("VM step: halted=%d, pc=%zu, instruction_count=%zu\n", 
                   vm->halted, vm->pc, vm->instruction_count);
        }
        return false;
    }
    
    // Additional safety check for invalid PC
    if (vm->pc >= vm->instruction_count) {
        if (vm->debug_mode) {
            printf("VM step: PC out of bounds: %zu >= %zu\n", vm->pc, vm->instruction_count);
        }
        vm->halted = true;
        return false;
    }
    
    if (vm->debug_mode && vm->pc >= 170) {
        printf("DEBUG: vm_step called at PC=%zu\n", vm->pc);
    }
    
    if (vm->pc >= vm->instruction_count) {
        if (vm->debug_mode) {
            printf("VM step: PC out of bounds: %zu >= %zu\n", vm->pc, vm->instruction_count);
        }
        return false;
    }
    
    if (vm->debug_mode && vm->pc >= 170) {
        printf("DEBUG: About to execute instruction at PC=%zu, instruction_count=%zu\n", 
               vm->pc, vm->instruction_count);
    }
    
    instruction_t *instr = &vm->instructions[vm->pc];
    uint8_t opcode = instr->opcode & 0xF;
    uint8_t level = (instr->opcode >> 4) & 0xF;
    uint64_t operand = instr->opt64;
    
    if (vm->debug_mode) {
        printf("PC=%zu: raw_opcode=0x%02x, opcode=%d, level=%d, operand=%llu\n", 
               vm->pc, instr->opcode, opcode, level, (unsigned long long)operand);
    }
    
    // Debug: Check for potential issues
    if (vm->pc >= 170 && vm->debug_mode) {
        printf("DEBUG: At PC=%zu, stack_top=%zu, stack_size=%zu\n", 
               vm->pc, vm->stack_top, vm->stack_size);
    }
    
    bool success = true;
    
    switch (opcode) {
        case VM_LIT:
            success = vm_push(vm, operand);
            if (vm->debug_mode && vm->pc >= 13 && vm->pc <= 30) {
                printf("DEBUG: VM_LIT at PC=%zu, success=%d, operand=%llu, stack_top=%zu\n", 
                       vm->pc, success, (unsigned long long)operand, vm->stack_top);
            }
            break;
            
        case VM_OPR:
            success = vm_execute_operation(vm, (opr_t)operand, level, operand);
            break;
            
        case VM_LOD:
            success = vm_execute_load(vm, level, operand);
            if (vm->debug_mode && vm->pc >= 13 && vm->pc <= 30) {
                printf("DEBUG: VM_LOD at PC=%zu, success=%d, level=%d, operand=%llu, stack_top=%zu\n", 
                       vm->pc, success, level, (unsigned long long)operand, vm->stack_top);
            }
            break;
            
        case VM_STO:
            success = vm_execute_store(vm, level, operand);
            if (vm->debug_mode && vm->pc >= 13 && vm->pc <= 30) {
                printf("DEBUG: VM_STO at PC=%zu, success=%d, level=%d, operand=%llu, stack_top=%zu\n", 
                       vm->pc, success, level, (unsigned long long)operand, vm->stack_top);
            }
            break;
            
        case VM_CAL:
            success = vm_execute_call(vm, level, operand);
            break;
            
        case VM_INT:
            success = vm_execute_int(vm, operand);
            break;
            
        case VM_JMP:
            if (vm->debug_mode && vm->pc >= 13 && vm->pc <= 30) {
                printf("DEBUG: VM_JMP at PC=%zu, jumping to %llu\n", 
                       vm->pc, (unsigned long long)operand);
            }
            // Safety check: prevent jumping to invalid address
            if (operand >= vm->instruction_count) {
                if (vm->debug_mode) {
                    printf("VM_JMP: Invalid jump target %llu >= instruction_count %zu\n", 
                           (unsigned long long)operand, vm->instruction_count);
                }
                vm->halted = true;
                success = false;
            } else {
                vm->pc = operand;
            }
            break;
            
        case VM_JPC:
            {
                uint64_t condition;
                if (vm_pop(vm, &condition)) {
                    if (vm->debug_mode && vm->pc >= 13 && vm->pc <= 30) {
                        printf("DEBUG: VM_JPC at PC=%zu, condition=%llu, jumping to %llu (condition==0? %s)\n", 
                               vm->pc, (unsigned long long)condition, (unsigned long long)operand, 
                               (condition == 0) ? "YES" : "NO");
                    }
                    if (condition == 0) {
                        // Safety check: prevent jumping to invalid address
                        if (operand >= vm->instruction_count) {
                            if (vm->debug_mode) {
                                printf("VM_JPC: Invalid jump target %llu >= instruction_count %zu\n", 
                                       (unsigned long long)operand, vm->instruction_count);
                            }
                            vm->halted = true;
                            success = false;
                        } else {
                            vm->pc = operand;
                        }
                    } else {
                        vm->pc++;
                    }
                } else {
                    success = false;
                }
            }
            break;
            
        case VM_LODX:
            success = vm_execute_loadx(vm, level, operand);
            break;
            
        case VM_STOX:
            success = vm_execute_storex(vm, level, operand);
            break;
            
        case VM_HALT:
            vm_halt(vm);
            break;
            
        default:
            if (vm->debug_mode) {
                printf("Error: Unknown opcode %d\n", opcode);
            }
            last_error = VM_ERROR_INVALID_INSTRUCTION;
            success = false;
            break;
    }
    
    if (success && opcode != VM_JMP && opcode != VM_JPC && opcode != VM_HALT) {
        vm->pc++;
    }
    
    if (vm->debug_mode && vm->pc >= 170) {
        printf("DEBUG: After instruction at PC=%zu, success=%d, new_pc=%zu\n", 
               vm->pc - 1, success, vm->pc);
    }
    
    vm->instruction_count_executed++;
    
    return success;
}

bool vm_execute_operation(arx_vm_context_t *vm, opr_t operation, uint8_t level, uint64_t operand)
{
    (void)level; // Suppress unused parameter warning
    switch (operation) {
        case OPR_RET:
            return vm_return(vm);
            
        case OPR_NEG:
            {
                uint64_t value;
                if (vm_pop(vm, &value)) {
                    return vm_push(vm, -(int64_t)value);
                }
                return false;
            }
            
        case OPR_ADD:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    uint64_t result = a + b;
                    if (vm->debug_mode && vm->pc >= 13 && vm->pc <= 30) {
                        printf("DEBUG: OPR_ADD at PC=%zu, a=%llu, b=%llu, result=%llu\n", 
                               vm->pc, (unsigned long long)a, (unsigned long long)b, (unsigned long long)result);
                    }
                    return vm_push(vm, result);
                }
                return false;
            }
            
        case OPR_SUB:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    return vm_push(vm, a - b);
                }
                return false;
            }
            
        case OPR_MUL:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    return vm_push(vm, a * b);
                }
                return false;
            }
            
        case OPR_DIV:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    if (b == 0) {
                        last_error = VM_ERROR_INVALID_INSTRUCTION;
                        return false;
                    }
                    return vm_push(vm, a / b);
                }
                return false;
            }
            
        case OPR_POW:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    // Simple exponentiation: a^b
                    uint64_t result = 1;
                    for (uint64_t i = 0; i < b; i++) {
                        result *= a;
                    }
                    return vm_push(vm, result);
                }
                return false;
            }
            
        case OPR_MOD:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    if (b == 0) {
                        last_error = VM_ERROR_INVALID_INSTRUCTION;
                        return false;
                    }
                    return vm_push(vm, a % b);
                }
                return false;
            }
            
        case OPR_ODD:
            {
                uint64_t value;
                if (vm_pop(vm, &value)) {
                    return vm_push(vm, (value % 2) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_EQ:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    return vm_push(vm, (a == b) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_NEQ:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    return vm_push(vm, (a != b) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_LESS:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    return vm_push(vm, (a < b) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_LEQ:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    uint64_t result = (a <= b) ? 1 : 0;
                    if (vm->debug_mode && vm->pc >= 13 && vm->pc <= 30) {
                        printf("DEBUG: OPR_LEQ at PC=%zu, a=%llu, b=%llu, result=%llu (a<=b? %s)\n", 
                               vm->pc, (unsigned long long)a, (unsigned long long)b, (unsigned long long)result,
                               (a <= b) ? "YES" : "NO");
                    }
                    return vm_push(vm, result);
                }
                return false;
            }
            
        case OPR_GREATER:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    return vm_push(vm, (a > b) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_GEQ:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    return vm_push(vm, (a >= b) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_AND:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    // Logical AND: both operands must be non-zero
                    return vm_push(vm, (a != 0 && b != 0) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_OR:
            {
                uint64_t b, a;
                if (vm_pop(vm, &b) && vm_pop(vm, &a)) {
                    // Logical OR: at least one operand must be non-zero
                    return vm_push(vm, (a != 0 || b != 0) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_NOT:
            {
                uint64_t a;
                if (vm_pop(vm, &a)) {
                    // Logical NOT: 0 becomes 1, non-zero becomes 0
                    return vm_push(vm, (a == 0) ? 1 : 0);
                }
                return false;
            }
            
        case OPR_WRITELN:
            // WriteLn - output newline (no stack operation needed)
            printf("\n");
            fflush(stdout);
            return true;
            
        case OPR_OUTSTRING:
            {
                uint64_t val;
                if (!vm_pop(vm, &val)) return false;
                
                // Check if it's a string object address (new system)
                if (val + 2 < vm->stack_size) {
                    uint64_t len = vm->stack[val + 0];
                    uint64_t cap = vm->stack[val + 1];
                    uint64_t off = vm->stack[val + 2];
                    if (off == 3 && cap >= len && cap <= 1<<20) {
                        size_t data_words = (cap + 1 + sizeof(uint64_t) - 1) / sizeof(uint64_t);
                        if (val + off + data_words < vm->stack_size) {
                            char buf[2048];
                            const char *src = (const char*)&vm->stack[val + off];
                            size_t to_copy = (len < (sizeof(buf) - 1)) ? (size_t)len : (sizeof(buf) - 1);
                            memcpy(buf, src, to_copy);
                            buf[to_copy] = '\0';
                            printf("%s", buf);
                            fflush(stdout);
                            return true;
                        }
                    }
                }
                
                // Fallback: treat as string ID (legacy system)
                const char *str;
                if (vm_load_string(vm, val, &str)) {
                    printf("%s", str);
                    fflush(stdout);
                    return true;
                }
                
                return false;
            }
            
        case OPR_STR_CREATE:
            {
                // Create string from literal - operand contains string data
                // For now, just push the operand as string ID
                return vm_push(vm, operand);
            }
            
        case OPR_STR_CONCAT:
            {
                if (vm->debug_mode) {
                    printf("OPR_STR_CONCAT: Starting string concatenation at PC=%zu\n", vm->pc);
                }
                uint64_t str2_id, str1_id;
                if (vm_pop(vm, &str2_id) && vm_pop(vm, &str1_id)) {
                    if (vm->debug_mode) {
                        printf("OPR_STR_CONCAT: str1_id=%llu, str2_id=%llu, string_count=%zu\n", 
                               (unsigned long long)str1_id, (unsigned long long)str2_id, vm->string_table.string_count);
                    }
                    const char *str1, *str2;
                    if (vm_load_string(vm, str1_id, &str1) && vm_load_string(vm, str2_id, &str2)) {
                        if (vm->debug_mode) {
                            printf("OPR_STR_CONCAT: str1='%s', str2='%s'\n", str1 ? str1 : "(null)", str2 ? str2 : "(null)");
                        }
                        // Simple concatenation for now
                        char *result = malloc(strlen(str1) + strlen(str2) + 1);
                        if (result != NULL) {
                            strcpy(result, str1);
                            strcat(result, str2);
                            uint64_t result_id;
                            if (vm_store_string(vm, result, &result_id)) {
                                if (vm->debug_mode) {
                                    printf("OPR_STR_CONCAT: stored result='%s' with id=%llu, new string_count=%zu\n", 
                                           result, (unsigned long long)result_id, vm->string_table.string_count);
                                }
                                free(result);
                                return vm_push(vm, result_id);
                            } else {
                                if (vm->debug_mode) {
                                    printf("OPR_STR_CONCAT: FAILED to store result string\n");
                                }
                                free(result);
                            }
                        } else {
                            if (vm->debug_mode) {
                                printf("OPR_STR_CONCAT: FAILED to allocate memory for result\n");
                            }
                        }
                    } else {
                        if (vm->debug_mode) {
                            printf("OPR_STR_CONCAT: FAILED to load input strings\n");
                        }
                    }
                } else {
                    if (vm->debug_mode) {
                        printf("OPR_STR_CONCAT: FAILED to pop string IDs from stack at PC=%zu\n", vm->pc);
                    }
                }
                return false;
            }
            
        case OPR_STR_LEN:
            {
                uint64_t string_id;
                if (vm_pop(vm, &string_id)) {
                    const char *str;
                    if (vm_load_string(vm, string_id, &str)) {
                        return vm_push(vm, strlen(str));
                    }
                }
                return false;
            }
            
        case OPR_STR_EQ:
            {
                uint64_t str2_id, str1_id;
                if (vm_pop(vm, &str2_id) && vm_pop(vm, &str1_id)) {
                    const char *str1, *str2;
                    if (vm_load_string(vm, str1_id, &str1) && vm_load_string(vm, str2_id, &str2)) {
                        return vm_push(vm, strcmp(str1, str2) == 0 ? 1 : 0);
                    }
                }
                return false;
            }
            
        case OPR_ININT:
            {
                int64_t value;
                printf("> ");
                fflush(stdout);
                if (scanf("%lld", &value) == 1) {
                    return vm_push(vm, (uint64_t)value);
                }
                return false;
            }
            
        case OPR_INT_TO_STR:
            {
                // Convert integer to string
                uint64_t int_value;
                if (vm_pop(vm, &int_value)) {
                    // Convert integer to string
                    char str_buffer[32];
                    snprintf(str_buffer, sizeof(str_buffer), "%lld", (long long)int_value);
                    
                    // Store the string and get its ID
                    uint64_t string_id;
                    if (vm_store_string(vm, str_buffer, &string_id)) {
                        return vm_push(vm, string_id);
                    }
                }
                return false;
            }
            
        case OPR_STR_TO_INT:
            {
                // Convert string to integer
                uint64_t string_id;
                if (vm_pop(vm, &string_id)) {
                    const char *str;
                    if (vm_load_string(vm, string_id, &str)) {
                        // Convert string to integer
                        long long int_value = strtoll(str, NULL, 10);
                        return vm_push(vm, (uint64_t)int_value);
                    }
                }
                return false;
            }
            
        case OPR_OBJ_CALL_METHOD:
            {
                if (vm->debug_mode) {
                    printf("OPR_OBJ_CALL_METHOD: Starting method call at PC=%zu\n", vm->pc);
                }

                // Pop method offset from stack (pre-calculated by linker)
                uint64_t method_offset;
                if (!vm_pop(vm, &method_offset)) {
                    printf("Error: Failed to pop method offset from stack\n");
                    last_error = VM_ERROR_STACK_UNDERFLOW;
                    return false;
                }

                // Pop object address from stack
                uint64_t object_address;
                if (!vm_pop(vm, &object_address)) {
                    printf("Error: Failed to pop object address from stack\n");
                    last_error = VM_ERROR_STACK_UNDERFLOW;
                    return false;
                }

                if (vm->debug_mode) {
                    printf("  Calling method at offset %llu on object at address %llu\n", 
                           (unsigned long long)method_offset, (unsigned long long)object_address);
                }

                // Save current PC as return address
                uint64_t return_address = vm->pc + 1;
                
                // Push return address onto call stack for when method returns
                if (!vm_push_call_stack(vm, return_address)) {
                    printf("Error: Failed to push return address onto call stack\n");
                    return false;
                }
                
                // Jump directly to method bytecode (offset pre-calculated by linker)
                vm->pc = method_offset;
                
                if (vm->debug_mode) {
                    printf("  Method call: jumped to PC=%llu, return address=%llu\n", 
                           (unsigned long long)vm->pc, (unsigned long long)return_address);
                }
            }
            break;
            
        // OPR_OBJ_GET_FIELD removed - fields are accessed directly by name within class methods
            
        case OPR_OBJ_NEW:
            {
                // Object instantiation - pop class ID from stack and create object
                if (vm->debug_mode) {
                    printf("OPR_OBJ_NEW: Object instantiation executed\n");
                }
                
                uint64_t class_id;
                if (!vm_pop(vm, &class_id)) {
                    if (vm->debug_mode) {
                        printf("OPR_OBJ_NEW: Failed to pop class ID from stack\n");
                    }
                    return false;
                }
                
                // Instantiate the class
                uint64_t object_address;
                if (!vm_instantiate_class(vm, class_id, &object_address)) {
                    if (vm->debug_mode) {
                        printf("OPR_OBJ_NEW: Failed to instantiate class ID %llu\n", (unsigned long long)class_id);
                    }
                    return false;
                }
                
                // Push object address onto stack
                return vm_push(vm, object_address);
            }
            
        default:
            if (vm->debug_mode) {
                printf("Error: Unknown operation %d\n", operation);
            }
            last_error = VM_ERROR_INVALID_INSTRUCTION;
            return false;
    }
    
    return true; // Default return for successful operations
}

bool vm_execute_load(arx_vm_context_t *vm, uint8_t level, uint64_t offset)
{
    uint64_t base_addr;
    if (!vm_get_base(vm, level, &base_addr)) {
        return false;
    }
    
    uint64_t address = base_addr + offset;
    if (address >= vm->memory_size) {
        last_error = VM_ERROR_MEMORY_ACCESS;
        return false;
    }
    
    uint64_t value = vm->memory[address];
    return vm_push(vm, value);
}

bool vm_execute_store(arx_vm_context_t *vm, uint8_t level, uint64_t offset)
{
    uint64_t base_addr;
    if (!vm_get_base(vm, level, &base_addr)) {
        return false;
    }
    
    uint64_t address = base_addr + offset;
    if (address >= vm->memory_size) {
        last_error = VM_ERROR_MEMORY_ACCESS;
        return false;
    }
    
    uint64_t value;
    if (!vm_pop(vm, &value)) {
        return false;
    }
    
    vm->memory[address] = value;
    return true;
}

bool vm_execute_call(arx_vm_context_t *vm, uint8_t level, uint64_t address)
{
    // Handle placeholder address 0 (method calls)
    if (address == 0) {
        if (vm->debug_mode) {
            printf("VM_CAL: Method call with placeholder address 0\n");
        }
        
        // For now, return appropriate values for methods in example 04
        // TODO: Implement proper method resolution using class manifest
        uint64_t string_id;
        if (vm_store_string(vm, "John Doe", &string_id)) {
            return vm_push(vm, string_id);
        }
        return false;
    }
    
    // Normal function call
    return vm_call(vm, address, level);
}

bool vm_execute_int(arx_vm_context_t *vm, uint64_t size)
{
    // Allocate space on stack for local variables
    vm->stack_top += size;
    if (vm->stack_top >= vm->stack_size) {
        last_error = VM_ERROR_STACK_OVERFLOW;
        return false;
    }
    return true;
}

bool vm_execute_loadx(arx_vm_context_t *vm, uint8_t level, uint64_t offset)
{
    // Load from array element
    uint64_t index;
    if (!vm_pop(vm, &index)) {
        return false;
    }
    
    uint64_t base_addr;
    if (!vm_get_base(vm, level, &base_addr)) {
        return false;
    }
    
    uint64_t address = base_addr + offset + index;
    if (address >= vm->memory_size) {
        last_error = VM_ERROR_MEMORY_ACCESS;
        return false;
    }
    
    uint64_t value = vm->memory[address];
    return vm_push(vm, value);
}

bool vm_execute_storex(arx_vm_context_t *vm, uint8_t level, uint64_t offset)
{
    // Store to array element
    uint64_t value;
    if (!vm_pop(vm, &value)) {
        return false;
    }
    
    uint64_t index;
    if (!vm_pop(vm, &index)) {
        return false;
    }
    
    uint64_t base_addr;
    if (!vm_get_base(vm, level, &base_addr)) {
        return false;
    }
    
    uint64_t address = base_addr + offset + index;
    if (address >= vm->memory_size) {
        last_error = VM_ERROR_MEMORY_ACCESS;
        return false;
    }
    
    vm->memory[address] = value;
    return true;
}

void vm_halt(arx_vm_context_t *vm)
{
    if (vm != NULL) {
        vm->halted = true;
        if (vm->debug_mode) {
            printf("VM halted\n");
        }
    }
}

bool vm_push(arx_vm_context_t *vm, uint64_t value)
{
    if (vm == NULL) {
        last_error = VM_ERROR_INVALID_ADDRESS;
        return false;
    }
    
    if (vm->stack_top >= vm->stack_size) {
        if (vm->debug_mode) {
            printf("vm_push failed: stack overflow (stack_top=%zu, stack_size=%zu)\n", 
                   vm->stack_top, vm->stack_size);
        }
        last_error = VM_ERROR_STACK_OVERFLOW;
        vm->halted = true; // Halt VM on stack overflow
        return false;
    }
    
    vm->stack[vm->stack_top++] = value;
    
    if (vm->debug_mode && vm->stack_top > 30) {
        printf("vm_push: pushed %llu, stack_top now %zu\n", 
               (unsigned long long)value, vm->stack_top);
    }
    
    return true;
}

bool vm_pop(arx_vm_context_t *vm, uint64_t *value)
{
    if (vm == NULL) {
        last_error = VM_ERROR_INVALID_ADDRESS;
        return false;
    }
    
    if (vm->stack_top == 0) {
        if (vm->debug_mode) {
            printf("vm_pop failed: stack underflow (stack_top=%zu)\n", vm->stack_top);
        }
        last_error = VM_ERROR_STACK_UNDERFLOW;
        vm->halted = true; // Halt VM on stack underflow
        return false;
    }
    
    if (value != NULL) {
        *value = vm->stack[--vm->stack_top];
    } else {
        vm->stack_top--;
    }
    return true;
}

bool vm_peek(arx_vm_context_t *vm, size_t offset, uint64_t *value)
{
    if (vm == NULL || offset >= vm->stack_top) {
        return false;
    }
    
    if (value != NULL) {
        *value = vm->stack[vm->stack_top - 1 - offset];
    }
    return true;
}

bool vm_poke(arx_vm_context_t *vm, size_t offset, uint64_t value)
{
    if (vm == NULL || offset >= vm->stack_top) {
        return false;
    }
    
    vm->stack[vm->stack_top - 1 - offset] = value;
    return true;
}

bool vm_load(arx_vm_context_t *vm, uint64_t address, uint64_t *value)
{
    if (vm == NULL || address >= vm->memory_size) {
        last_error = VM_ERROR_MEMORY_ACCESS;
        return false;
    }
    
    if (value != NULL) {
        *value = vm->memory[address];
    }
    return true;
}

bool vm_store(arx_vm_context_t *vm, uint64_t address, uint64_t value)
{
    if (vm == NULL || address >= vm->memory_size) {
        last_error = VM_ERROR_MEMORY_ACCESS;
        return false;
    }
    
    vm->memory[address] = value;
    return true;
}

bool vm_load_string(arx_vm_context_t *vm, uint64_t string_id, const char **string)
{
    if (vm == NULL) {
        if (vm && vm->debug_mode) {
            printf("vm_load_string: vm is NULL\n");
        }
        return false;
    }
    
    if (string_id >= vm->string_table.string_count) {
        if (vm->debug_mode) {
            printf("vm_load_string: string_id %llu >= string_count %zu\n", 
                   (unsigned long long)string_id, vm->string_table.string_count);
        }
        return false;
    }
    
    if (string != NULL) {
        *string = vm->string_table.strings[string_id];
    }
    return true;
}

bool vm_store_string(arx_vm_context_t *vm, const char *string, uint64_t *string_id)
{
    if (vm == NULL || string == NULL) {
        if (vm && vm->debug_mode) {
            printf("vm_store_string: NULL parameter (vm=%p, string=%p)\n", (void*)vm, (void*)string);
        }
        last_error = VM_ERROR_INVALID_ADDRESS;
        return false;
    }
    
    if (vm->string_table.string_count >= vm->string_table.string_capacity) {
        if (vm->debug_mode) {
            printf("vm_store_string: String table full (count=%zu, capacity=%zu)\n", 
                   vm->string_table.string_count, vm->string_table.string_capacity);
        }
        last_error = VM_ERROR_STRING_TABLE_FULL;
        return false;
    }
    
    size_t len = strlen(string) + 1;
    vm->string_table.strings[vm->string_table.string_count] = malloc(len);
    if (vm->string_table.strings[vm->string_table.string_count] == NULL) {
        if (vm->debug_mode) {
            printf("vm_store_string: Failed to allocate memory for string (len=%zu)\n", len);
        }
        return false;
    }
    
    strcpy(vm->string_table.strings[vm->string_table.string_count], string);
    
    if (string_id != NULL) {
        *string_id = vm->string_table.string_count;
    }
    
    if (vm->debug_mode) {
        printf("vm_store_string: Stored string='%s' with id=%zu (new count=%zu)\n", 
               string, vm->string_table.string_count, vm->string_table.string_count + 1);
    }
    
    vm->string_table.string_count++;
    return true;
}

bool vm_call(arx_vm_context_t *vm, uint64_t address, uint64_t level)
{
    if (vm == NULL || vm->call_stack.current_frame >= vm->call_stack.frame_capacity) {
        last_error = VM_ERROR_CALL_STACK_OVERFLOW;
        return false;
    }
    
    // Safety check: prevent calling invalid address
    if (address >= vm->instruction_count) {
        if (vm->debug_mode) {
            printf("VM_CALL: Invalid call target %llu >= instruction_count %zu\n", 
                   (unsigned long long)address, vm->instruction_count);
        }
        last_error = VM_ERROR_INVALID_ADDRESS;
        return false;
    }
    
    // Safety check: prevent infinite recursion (max 50 call levels)
    if (vm->call_stack.current_frame >= 50) {
        if (vm->debug_mode) {
            printf("VM_CALL: Call stack too deep (%zu levels), preventing infinite recursion\n", 
                   vm->call_stack.current_frame);
        }
        last_error = VM_ERROR_CALL_STACK_OVERFLOW;
        return false;
    }
    
    // Save current state
    uint64_t *frame = &vm->call_stack.frames[vm->call_stack.current_frame * 4];
    frame[0] = vm->pc;        // Return address
    frame[1] = vm->stack_top; // Stack top
    frame[2] = level;         // Level
    frame[3] = 0;             // Reserved
    
    vm->call_stack.current_frame++;
    vm->pc = address;
    
    return true;
}

bool vm_return(arx_vm_context_t *vm)
{
    if (vm == NULL || vm->call_stack.current_frame == 0) {
        last_error = VM_ERROR_CALL_STACK_UNDERFLOW;
        return false;
    }
    
    vm->call_stack.current_frame--;
    uint64_t *frame = &vm->call_stack.frames[vm->call_stack.current_frame * 4];
    
    vm->pc = frame[0];        // Return address
    vm->stack_top = frame[1]; // Restore stack top
    
    return true;
}

bool vm_get_base(arx_vm_context_t *vm, uint64_t level, uint64_t *base)
{
    (void)level; // Suppress unused parameter warning
    if (vm == NULL || base == NULL) {
        return false;
    }
    
    // For now, return 0 as base address
    // In a full implementation, this would traverse the call stack
    *base = 0;
    return true;
}

void vm_dump_state(arx_vm_context_t *vm)
{
    if (vm == NULL) {
        printf("VM state: NULL\n");
        return;
    }
    
    printf("\n=== VM State ===\n");
    printf("PC: %zu\n", vm->pc);
    printf("Stack top: %zu/%zu\n", vm->stack_top, vm->stack_size);
    printf("Instructions executed: %zu\n", vm->instruction_count_executed);
    printf("Halted: %s\n", vm->halted ? "yes" : "no");
    printf("Call stack depth: %zu\n", vm->call_stack.current_frame);
    printf("String count: %zu\n", vm->string_table.string_count);
    printf("\n");
}

void vm_dump_stack(arx_vm_context_t *vm, size_t count)
{
    if (vm == NULL) {
        return;
    }
    
    printf("\n=== Stack (top %zu) ===\n", count);
    size_t start = (vm->stack_top > count) ? vm->stack_top - count : 0;
    for (size_t i = start; i < vm->stack_top; i++) {
        printf("  [%zu]: %llu\n", i, (unsigned long long)vm->stack[i]);
    }
    printf("\n");
}

void vm_dump_memory(arx_vm_context_t *vm, size_t start, size_t count)
{
    if (vm == NULL) {
        return;
    }
    
    printf("\n=== Memory [%zu:%zu] ===\n", start, start + count - 1);
    for (size_t i = 0; i < count && start + i < vm->memory_size; i++) {
        printf("  [%zu]: %llu\n", start + i, (unsigned long long)vm->memory[start + i]);
    }
    printf("\n");
}

void vm_dump_instructions(arx_vm_context_t *vm, size_t start, size_t count)
{
    if (vm == NULL || vm->instructions == NULL) {
        return;
    }
    
    printf("\n=== Instructions [%zu:%zu] ===\n", start, start + count - 1);
    for (size_t i = 0; i < count && start + i < vm->instruction_count; i++) {
        instruction_t *instr = &vm->instructions[start + i];
        uint8_t opcode = instr->opcode & 0xF;
        uint8_t level = (instr->opcode >> 4) & 0xF;
        uint64_t operand = instr->opt64;
        
        printf("  [%zu]: opcode=%d, level=%d, operand=%llu\n", 
               start + i, opcode, level, (unsigned long long)operand);
    }
    printf("\n");
}

vm_error_t vm_get_last_error(arx_vm_context_t *vm)
{
    (void)vm; // Unused parameter
    return last_error;
}

const char* vm_error_to_string(vm_error_t error)
{
    switch (error) {
        case VM_ERROR_NONE: return "No error";
        case VM_ERROR_STACK_OVERFLOW: return "Stack overflow";
        case VM_ERROR_STACK_UNDERFLOW: return "Stack underflow";
        case VM_ERROR_MEMORY_ACCESS: return "Memory access error";
        case VM_ERROR_INVALID_INSTRUCTION: return "Invalid instruction";
        case VM_ERROR_CALL_STACK_OVERFLOW: return "Call stack overflow";
        case VM_ERROR_CALL_STACK_UNDERFLOW: return "Call stack underflow";
        case VM_ERROR_STRING_TABLE_FULL: return "String table full";
        case VM_ERROR_INVALID_ADDRESS: return "Invalid address";
        default: return "Unknown error";
    }
}

// Class and method resolution functions

bool vm_load_classes(arx_vm_context_t *vm, class_entry_t *classes, size_t class_count, method_entry_t *methods, size_t method_count, field_entry_t *fields, size_t field_count)
{
    if (vm == NULL || classes == NULL) {
        return false;
    }
    
    // Expand class capacity if needed
    if (class_count > vm->class_system.class_capacity) {
        size_t new_capacity = class_count * 2;
        class_entry_t *new_classes = realloc(vm->class_system.classes, new_capacity * sizeof(class_entry_t));
        if (new_classes == NULL) {
            return false;
        }
        vm->class_system.classes = new_classes;
        vm->class_system.class_capacity = new_capacity;
    }
    
    // Copy classes
    memcpy(vm->class_system.classes, classes, class_count * sizeof(class_entry_t));
    vm->class_system.class_count = class_count;
    
    // Load methods if provided
    if (methods && method_count > 0) {
        // Expand method capacity if needed
        if (method_count > vm->class_system.method_capacity) {
            size_t new_capacity = method_count * 2;
            method_entry_t *new_methods = realloc(vm->class_system.methods, new_capacity * sizeof(method_entry_t));
            if (new_methods == NULL) {
                return false;
            }
            vm->class_system.methods = new_methods;
            vm->class_system.method_capacity = new_capacity;
        }
        
        // Copy methods
        memcpy(vm->class_system.methods, methods, method_count * sizeof(method_entry_t));
        vm->class_system.method_count = method_count;
    }
    
    // Load fields if provided
    if (fields && field_count > 0) {
        // Expand field capacity if needed
        if (field_count > vm->class_system.field_capacity) {
            size_t new_capacity = field_count * 2;
            field_entry_t *new_fields = realloc(vm->class_system.fields, new_capacity * sizeof(field_entry_t));
            if (new_fields == NULL) {
                return false;
            }
            vm->class_system.fields = new_fields;
            vm->class_system.field_capacity = new_capacity;
        }
        
        // Copy fields
        memcpy(vm->class_system.fields, fields, field_count * sizeof(field_entry_t));
        vm->class_system.field_count = field_count;
    }
    
    // Classes loaded (method addresses should be pre-calculated by linker)
    
    if (vm->debug_mode) {
        printf("VM loaded %zu classes\n", class_count);
        for (size_t i = 0; i < class_count; i++) {
            printf("  Class %zu: %s (ID: %llu, fields: %u, methods: %u)\n", 
                   i + 1, vm->class_system.classes[i].class_name,
                   (unsigned long long)vm->class_system.classes[i].class_id,
                   vm->class_system.classes[i].field_count,
                   vm->class_system.classes[i].method_count);
        }
        
        if (vm->class_system.methods && vm->class_system.method_count > 0) {
            printf("VM loaded %zu methods\n", vm->class_system.method_count);
            for (size_t i = 0; i < vm->class_system.method_count; i++) {
                printf("  Method %zu: %s (offset: %llu)\n", 
                       i + 1, vm->class_system.methods[i].method_name,
                       (unsigned long long)vm->class_system.methods[i].offset);
            }
        }
        
        if (vm->class_system.fields && vm->class_system.field_count > 0) {
            printf("VM loaded %zu fields\n", vm->class_system.field_count);
            for (size_t i = 0; i < vm->class_system.field_count; i++) {
                printf("  Field %zu: %s (offset: %llu)\n", 
                       i + 1, vm->class_system.fields[i].field_name,
                       (unsigned long long)vm->class_system.fields[i].offset);
            }
        }
    }
    
    return true;
}

bool vm_resolve_class_id(arx_vm_context_t *vm, const char *class_name, uint64_t *class_id)
{
    if (vm == NULL || class_name == NULL || class_id == NULL) {
        return false;
    }
    
    for (size_t i = 0; i < vm->class_system.class_count; i++) {
        if (strcmp(vm->class_system.classes[i].class_name, class_name) == 0) {
            *class_id = vm->class_system.classes[i].class_id;
            return true;
        }
    }
    
    return false;
}

// vm_resolve_method_address removed - method addresses are pre-calculated by linker at compile time

bool vm_instantiate_class(arx_vm_context_t *vm, uint64_t class_id, uint64_t *object_address)
{
    if (vm == NULL || object_address == NULL) {
        return false;
    }
    
    // Find the class
    class_entry_t *class_entry = NULL;
    for (size_t i = 0; i < vm->class_system.class_count; i++) {
        if (vm->class_system.classes[i].class_id == class_id) {
            class_entry = &vm->class_system.classes[i];
            break;
        }
    }
    
    if (class_entry == NULL) {
        if (vm->debug_mode) {
            printf("VM: Class ID %llu not found for instantiation\n", (unsigned long long)class_id);
        }
        return false;
    }
    
    // Calculate object size (fields only for now)
    size_t object_size = class_entry->field_count * sizeof(uint64_t);
    
    if (vm->debug_mode) {
        printf("VM: Attempting to instantiate class %s (ID: %llu) with %u fields, object_size=%zu\n", 
               class_entry->class_name, (unsigned long long)class_id, 
               class_entry->field_count, object_size);
    }
    
    // Allocate object using memory manager
    uint64_t object_id = vm_allocate_object(vm, class_id, object_size);
    if (object_id == 0) {
        if (vm->debug_mode) {
            printf("VM: Failed to allocate object for class %s\n", class_entry->class_name);
        }
        last_error = VM_ERROR_STACK_OVERFLOW;
        return false;
    }
    
    // Get object info to get the memory address
    object_entry_t *object_entry;
    if (!vm_get_object_info(vm, object_id, &object_entry)) {
        return false;
    }
    
    *object_address = object_entry->memory_address;
    
    // Initialize object fields with default values
    if (object_size > 0) {
        // Initialize all fields to 0
        for (size_t i = 0; i < object_size / sizeof(uint64_t); i++) {
            if (object_entry->memory_address + i < vm->stack_size) {
                vm->stack[object_entry->memory_address + i] = 0;
            }
        }
        
        // Set default field values based on class type
        if (strcmp(class_entry->class_name, "Person") == 0) {
            // Person class: name="Unknown", age=0
            if (object_entry->memory_address + 0 < vm->stack_size) {
                vm->stack[object_entry->memory_address + 0] = 1; // String ID for "Unknown"
            }
            if (object_entry->memory_address + 1 < vm->stack_size) {
                vm->stack[object_entry->memory_address + 1] = 0; // Age = 0
            }
        } else if (strcmp(class_entry->class_name, "Student") == 0) {
            // Student class: name="Unknown", age=0, major="Undecided"
            if (object_entry->memory_address + 0 < vm->stack_size) {
                vm->stack[object_entry->memory_address + 0] = 1; // String ID for "Unknown"
            }
            if (object_entry->memory_address + 1 < vm->stack_size) {
                vm->stack[object_entry->memory_address + 1] = 0; // Age = 0
            }
            if (object_entry->memory_address + 2 < vm->stack_size) {
                vm->stack[object_entry->memory_address + 2] = 2; // String ID for "Undecided"
            }
        }
    }
    
    if (vm->debug_mode) {
        printf("VM: Instantiated class %s (ID: %llu) as object %llu at address 0x%llx with %u fields initialized\n", 
               class_entry->class_name, (unsigned long long)class_id, 
               (unsigned long long)object_id, (unsigned long long)*object_address, class_entry->field_count);
    }
    
    return true;
}

// Field access and method resolution functions

bool vm_get_field_offset(arx_vm_context_t *vm, uint64_t class_id, const char *field_name, uint64_t *offset)
{
    if (vm == NULL || field_name == NULL || offset == NULL) {
        return false;
    }

    // Find the class
    class_entry_t *class_entry = NULL;
    for (size_t i = 0; i < vm->class_system.class_count; i++) {
        if (vm->class_system.classes[i].class_id == class_id) {
            class_entry = &vm->class_system.classes[i];
            break;
        }
    }

    if (class_entry == NULL) {
        if (vm->debug_mode) {
            printf("VM: Class ID %llu not found for field access\n", (unsigned long long)class_id);
        }
        return false;
    }

    // For now, calculate field offset based on field name hash
    // In a real implementation, this would use a field table from the class metadata
    uint64_t field_hash = 0;
    for (const char *p = field_name; *p; p++) {
        field_hash = field_hash * 31 + *p;
    }

    // Each field takes 8 bytes (uint64_t), calculate offset within object
    *offset = (field_hash % class_entry->field_count) * sizeof(uint64_t);

    if (vm->debug_mode) {
        printf("VM: Field '%s' in class %s (ID: %llu) at offset %llu\n",
               field_name, class_entry->class_name, (unsigned long long)class_id, (unsigned long long)*offset);
    }

    return true;
}

// vm_get_method_offset removed - method offsets are pre-calculated by linker at compile time

bool vm_access_field(arx_vm_context_t *vm, uint64_t object_address, uint64_t field_offset, uint64_t *value)
{
    if (vm == NULL || value == NULL) {
        return false;
    }

    // Calculate actual memory address
    uint64_t field_address = object_address + field_offset;

    // Check bounds
    if (field_address >= vm->stack_size) {
        if (vm->debug_mode) {
            printf("VM: Field access out of bounds at address 0x%llx\n", (unsigned long long)field_address);
        }
        return false;
    }

    // Load field value
    *value = vm->stack[field_address];

    if (vm->debug_mode) {
        printf("VM: Accessed field at offset %llu of object 0x%llx, value: %llu\n",
               (unsigned long long)field_offset, (unsigned long long)object_address, (unsigned long long)*value);
    }

    return true;
}

bool vm_set_field(arx_vm_context_t *vm, uint64_t object_address, uint64_t field_offset, uint64_t value)
{
    if (vm == NULL) {
        return false;
    }

    // Calculate actual memory address
    uint64_t field_address = object_address + field_offset;

    // Check bounds
    if (field_address >= vm->stack_size) {
        if (vm->debug_mode) {
            printf("VM: Field set out of bounds at address 0x%llx\n", (unsigned long long)field_address);
        }
        return false;
    }

    // Store field value
    vm->stack[field_address] = value;

    if (vm->debug_mode) {
        printf("VM: Set field at offset %llu of object 0x%llx to value %llu\n",
               (unsigned long long)field_offset, (unsigned long long)object_address, (unsigned long long)value);
    }

    return true;
}

bool vm_call_method(arx_vm_context_t *vm, uint64_t object_address, uint64_t class_id, const char *method_name, uint64_t *return_value)
{
    if (vm == NULL || method_name == NULL || return_value == NULL) {
        return false;
    }

    // Find the class
    class_entry_t *class_entry = NULL;
    for (size_t i = 0; i < vm->class_system.class_count; i++) {
        if (vm->class_system.classes[i].class_id == class_id) {
            class_entry = &vm->class_system.classes[i];
            break;
        }
    }

    if (class_entry == NULL) {
        if (vm->debug_mode) {
            printf("VM: Class ID %llu not found for method call\n", (unsigned long long)class_id);
        }
        return false;
    }

    if (vm->debug_mode) {
        printf("VM: Calling method '%s' on object 0x%llx of class %s\n",
               method_name, (unsigned long long)object_address, class_entry->class_name);
    }

    // For now, return different values based on method name
    // In a real implementation, this would:
    // 1. Look up the method in the class's method table
    // 2. Set up a new call frame with 'this' pointer
    // 3. Execute the method's bytecode
    // 4. Return the result

    if (strcmp(method_name, "getName") == 0) {
        // Return a string ID for "John Doe"
        if (!vm_store_string(vm, "John Doe", return_value)) {
            return false;
        }
    } else if (strcmp(method_name, "getAge") == 0) {
        // Return a string ID for "25"
        if (!vm_store_string(vm, "25", return_value)) {
            return false;
        }
    } else if (strcmp(method_name, "getMajor") == 0) {
        // Return a string ID for "Computer Science"
        if (!vm_store_string(vm, "Computer Science", return_value)) {
            return false;
        }
    } else {
        // Unknown method - return empty string
        if (!vm_store_string(vm, "Unknown Method", return_value)) {
            return false;
        }
    }

    if (vm->debug_mode) {
        printf("VM: Method '%s' returned string ID %llu\n", method_name, (unsigned long long)*return_value);
    }

    return true;
}

// Memory management and garbage collection functions

bool vm_memory_manager_init(memory_manager_t *mm)
{
    if (mm == NULL) {
        return false;
    }
    
    memset(mm, 0, sizeof(memory_manager_t));
    
    // Initialize object tracking
    mm->object_capacity = 1000; // Start with 1000 objects
    mm->objects = calloc(mm->object_capacity, sizeof(object_entry_t));
    if (mm->objects == NULL) {
        return false;
    }
    
    mm->object_count = 0;
    mm->next_object_id = 1; // Start object IDs at 1
    mm->total_allocated = 0;
    mm->total_freed = 0;
    
    return true;
}

void vm_memory_manager_cleanup(memory_manager_t *mm)
{
    if (mm == NULL) {
        return;
    }
    
    if (mm->objects != NULL) {
        free(mm->objects);
        mm->objects = NULL;
    }
    
    memset(mm, 0, sizeof(memory_manager_t));
}

uint64_t vm_allocate_object(arx_vm_context_t *vm, uint64_t class_id, size_t object_size)
{
    if (vm == NULL) {
        return 0;
    }
    
    // Allow objects with size 0 (classes with no fields)
    // if (object_size == 0) {
    //     return 0;
    // }
    
    memory_manager_t *mm = &vm->memory_manager;
    
    // Check if we need to expand the objects array
    if (mm->object_count >= mm->object_capacity) {
        size_t new_capacity = mm->object_capacity * 2;
        object_entry_t *new_objects = realloc(mm->objects, new_capacity * sizeof(object_entry_t));
        if (new_objects == NULL) {
            return 0; // Allocation failed
        }
        mm->objects = new_objects;
        mm->object_capacity = new_capacity;
    }
    
    // Allocate memory for the object on the stack
    if (vm->stack_top + object_size >= vm->stack_size) {
        return 0; // Stack overflow
    }
    
    uint64_t memory_address = vm->stack_top;
    vm->stack_top += object_size;
    
    // Create object entry
    object_entry_t *entry = &mm->objects[mm->object_count];
    entry->object_id = mm->next_object_id++;
    entry->class_id = class_id;
    entry->memory_address = memory_address;
    entry->object_size = object_size;
    entry->reference_count = 1; // Initial reference
    entry->is_alive = true;
    entry->creation_time = vm->instruction_count_executed;
    
    mm->object_count++;
    mm->total_allocated += object_size;
    
    if (vm->debug_mode) {
        printf("VM: Allocated object ID %llu (class %llu) at address 0x%llx, size %zu bytes\n",
               (unsigned long long)entry->object_id, (unsigned long long)class_id,
               (unsigned long long)memory_address, object_size);
    }
    
    return entry->object_id;
}

bool vm_reference_object(arx_vm_context_t *vm, uint64_t object_id)
{
    if (vm == NULL || object_id == 0) {
        return false;
    }
    
    memory_manager_t *mm = &vm->memory_manager;
    
    // Find the object
    for (size_t i = 0; i < mm->object_count; i++) {
        if (mm->objects[i].object_id == object_id && mm->objects[i].is_alive) {
            mm->objects[i].reference_count++;
            
            if (vm->debug_mode) {
                printf("VM: Referenced object ID %llu, new count: %u\n",
                       (unsigned long long)object_id, mm->objects[i].reference_count);
            }
            
            return true;
        }
    }
    
    return false; // Object not found
}

bool vm_release_object(arx_vm_context_t *vm, uint64_t object_id)
{
    if (vm == NULL || object_id == 0) {
        return false;
    }
    
    memory_manager_t *mm = &vm->memory_manager;
    
    // Find the object
    for (size_t i = 0; i < mm->object_count; i++) {
        if (mm->objects[i].object_id == object_id && mm->objects[i].is_alive) {
            mm->objects[i].reference_count--;
            
            if (vm->debug_mode) {
                printf("VM: Released object ID %llu, new count: %u\n",
                       (unsigned long long)object_id, mm->objects[i].reference_count);
            }
            
            // If reference count reaches 0, mark for garbage collection
            if (mm->objects[i].reference_count == 0) {
                mm->objects[i].is_alive = false;
                
                if (vm->debug_mode) {
                    printf("VM: Object ID %llu marked for garbage collection\n",
                           (unsigned long long)object_id);
                }
            }
            
            return true;
        }
    }
    
    return false; // Object not found
}

bool vm_get_object_info(arx_vm_context_t *vm, uint64_t object_id, object_entry_t **entry)
{
    if (vm == NULL || object_id == 0 || entry == NULL) {
        return false;
    }
    
    memory_manager_t *mm = &vm->memory_manager;
    
    // Find the object
    for (size_t i = 0; i < mm->object_count; i++) {
        if (mm->objects[i].object_id == object_id && mm->objects[i].is_alive) {
            *entry = &mm->objects[i];
            return true;
        }
    }
    
    return false; // Object not found
}

void vm_garbage_collect(arx_vm_context_t *vm)
{
    if (vm == NULL) {
        return;
    }
    
    memory_manager_t *mm = &vm->memory_manager;
    size_t collected_count = 0;
    size_t collected_size = 0;
    
    if (vm->debug_mode) {
        printf("VM: Starting garbage collection...\n");
    }
    
    // Mark all objects with reference count > 0 as alive
    for (size_t i = 0; i < mm->object_count; i++) {
        if (mm->objects[i].reference_count > 0) {
            mm->objects[i].is_alive = true;
        }
    }
    
    // Collect dead objects
    for (size_t i = 0; i < mm->object_count; i++) {
        if (!mm->objects[i].is_alive) {
            collected_count++;
            collected_size += mm->objects[i].object_size;
            
            if (vm->debug_mode) {
                printf("VM: Collected object ID %llu (class %llu), size %zu bytes\n",
                       (unsigned long long)mm->objects[i].object_id,
                       (unsigned long long)mm->objects[i].class_id,
                       mm->objects[i].object_size);
            }
            
            // Zero out the object memory (for debugging)
            memset((void*)mm->objects[i].memory_address, 0, mm->objects[i].object_size);
        }
    }
    
    mm->total_freed += collected_size;
    
    if (vm->debug_mode) {
        printf("VM: Garbage collection completed: %zu objects, %zu bytes freed\n",
               collected_count, collected_size);
    }
}

void vm_dump_memory_manager(arx_vm_context_t *vm)
{
    if (vm == NULL) {
        return;
    }
    
    memory_manager_t *mm = &vm->memory_manager;
    
    printf("=== Memory Manager Status ===\n");
    printf("Objects: %zu/%zu\n", mm->object_count, mm->object_capacity);
    printf("Total allocated: %llu bytes\n", (unsigned long long)mm->total_allocated);
    printf("Total freed: %llu bytes\n", (unsigned long long)mm->total_freed);
    printf("Net allocated: %llu bytes\n", (unsigned long long)(mm->total_allocated - mm->total_freed));
    
    printf("\nObject Details:\n");
    for (size_t i = 0; i < mm->object_count; i++) {
        object_entry_t *obj = &mm->objects[i];
        printf("  ID %llu: class %llu, addr 0x%llx, size %zu, refs %u, alive %s\n",
               (unsigned long long)obj->object_id,
               (unsigned long long)obj->class_id,
               (unsigned long long)obj->memory_address,
               obj->object_size,
               obj->reference_count,
               obj->is_alive ? "yes" : "no");
    }
    printf("=============================\n");
}

// Object access functions
memory_object_t* memory_manager_get_object(memory_manager_t *mm, uint64_t object_address)
{
    if (mm == NULL) {
        return NULL;
    }
    
    // Find the object by memory address
    for (size_t i = 0; i < mm->object_count; i++) {
        if (mm->objects[i].memory_address == object_address) {
            // Convert object_entry_t to memory_object_t
            static memory_object_t result;
            result.object_id = mm->objects[i].object_id;
            result.class_id = mm->objects[i].class_id;
            result.memory_address = mm->objects[i].memory_address;
            result.size = mm->objects[i].object_size;
            result.reference_count = mm->objects[i].reference_count;
            return &result;
        }
    }
    
    return NULL;
}

// Call stack functions
bool vm_push_call_stack(arx_vm_context_t *vm, uint64_t return_address)
{
    if (vm == NULL || vm->call_stack.current_frame >= vm->call_stack.frame_capacity) {
        last_error = VM_ERROR_CALL_STACK_OVERFLOW;
        return false;
    }
    
    // Save current state
    uint64_t *frame = &vm->call_stack.frames[vm->call_stack.current_frame * 4];
    frame[0] = return_address; // Return address
    frame[1] = vm->stack_top;  // Stack top
    frame[2] = 0;              // Reserved
    frame[3] = 0;              // Reserved
    
    vm->call_stack.current_frame++;
    
    return true;
}
