# ARX Project Context

## Project Overview
ARX is a Pascal-inspired, object-oriented programming language designed for cross-platform development. The project includes a complete compiler toolchain and virtual machine implementation.

## Project Goals
- **Modern OOP**: Support for classes, inheritance, method overriding, and polymorphism
- **Strong Typing**: Compile-time type checking and runtime type safety
- **Cross-Platform**: Portable bytecode that runs on multiple architectures
- **Performance**: Efficient compilation and execution
- **Simplicity**: Clean, readable syntax inspired by Pascal
- **Extensibility**: Modular design for easy feature addition

## Current Status (December 2024)

### ✅ Completed Features
- **Complete Lexer**: Token recognition for all language constructs
- **AST-Based Parser**: ✅ Complete - captures actual values from source code in AST nodes
- **Object-Oriented Features**: Classes, inheritance, method overriding, NEW expressions
- **Code Generation**: Bytecode generation for OO operations
- **ARX Module Format**: Complete .arxmod file format with sections
- **Virtual Machine**: Basic VM with instruction execution
- **Symbol Table**: Complete symbol management with inheritance tracking
- **Dynamic Values**: ✅ No hardcoded values - all calculated at runtime in VM

### 🎯 **Major Breakthrough: AST Implementation**
- **✅ AST Parsing Complete**: All expressions parsed into AST nodes with actual values
- **✅ Number Literals**: Values like `20`, `5`, `2` captured as AST nodes
- **✅ String Literals**: All string literals captured as AST nodes
- **✅ Identifiers**: Variable references captured as AST nodes
- **✅ Binary Operations**: `*`, `/`, `+` operators captured as AST nodes
- **✅ Assignment Statements**: Complete assignment parsing with AST nodes
- **✅ Parenthesized Expressions**: Complex expressions correctly parsed

### 🔄 In Progress
- **AST-Based Code Generation**: Generate instructions from AST nodes instead of hardcoded patterns
- **Runtime Value Calculation**: VM performs calculations using values from AST
- **Dynamic Code Generation**: Instructions generated based on actual parsed expressions

### ❌ Planned Features
- **Control Flow**: if-then-else, while loops, for loops
- **Array Operations**: Array creation, indexing, and manipulation
- **Exception Handling**: Try-catch blocks and error handling
- **Standard Library**: Built-in classes and functions
- **Advanced OO**: Interfaces, generics, abstract classes

## Architecture

### Compiler Architecture
```
ARX Source (.arx) → Lexer → Parser → Code Generator → Bytecode (.arxmod)
```

**Components:**
- **Lexer**: Tokenizes source code with keyword recognition
- **Parser**: Builds AST, parses expressions, statements, and classes
- **Code Generator**: Emits instruction_t arrays for OO operations
- **Symbol Table**: Manages classes, methods, fields, and inheritance
- **Type Checker**: Validates types and semantics

### Virtual Machine Architecture
```
Bytecode (.arxmod) → VM Loader → VM Core → Runtime Execution
```

**Components:**
- **VM Core**: Instruction execution engine with stack management
- **Memory Manager**: Object allocation and garbage collection
- **ARX Module Loader**: Loads .arxmod files with sections
- **Runtime**: Basic operations and I/O handling

## Language Features

### Object-Oriented Programming
- **Classes**: Define objects with fields and methods
- **Inheritance**: `class Child extends Parent` syntax
- **Method Overriding**: Child classes can override parent methods
- **Object Creation**: `NEW ClassName` and `NEW ClassName(params)`
- **Method Calls**: `object.method()` and `object.method(params)`
- **Field Access**: `object.field`

### Type System
- **Primitive Types**: INTEGER, BOOLEAN, CHAR, REAL
- **Object Types**: STRING, ARRAY, custom classes
- **Strong Typing**: Compile-time type checking
- **Type Safety**: Runtime type validation

### Syntax
- **Pascal-Inspired**: Clean, readable syntax
- **Case-Insensitive**: Keywords and identifiers
- **Block Structure**: BEGIN/END blocks for code organization
- **Assignment**: Uses `:=` for assignment operations

## Development Environment

### Build System
- **Makefiles**: Separate build systems for compiler and VM
- **Cross-Platform**: Supports macOS, Linux, and Windows
- **Debug Support**: Debug builds with comprehensive logging

### Tools
- **Compiler**: `arx` - Compiles ARX source to bytecode
- **VM**: `arxvm` - Executes ARX bytecode
- **Module Inspector**: `arxmod_info` - Inspects .arxmod files
- **Debug Flags**: Comprehensive debug output for troubleshooting

### Testing
- **Unit Tests**: Individual component testing
- **Integration Tests**: End-to-end compilation and execution
- **Regression Tests**: Ensure new features don't break existing functionality

## Project Structure

### Source Code Organization
```
arx/
├── compiler/           # Compiler implementation
│   ├── lexer/         # Lexical analysis
│   ├── parser/        # Syntax analysis and parsing
│   ├── codegen/       # Code generation
│   ├── symbols/       # Symbol table management
│   ├── types/         # Type system
│   └── arxmod/        # ARX module format
├── vm/                # Virtual machine implementation
│   ├── core/          # VM core and instruction execution
│   ├── loader/        # ARX module loading
│   └── runtime/       # Runtime operations
├── docs/              # Documentation
├── build/             # Build artifacts
└── archive/           # Reference implementation (old)
```

### Documentation Structure
```
docs/
├── architecture/      # System architecture documentation
├── language/          # Language reference and examples
├── development/       # Development guidelines and tools
├── context/           # Project context and decisions
└── format/            # File format specifications
```

## Design Decisions

### Language Design
- **Pascal Inspiration**: Familiar syntax for developers
- **Object-Oriented**: Modern OOP features for complex applications
- **Strong Typing**: Safety and performance benefits
- **Cross-Platform**: Bytecode approach for portability

### Implementation Design
- **Modular Architecture**: Separate compiler and VM for flexibility
- **Stack-Based VM**: Simple and efficient execution model
- **Bytecode Format**: Portable and extensible instruction set
- **Symbol Table**: Comprehensive symbol management

### Tool Design
- **Command-Line Tools**: Simple, scriptable interface
- **Debug Support**: Comprehensive debugging capabilities
- **Cross-Platform**: Consistent behavior across platforms

## Future Plans

### Short-term (Next 3 months)
- **Complete String Operations**: Full string literal integration with VM
- **Implement OO Operations**: Complete VM support for object operations
- **Method Resolution**: Virtual method dispatch
- **Field Access**: Object field access and assignment

### Medium-term (3-6 months)
- **Control Flow**: Implement conditionals and loops
- **Array Operations**: Array creation, indexing, and manipulation
- **Error Handling**: Comprehensive error reporting and recovery
- **Performance Optimization**: Optimize compilation and execution

### Long-term (6+ months)
- **Standard Library**: Built-in classes and functions
- **Advanced OO Features**: Interfaces, generics, abstract classes
- **IDE Support**: Language server and editor integration
- **Package Management**: Module system and dependency management

## Contributing

### Getting Started
1. **Read Documentation**: Review architecture and language documentation
2. **Set Up Environment**: Build compiler and VM from source
3. **Run Tests**: Verify existing functionality works
4. **Choose Feature**: Pick a feature from the roadmap
5. **Implement**: Follow coding standards and patterns
6. **Test**: Add tests for new functionality
7. **Document**: Update relevant documentation

### Development Guidelines
- **Follow Patterns**: Use existing code patterns and conventions
- **Add Tests**: Include tests for new features
- **Update Docs**: Keep documentation current
- **Cross-Platform**: Ensure compatibility across platforms
- **Performance**: Consider performance implications

### Code Quality
- **C99 Standard**: Use modern C features where appropriate
- **Error Handling**: Comprehensive error checking and reporting
- **Memory Management**: Proper allocation and cleanup
- **Documentation**: Clear comments and documentation

## License
ARX is licensed under the Apache 2.0 License, allowing for both commercial and non-commercial use with minimal restrictions.

## Community
- **Documentation**: Comprehensive documentation for developers
- **Examples**: Working examples and tutorials
- **Debugging**: Tools and techniques for troubleshooting
- **Testing**: Test suite for validation and regression testing

## Conclusion
ARX represents a modern approach to programming language design, combining the familiarity of Pascal syntax with modern object-oriented features. The project has a solid foundation with complete parsing of OO features and is well-positioned for continued development of advanced language features and tooling.
