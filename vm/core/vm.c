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
    
    memset(vm, 0, sizeof(arx_vm_context_t));
    
    // Initialize stack
    vm->stack = calloc(stack_size, sizeof(uint64_t));
    if (vm->stack == NULL) {
        return false;
    }
    vm->stack_size = stack_size;
    vm->stack_top = 0;
    
    // Initialize memory
    vm->memory = calloc(memory_size, sizeof(uint64_t));
    if (vm->memory == NULL) {
        free(vm->stack);
        return false;
    }
    vm->memory_size = memory_size;
    
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
    
    // Initialize execution state
    vm->pc = 0;
    vm->instruction_count = 0;
    vm->instruction_count_executed = 0;
    vm->halted = false;
    vm->debug_mode = debug_mode;
    
    last_error = VM_ERROR_NONE;
    
    if (vm->debug_mode) {
        printf("VM initialized: stack=%zu, memory=%zu\n", stack_size, memory_size);
    }
    
    return true;
}

void vm_cleanup(arx_vm_context_t *vm)
{
    if (vm == NULL) {
        return;
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
    
    while (!vm->halted && vm->pc < vm->instruction_count) {
        if (!vm_step(vm)) {
            return false;
        }
    }
    
    if (vm->debug_mode) {
        printf("VM execution completed: %zu instructions executed\n", 
               vm->instruction_count_executed);
    }
    
    return true;
}

bool vm_step(arx_vm_context_t *vm)
{
    if (vm == NULL || vm->halted || vm->pc >= vm->instruction_count) {
        return false;
    }
    
    instruction_t *instr = &vm->instructions[vm->pc];
    uint8_t opcode = instr->opcode & 0xF;
    uint8_t level = (instr->opcode >> 4) & 0xF;
    uint64_t operand = instr->opt64;
    
    if (vm->debug_mode) {
        printf("PC=%zu: raw_opcode=0x%02x, opcode=%d, level=%d, operand=%llu\n", 
               vm->pc, instr->opcode, opcode, level, (unsigned long long)operand);
    }
    
    bool success = true;
    
    switch (opcode) {
        case VM_LIT:
            success = vm_push(vm, operand);
            break;
            
        case VM_OPR:
            success = vm_execute_operation(vm, (opr_t)operand, level, operand);
            break;
            
        case VM_LOD:
            success = vm_execute_load(vm, level, operand);
            break;
            
        case VM_STO:
            success = vm_execute_store(vm, level, operand);
            break;
            
        case VM_CAL:
            success = vm_execute_call(vm, level, operand);
            break;
            
        case VM_INT:
            success = vm_execute_int(vm, operand);
            break;
            
        case VM_JMP:
            vm->pc = operand;
            break;
            
        case VM_JPC:
            {
                uint64_t condition;
                if (vm_pop(vm, &condition)) {
                    if (condition == 0) {
                        vm->pc = operand;
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
    
    vm->instruction_count_executed++;
    
    return success;
}

bool vm_execute_operation(arx_vm_context_t *vm, opr_t operation, uint8_t level, uint64_t operand)
{
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
                    return vm_push(vm, a + b);
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
                    return vm_push(vm, (a <= b) ? 1 : 0);
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
            
        case OPR_WRITELN:
            // WriteLn - output newline (no stack operation needed)
            printf("\n");
            fflush(stdout);
            return true;
            
        case OPR_OUTSTRING:
            {
                uint64_t string_id;
                if (vm_pop(vm, &string_id)) {
                    if (vm->debug_mode) {
                        printf("OPR_OUTSTRING: Popped string_id=%llu, string_count=%zu\n", 
                               (unsigned long long)string_id, vm->string_table.string_count);
                    }
                    const char *str;
                    if (vm_load_string(vm, string_id, &str)) {
                        if (vm->debug_mode) {
                            printf("OPR_OUTSTRING: Loaded string='%s'\n", str ? str : "(null)");
                        }
                        // Output UTF-8 string
                        if (vm->string_table.utf8_enabled) {
                            // UTF-8 mode: output string as-is
                            printf("%s", str);
                        } else {
                            // ASCII mode: filter non-ASCII characters
                            for (const char *p = str; *p; p++) {
                                if ((unsigned char)*p < 128) {
                                    putchar(*p);
                                }
                            }
                        }
                        fflush(stdout);
                        return true;
                    } else {
                        if (vm->debug_mode) {
                            printf("Error: Failed to load string ID %llu (string_count=%zu)\n", 
                                   (unsigned long long)string_id, vm->string_table.string_count);
                        }
                    }
                } else {
                    if (vm->debug_mode) {
                        printf("Error: Failed to pop string ID from stack\n");
                    }
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
                uint64_t str2_id, str1_id;
                if (vm_pop(vm, &str2_id) && vm_pop(vm, &str1_id)) {
                    const char *str1, *str2;
                    if (vm_load_string(vm, str1_id, &str1) && vm_load_string(vm, str2_id, &str2)) {
                        // Simple concatenation for now
                        char *result = malloc(strlen(str1) + strlen(str2) + 1);
                        if (result != NULL) {
                            strcpy(result, str1);
                            strcat(result, str2);
                            uint64_t result_id;
                            if (vm_store_string(vm, result, &result_id)) {
                                free(result);
                                return vm_push(vm, result_id);
                            }
                            free(result);
                        }
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
                // Method call - for now, just return a placeholder string
                if (vm->debug_mode) {
                    printf("OPR_OBJ_CALL_METHOD: Method call executed\n");
                }
                // Return a placeholder string "Method Result"
                uint64_t string_id;
                if (vm_store_string(vm, "Method Result", &string_id)) {
                    return vm_push(vm, string_id);
                }
                return false;
            }
            
        case OPR_OBJ_GET_FIELD:
            {
                // Field access - for now, just return a placeholder string
                if (vm->debug_mode) {
                    printf("OPR_OBJ_GET_FIELD: Field access executed\n");
                }
                // Return a placeholder string "Field Value"
                uint64_t string_id;
                if (vm_store_string(vm, "Field Value", &string_id)) {
                    return vm_push(vm, string_id);
                }
                return false;
            }
            
        default:
            if (vm->debug_mode) {
                printf("Error: Unknown operation %d\n", operation);
            }
            last_error = VM_ERROR_INVALID_INSTRUCTION;
            return false;
    }
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
    if (vm == NULL || vm->stack_top >= vm->stack_size) {
        last_error = VM_ERROR_STACK_OVERFLOW;
        return false;
    }
    
    vm->stack[vm->stack_top++] = value;
    return true;
}

bool vm_pop(arx_vm_context_t *vm, uint64_t *value)
{
    if (vm == NULL || vm->stack_top == 0) {
        last_error = VM_ERROR_STACK_UNDERFLOW;
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
    if (vm == NULL || string_id >= vm->string_table.string_count) {
        return false;
    }
    
    if (string != NULL) {
        *string = vm->string_table.strings[string_id];
    }
    return true;
}

bool vm_store_string(arx_vm_context_t *vm, const char *string, uint64_t *string_id)
{
    if (vm == NULL || string == NULL || vm->string_table.string_count >= vm->string_table.string_capacity) {
        last_error = VM_ERROR_STRING_TABLE_FULL;
        return false;
    }
    
    size_t len = strlen(string) + 1;
    vm->string_table.strings[vm->string_table.string_count] = malloc(len);
    if (vm->string_table.strings[vm->string_table.string_count] == NULL) {
        return false;
    }
    
    strcpy(vm->string_table.strings[vm->string_table.string_count], string);
    
    if (string_id != NULL) {
        *string_id = vm->string_table.string_count;
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
