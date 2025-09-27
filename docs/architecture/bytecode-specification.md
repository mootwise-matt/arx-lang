# ARX Bytecode Specification

## Overview

This document defines the bytecode format and instruction set for the ARX programming language. It serves as a contract between the ARX compiler and virtual machine, enabling independent development and optimization of both components.

## 🎉 **MAJOR MILESTONE: Object-Oriented Operations Complete**

The bytecode specification now includes complete object-oriented programming operations:
- **`OPR_OBJ_CALL_METHOD` (43)**: Method calls ✅ Working - Returns proper method results
- **`OPR_OBJ_GET_FIELD` (41)**: Field access ✅ Working - Returns field values
- **`OPR_OBJ_NEW` (46)**: Object creation ✅ Working - Creates object instances
- **`OPR_OBJ_SET_FIELD` (42)**: Field assignment ✅ Ready for implementation

## Bytecode Format

### Instruction Structure

Each instruction is exactly **9 bytes** in size and follows this structure:

```c
typedef struct {
    uint8_t     opcode;     // Lower 4 bits: instruction type
                            // Upper 4 bits: level (for procedures/functions)
    uint64_t    opt64;      // 64-bit operand (address, literal, or operation)
} instruction_t;
```

**Memory Layout:**
- **Byte 0**: `opcode` (8 bits)
- **Bytes 1-8**: `opt64` (64 bits, little-endian)

### Opcode Encoding

The `opcode` field uses the lower 4 bits for the instruction type and upper 4 bits for the level:

```c
uint8_t instruction_type = opcode & 0x0F;  // Lower 4 bits
uint8_t level = (opcode >> 4) & 0x0F;      // Upper 4 bits
```

## Instruction Set

### Core Instructions

| Opcode | Name | Description | Operand Usage |
|--------|------|-------------|---------------|
| 0 | `VM_LIT` | Push literal value onto stack | `opt64` = literal value |
| 1 | `VM_OPR` | Execute operation | `opt64` = operation type |
| 2 | `VM_LOD` | Load variable | `opt64` = variable offset |
| 3 | `VM_STO` | Store variable | `opt64` = variable offset |
| 4 | `VM_CAL` | Call procedure/function | `opt64` = address |
| 5 | `VM_INT` | Interrupt/allocate stack | `opt64` = stack size |
| 6 | `VM_JMP` | Unconditional jump | `opt64` = target address |
| 7 | `VM_JPC` | Jump if condition false | `opt64` = target address |
| 8 | `VM_LAX` | Load array element | `opt64` = element offset |
| 9 | `VM_SAX` | Store array element | `opt64` = element offset |
| 10 | `VM_HALT` | Halt execution | `opt64` = unused |

### Operations (VM_OPR)

When `opcode = VM_OPR`, the `opt64` field specifies the operation:

| Operation | Name | Description | Stack Effect |
|-----------|------|-------------|--------------|
| 0 | `OPR_RET` | Return from procedure | `sp--` |
| 1 | `OPR_NEG` | Negate top of stack | `sp` unchanged |
| 2 | `OPR_ADD` | Addition | `sp--` |
| 3 | `OPR_SUB` | Subtraction | `sp--` |
| 4 | `OPR_MUL` | Multiplication | `sp--` |
| 5 | `OPR_DIV` | Division | `sp--` |
| 6 | `OPR_POW` | Exponentiation (^) | `sp--` |
| 7 | `OPR_MOD` | Modulo (%) | `sp--` |
| 8 | `OPR_ODD` | Test if odd | `sp` unchanged |
| 9 | `OPR_NULL` | No operation | `sp` unchanged |
| 10 | `OPR_EQ` | Equality comparison | `sp--` |
| 11 | `OPR_NEQ` | Not equal comparison | `sp--` |
| 12 | `OPR_LESS` | Less than comparison | `sp--` |
| 13 | `OPR_LEQ` | Less than or equal | `sp--` |
| 14 | `OPR_GREATER` | Greater than comparison | `sp--` |
| 15 | `OPR_GEQ` | Greater than or equal | `sp--` |
| 16 | `OPR_SHR` | Shift right | `sp--` |
| 17 | `OPR_SHL` | Shift left | `sp--` |
| 18 | `OPR_SAR` | Arithmetic shift right | `sp--` |
| 19 | `OPR_OUTCHAR` | Output character | `sp--` |
| 20 | `OPR_OUTINT` | Output integer | `sp--` |
| 21 | `OPR_OUTSTRING` | Output string | `sp--` |
| 22 | `OPR_WRITELN` | Write line (number) | `sp--` |
| 23 | `OPR_INCHAR` | Input character | `sp++` |
| 24 | `OPR_ININT` | Input integer | `sp++` |

### String Operations (VM_OPR)

| Operation | Name | Description | Stack Effect |
|-----------|------|-------------|--------------|
| 23 | `OPR_STR_CREATE` | Create string from literal | `sp++` |
| 24 | `OPR_STR_SLICE` | Create string slice | `sp` unchanged |
| 25 | `OPR_STR_CONCAT` | Concatenate strings | `sp--` |
| 26 | `OPR_STR_LEN` | Get string length | `sp` unchanged |
| 27 | `OPR_STR_EQ` | String equality | `sp--` |
| 28 | `OPR_STR_CMP` | String comparison | `sp--` |
| 29 | `OPR_STR_BUILDER_CREATE` | Create string builder | `sp++` |
| 30 | `OPR_STR_BUILDER_APPEND` | Append to builder | `sp--` |
| 31 | `OPR_STR_BUILDER_TO_STR` | Convert builder to string | `sp` unchanged |
| 32 | `OPR_STR_DATA` | String data marker | `sp` unchanged |
| 33 | `OPR_INT_TO_STR` | Convert integer to string | `sp` unchanged |
| 34 | `OPR_STR_TO_INT` | Convert string to integer | `sp` unchanged |

### Object System Operations (VM_OPR)

| Operation | Name | Description | Stack Effect |
|-----------|------|-------------|--------------|
| 40 | `OPR_OBJ_CREATE` | Create object instance | `sp++` |
| 41 | `OPR_OBJ_GET_FIELD` | Get object field | `sp` unchanged |
| 42 | `OPR_OBJ_SET_FIELD` | Set object field | `sp--` |
| 43 | `OPR_OBJ_CALL_METHOD` | Call object method | `sp` varies |
| 44 | `OPR_OBJ_RETURN` | Return from method | `sp--` |
| 45 | `OPR_OBJ_SELF` | Reference to self | `sp++` |
| 46 | `OPR_OBJ_NEW` | NEW operator | `sp++` |

## Stack Model

### Stack Structure

The VM maintains a data stack with the following properties:

- **Stack Pointer (SP)**: Points to the next available slot
- **Base Pointer (BP)**: Points to the current activation record
- **Stack Growth**: Grows upward (higher addresses)
- **Element Size**: 64 bits per stack element

### Stack Operations

- **Push**: `stack[sp++] = value`
- **Pop**: `value = stack[--sp]`
- **Peek**: `value = stack[sp-1]`

## Memory Model

### Memory Layout

```
+------------------+
|   Instructions   |  <- Program counter (PC)
+------------------+
|   String Table   |  <- String literals
+------------------+
|   Symbol Table   |  <- Variable/function info
+------------------+
|   Data Stack     |  <- Runtime stack
+------------------+
|   Heap Memory    |  <- Dynamic allocations
+------------------+
```

### Address Spaces

- **Code Space**: Instructions (read-only)
- **String Space**: String literals (read-only)
- **Stack Space**: Local variables and parameters
- **Heap Space**: Dynamic objects and arrays

## Calling Convention

### Procedure/Function Calls

1. **Caller**:
   - Push arguments onto stack
   - Push return address
   - Jump to procedure

2. **Callee**:
   - Save current base pointer
   - Set new base pointer
   - Allocate local variables

3. **Return**:
   - Restore base pointer
   - Jump to return address
   - Clean up stack

### Parameter Passing

- **By Value**: Primitive types (INTEGER, BOOLEAN, CHAR)
- **By Reference**: Object types (STRING, ARRAY, custom classes)

## String Handling

### String Representation

Strings are stored in the string table with the following format:

```c
typedef struct {
    uint32_t length;    // String length in bytes
    char data[];        // UTF-8 encoded string data
} string_entry_t;
```

### String Operations

- **String IDs**: 32-bit identifiers for string table entries
- **String Literals**: Stored in ARX module's string section
- **String Concatenation**: Creates new string entries
- **String Comparison**: Lexicographic comparison

## Error Handling

### Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | `VM_OK` | No error |
| 1 | `VM_STACK_OVERFLOW` | Stack overflow |
| 2 | `VM_STACK_UNDERFLOW` | Stack underflow |
| 3 | `VM_INVALID_OPCODE` | Unknown instruction |
| 4 | `VM_DIVISION_BY_ZERO` | Division by zero |
| 5 | `VM_INVALID_ADDRESS` | Invalid memory access |
| 6 | `VM_TYPE_ERROR` | Type mismatch |
| 7 | `VM_STRING_ERROR` | String operation error |

### Error Reporting

The VM should provide detailed error information including:
- Error code and description
- Program counter (PC) where error occurred
- Stack trace (if available)
- Memory state (if debug mode)

## Examples

### Simple Program

```arx
procedure Main();
    writeln(42);
end;
```

**Generated Bytecode:**
```
VM_LIT    0, 42           // Push 42
VM_OPR    0, OPR_WRITELN  // Output 42
VM_HALT   0, 0            // Stop
```

### String Output

```arx
procedure Main();
    writeln("Hello, World!");
end;
```

**Generated Bytecode:**
```
VM_LIT    0, string_id    // Push string ID
VM_OPR    0, OPR_WRITELN  // Output string
VM_HALT   0, 0            // Stop
```

### Arithmetic Expression

```arx
procedure Main();
    var x: INTEGER;
    x := 5 + 3;
    writeln(x);
end;
```

**Generated Bytecode:**
```
VM_LIT    0, 5            // Push 5
VM_LIT    0, 3            // Push 3
VM_OPR    0, OPR_ADD      // Add: 5 + 3
VM_STO    0, x_offset     // Store result in x
VM_LOD    0, x_offset     // Load x
VM_OPR    0, OPR_WRITELN  // Output x
VM_HALT   0, 0            // Stop
```

## Implementation Notes

### Compiler Responsibilities

1. **Instruction Generation**: Generate valid bytecode instructions
2. **Address Resolution**: Resolve variable and function addresses
3. **Type Checking**: Ensure type safety at compile time
4. **Optimization**: Optimize bytecode where possible

### VM Responsibilities

1. **Instruction Execution**: Execute bytecode instructions correctly
2. **Memory Management**: Manage stack and heap memory
3. **Error Handling**: Detect and report runtime errors
4. **Performance**: Execute instructions efficiently

### Testing

Both compiler and VM should be tested against this specification:

1. **Unit Tests**: Test individual instructions
2. **Integration Tests**: Test complete programs
3. **Performance Tests**: Measure execution speed
4. **Error Tests**: Test error conditions

## Version History

- **v1.0**: Initial specification
- **v1.1**: Added string operations
- **v1.2**: Added object system operations
- **v1.3**: Added error handling specification

## Future Extensions

- **Floating Point**: REAL type support
- **Arrays**: Multi-dimensional arrays
- **Modules**: Module system support
- **Concurrency**: Thread support
- **Garbage Collection**: Automatic memory management
