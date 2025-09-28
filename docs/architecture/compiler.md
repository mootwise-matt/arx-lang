# ARX Compiler Architecture

## Overview

The ARX compiler is a multi-pass compiler that translates ARX source code into bytecode for execution by the ARX Virtual Machine. It follows a traditional compiler architecture with distinct phases for lexical analysis, parsing, semantic analysis, and code generation.

## 🎉 **MAJOR MILESTONE: AST-Based Architecture Complete**

The compiler now features a complete Abstract Syntax Tree (AST) implementation that enables:
- **Dynamic Value Processing**: All values computed at runtime in the VM
- **No Hardcoded Values**: Compiler generates code based on actual parsed expressions
- **Complete Expression Support**: Full arithmetic, string operations, and variable management
- **C-Style Syntax**: Modern variable declarations and assignments
- **Object-Oriented Programming**: Complete support for method calls and field access

## 🛡️ **LABEL RESOLUTION AND INFINITE LOOP PROTECTION COMPLETE**

**Compiler Safety and Stability Achieved!** The ARX compiler now features robust label resolution and infinite loop prevention:
- **Multi-Context Label Merging**: Labels from separate class contexts are properly merged into main context
- **Two-Pass Compilation**: First pass generates bytecode with label placeholders, second pass resolves all labels
- **Linker Integration**: Linker skips already-resolved jump instructions to prevent overwriting
- **Jump Address Resolution**: All jump instructions contain actual instruction addresses, not label IDs
- **Cross-Context Label Management**: Labels are properly adjusted by class base offset during merging

## Compiler Phases

### 1. Lexical Analysis (Lexer)

**Purpose**: Convert source code into a stream of tokens

**Input**: ARX source code (`.arx` files)
**Output**: Token stream

**Key Components**:
- `lexer_context_t`: Lexer state and configuration
- `token_t`: Token type enumeration
- `lexer_next()`: Advance to next token
- `lexer_peek()`: Look ahead at next token

**Token Types**:
- **Keywords**: `module`, `class`, `procedure`, `function`, `var`, `if`, `while`, etc.
- **Identifiers**: Variable names, class names, method names
- **Literals**: Numbers, strings, booleans
- **Operators**: `+`, `-`, `*`, `/`, `=`, `==`, etc.
- **Delimiters**: `(`, `)`, `{`, `}`, `;`, `:`, etc.

### 2. Syntax Analysis (Parser)

**Purpose**: Build an Abstract Syntax Tree (AST) from tokens

**Input**: Token stream
**Output**: AST

**Key Components**:
- `parser_context_t`: Parser state and configuration
- `ast_node_t`: AST node structure
- `parse_module()`: Parse module declarations
- `parse_class()`: Parse class definitions
- `parse_method()`: Parse method definitions
- `parse_statement()`: Parse statements
- `parse_expression()`: Parse expressions

**AST Node Types**:
- `AST_MODULE`: Module declaration
- `AST_CLASS`: Class definition
- `AST_FIELD`: Field declaration
- `AST_METHOD`: Method definition
- `AST_PROCEDURE`: Procedure definition
- `AST_FUNCTION`: Function definition
- `AST_VAR_DECL`: Variable declaration
- `AST_ASSIGNMENT`: Assignment statement
- `AST_METHOD_CALL`: Method call
- `AST_FIELD_ACCESS`: Field access
- `AST_LITERAL`: Literal value
- `AST_IDENTIFIER`: Identifier reference

### 3. Semantic Analysis

**Purpose**: Perform type checking and semantic validation

**Input**: AST
**Output**: Annotated AST with type information

**Key Components**:
- `type_info_t`: Type information structure
- `symbol_table_t`: Symbol table for scoping
- `type_check()`: Type checking functions
- `symbol_lookup()`: Symbol resolution

**Type System**:
- **Primitive Types**: `INTEGER`, `BOOLEAN`, `CHAR`
- **Object Types**: `STRING`, `ARRAY`, custom classes
- **Type Checking**: Compile-time validation
- **Symbol Resolution**: Variable and function lookup

### 4. Code Generation

**Purpose**: Generate bytecode instructions from AST

**Input**: Annotated AST
**Output**: Bytecode instructions

**Key Components**:
- `codegen_context_t`: Code generator state
- `instruction_t`: Bytecode instruction structure
- `emit_instruction()`: Emit single instruction
- `emit_operation()`: Emit operation instruction
- `generate_method()`: Generate method code
- `generate_statement()`: Generate statement code
- `generate_method_call_ast()`: Generate method call code
- `generate_field_access_ast()`: Generate field access code

**Code Generation Strategy**:
- **Stack-based**: All operations use stack
- **Three-address**: Complex expressions broken into simple operations
- **Register allocation**: Virtual registers for temporaries
- **Optimization**: Basic optimizations (constant folding, dead code elimination)
- **Two-pass compilation**: First pass generates bytecode with label placeholders, second pass resolves all labels
- **Multi-context label merging**: Labels from separate class contexts are properly merged into main context

## Compiler Structure

### Directory Layout

```
compiler/
├── main.c                 # Main compiler entry point
├── lexer/
│   ├── lexer.h           # Lexer interface
│   └── lexer.c           # Lexer implementation
├── parser/
│   ├── parser.h          # Parser interface
│   └── parser.c          # Parser implementation
├── codegen/
│   ├── codegen.h         # Code generator interface
│   └── codegen.c         # Code generator implementation
├── types/
│   ├── types.h           # Type system interface
│   └── types.c           # Type system implementation
├── symbols/
│   ├── symbols.h         # Symbol table interface
│   └── symbols.c         # Symbol table implementation
├── arxmod/
│   ├── arxmod.h          # ARX module format
│   ├── arxmod_writer.c   # Module writer
│   └── arxmod_reader.c   # Module reader
└── common/
    └── opcodes.h         # Opcode definitions
```

### Key Data Structures

#### Lexer Context

```c
typedef struct {
    const char *source;        // Source code
    size_t source_len;         // Source length
    size_t position;           // Current position
    token_t token;             // Current token
    const char *tokstart;      // Token start
    size_t toklen;             // Token length
    int64_t linenum;           // Line number
    char string_content[256];  // String literal buffer
} lexer_context_t;
```

#### Parser Context

```c
typedef struct {
    lexer_context_t *lexer;        // Lexer context
    ast_node_t *root;              // Root of AST
    ast_node_t *current_node;      // Current node being built
    symbol_table_t symbol_table;   // Symbol table
    int error_count;               // Number of errors
    bool in_error_recovery;        // Error recovery mode
    char *current_string_literal;  // Current string literal
} parser_context_t;
```

#### Code Generator Context

```c
typedef struct {
    instruction_t *instructions;    // Generated instructions
    size_t instruction_count;      // Number of instructions
    size_t instruction_capacity;   // Capacity of instructions array
    size_t label_counter;          // Label counter for jumps
    bool debug_output;             // Debug output flag
} codegen_context_t;
```

## Compiler Interface

### Main Compiler Function

```c
bool compile_arx_file(const char *input_file, const char *output_file, bool debug_mode);
```

**Parameters**:
- `input_file`: Path to ARX source file
- `output_file`: Path to output ARX module file
- `debug_mode`: Enable debug output

**Returns**: `true` if compilation successful, `false` otherwise

### Compiler Options

```c
typedef struct {
    bool debug_mode;           // Enable debug output
    bool show_bytecode;        // Show generated bytecode
    bool show_symbols;         // Show symbol table
    bool optimize;             // Enable optimizations
    const char *input_file;    // Input file path
    const char *output_file;   // Output file path
} compiler_options_t;
```

## Error Handling

### Error Types

- **Lexical Errors**: Invalid characters, unterminated strings
- **Syntax Errors**: Invalid grammar, missing tokens
- **Semantic Errors**: Type mismatches, undefined symbols
- **Code Generation Errors**: Internal compiler errors

### Error Reporting

```c
void compiler_error(const char *message, int line, int column);
void compiler_warning(const char *message, int line, int column);
```

### Error Recovery

- **Lexical**: Skip invalid characters
- **Syntax**: Skip to next statement
- **Semantic**: Continue with warnings
- **Code Generation**: Abort compilation

## Optimization

### Optimization Levels

- **Level 0**: No optimization
- **Level 1**: Basic optimizations (constant folding)
- **Level 2**: Advanced optimizations (dead code elimination)
- **Level 3**: Aggressive optimizations (inlining)

### Optimization Techniques

- **Constant Folding**: Evaluate constant expressions at compile time
- **Dead Code Elimination**: Remove unreachable code
- **Common Subexpression Elimination**: Reuse computed values
- **Loop Optimization**: Unroll simple loops
- **Register Allocation**: Minimize stack operations

## Testing

### Unit Tests

- **Lexer Tests**: Token recognition
- **Parser Tests**: AST construction
- **Type Checker Tests**: Type validation
- **Code Generator Tests**: Bytecode generation

### Integration Tests

- **End-to-End Tests**: Complete compilation pipeline
- **Regression Tests**: Prevent regressions
- **Performance Tests**: Compilation speed

### Test Framework

```c
// Test framework interface
bool run_lexer_tests(void);
bool run_parser_tests(void);
bool run_type_checker_tests(void);
bool run_codegen_tests(void);
bool run_integration_tests(void);
```

## Dynamic values
- **No Hard coding of value** Hard coding of values is not allowed, all values are calculated at runtime in the VM, all values come from the AST
- **No limit on strings** Strings known at compile time can be indefinite in number and the Arxmod file format allows for this, and are stored in the strings section of the Arxmod file

### Compilation Speed

- **Target**: < 1 second for 1000 lines of code
- **Memory Usage**: < 100MB for large programs
- **Scalability**: Linear with program size

### Optimization

- **Incremental Compilation**: Only recompile changed files
- **Parallel Processing**: Parse multiple files concurrently
- **Caching**: Cache parsed ASTs and symbol tables

## Future Enhancements

### Planned Features

- **Incremental Compilation**: Only recompile changed code
- **Parallel Compilation**: Multi-threaded compilation
- **Advanced Optimizations**: More sophisticated optimizations
- **Debug Information**: Source-level debugging support
- **Cross-Compilation**: Target different architectures

### Extensibility

- **Plugin System**: Loadable compiler extensions
- **Custom Optimizations**: User-defined optimization passes
- **Language Extensions**: Add new language features
- **Backend Support**: Target different VMs or native code

## Implementation Notes

### Memory Management

- **Arena Allocation**: Use memory arenas for AST nodes
- **String Interning**: Share identical strings
- **Garbage Collection**: Automatic cleanup of temporary data
- **Memory allocation in VM** The VM allocates memory for objects and strings, the compiler does not allocate memory for objects and strings

### Error Recovery

- **Panic Mode**: Skip to next statement on error
- **Error Correction**: Suggest fixes for common errors
- **Multiple Errors**: Report all errors in one pass

### Debugging

- **Debug Symbols**: Include source location information
- **AST Visualization**: Dump AST for debugging
- **Bytecode Disassembly**: Human-readable bytecode output
