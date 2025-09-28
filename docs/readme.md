# ARX Documentation

## Quick Start
- [CLI Commands](CLI_COMMANDS.md) - **Command-line interface reference**
- [Project Context](context/project-context.md) - Background and goals
- [Architecture Overview](architecture/overview.md) - High-level design
- [Language Syntax](language/syntax.md) - ARX language reference
- [Implementation Status](implementation-status.md) - Current development status

## Architecture
- [Compiler Design](architecture/compiler.md) - Parser and codegen
- [VM Design](architecture/vm.md) - Virtual machine implementation
- [Type System](architecture/type-system.md) - Type checking and runtime

## Language Reference
- [Grammar](language/grammar.md) - Formal language grammar
- [Examples](language/examples.md) - Code examples and patterns
- [API Reference](api/) - Function and class documentation

## Development
- [Build Process](development/build-process.md) - **Build system and binary management**
- [Coding Standards](development/coding-standards.md) - Development guidelines
- [Testing](development/testing.md) - Testing approach and tools
- [Debugging](development/debugging.md) - Debugging techniques

## Context for AI Assistants
This documentation provides context for AI assistants working on the ARX project. Key points:
- ARX is a Pascal-inspired, object-oriented language
- Compiles to portable bytecode (.arxmod)
- Executes on a stack-based virtual machine
- Supports modern OOP features with strong typing
- Designed for cross-platform compatibility
- Licensed under Apache 2.0

## Current Implementation Status
- ✅ **Lexer**: Complete tokenization with keyword recognition
- ✅ **Parser**: Full expression parsing, statement parsing, class parsing with AST
- ✅ **Object-Oriented Features**: Classes, inheritance, method overriding, NEW expressions
- ✅ **Code Generation**: Complete bytecode generation for all operations
- ✅ **ARX Module Format**: Complete .arxmod file format with sections
- ✅ **Virtual Machine**: Full VM with instruction execution and string operations
- ✅ **String Operations**: Complete string literals, concatenation, and output
- ✅ **Control Flow**: IF, FOR, WHILE statements with proper execution
- ✅ **Comparison Operators**: Complete support for ==, !=, <, <=, >, >=
- ✅ **Logical Operators**: Complete support for &&, ||, ! operators
- ✅ **Arithmetic Operations**: Complete arithmetic expression evaluation
- ✅ **Variable System**: Complete variable declarations, assignments, and references

## Contributing
When adding new features or making changes:
1. Update relevant documentation
2. Follow established patterns
3. Include tests and examples
4. Maintain cross-platform compatibility
5. Update this index if adding new sections