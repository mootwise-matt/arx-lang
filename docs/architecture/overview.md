# ARX Architecture Overview

## High-Level Architecture
- ARX Source (.arx) → Parser → Bytecode (.arxmod) → VM → Execution

## Core Components

### Compiler
- **Lexer**: Tokenizes source code with keyword recognition
- **Parser**: Builds AST, parses expressions, statements, and classes
- **Codegen**: Emits instruction_t arrays for OO operations
- **Symbol Table**: Manages classes, methods, fields, and inheritance
- **Type Checker**: Validates types and semantics

### Virtual Machine
- **Interpreter**: Executes bytecode instructions
- **Memory Manager**: Handles object allocation
- **Stack Manager**: Manages execution stack
- **Object System**: Handles OOP features (partially implemented)
- **ARX Module Loader**: Loads .arxmod files with sections

### Type System
- **Primitive Types**: INTEGER, BOOLEAN, CHAR
- **Object Types**: STRING, ARRAY, custom classes
- **Type Checking**: Compile-time validation
- **Runtime Types**: Dynamic type information

## Data Flow
1. Source code is tokenized by lexer
2. Parser builds AST with actual values from source code
3. Code generator traverses AST and emits dynamic bytecode instructions
4. VM loads and executes bytecode with runtime value calculation
5. Objects are managed by memory system

## 🎯 **AST-Based Architecture**

### Dynamic Value Processing
- **No Hardcoded Values**: All values are captured from source code in AST nodes
- **Runtime Calculation**: VM performs all calculations using values from AST
- **Dynamic Code Generation**: Instructions generated based on actual parsed expressions
- **String Management**: Unlimited strings stored in ARX module format

## Key Design Decisions
- Stack-based VM for simplicity
- Bytecode format for portability
- Object-oriented model for modern development
- Strong typing for safety
- Cross-platform compatibility

## Architecture Documentation

### Core Specifications
- **[Bytecode Specification](bytecode-specification.md)**: Complete instruction set and format
- **[Compiler Architecture](compiler.md)**: Compiler design and implementation
- **[Type System](type-system.md)**: Type system design and semantics

### Development Guidelines
- **[Coding Standards](../development/coding-standards.md)**: Code style and conventions
- **[Development Roadmap](../../DEVELOPMENT_ROADMAP.md)**: Feature roadmap and milestones

## Compiler-VM Contract

The ARX compiler and virtual machine communicate through a well-defined bytecode specification:

### Bytecode Format
- **Instruction Size**: 9 bytes per instruction
- **Opcode Encoding**: 4-bit instruction type + 4-bit level
- **Operand**: 64-bit value (address, literal, or operation)

### Instruction Set
- **Core Instructions**: VM_LIT, VM_OPR, VM_LOD, VM_STO, etc.
- **Operations**: Arithmetic, comparison, I/O operations
- **String Operations**: String creation, concatenation, comparison
- **Object Operations**: Object creation, method calls, field access

### Memory Model
- **Stack-based**: 64-bit stack elements
- **Address Spaces**: Code, string, stack, heap
- **Calling Convention**: Standard procedure/function calls

See [Bytecode Specification](bytecode-specification.md) for complete details.