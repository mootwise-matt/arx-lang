# ARX Compiler

Fresh implementation of the ARX compiler, built from scratch using modern C practices.

## Overview

The ARX compiler converts ARX source code (`.arx` files) into portable bytecode (`.arxmod` files) for execution by the ARX virtual machine.

## Architecture

```
ARX Source (.arx) → Lexer → Parser → Code Generator → Bytecode (.arxmod)
```

### Components

- **Lexer** (`lexer/`): Tokenizes ARX source code
- **Parser** (`parser/`): Builds Abstract Syntax Tree (AST)
- **Code Generator** (`codegen/`): Generates bytecode instructions
- **Common** (`common/`): Shared headers and opcodes

## Building

The compiler is built using the build system in the `/build/` directory:

```bash
cd build
make setup    # Create build directory structure
make          # Build the compiler
make test     # Test the compiler
```

### Build Targets

- `make` - Build the compiler
- `make debug` - Build with debug symbols
- `make release` - Build optimized release version
- `make clean` - Clean build artifacts
- `make test` - Test the compiler
- `make install` - Install to /usr/local/bin/
- `make uninstall` - Remove from /usr/local/bin/

## Usage

```bash
./arx [options] <input_file>

Options:
  -debug          Enable debug output
  -o <file>       Specify output file (default: input.arxmod)
  -h, --help      Show help message

Examples:
  ./arx program.arx
  ./arx -debug -o output.arxmod program.arx
```

## Features

### Current Implementation
- ✅ Lexical analysis (tokenization)
- ✅ Basic parsing (AST construction)
- ✅ Code generation (bytecode output)
- ✅ ARX module file format (.arxmod)
- ✅ Debug output support
- ✅ Error handling and reporting

### Language Support
- ✅ Module declarations
- ✅ Class definitions
- ✅ Field declarations
- ✅ Method declarations (procedures and functions)
- ✅ Basic syntax parsing

### Planned Features
- 🔄 Expression parsing and evaluation
- 🔄 Statement parsing and code generation
- 🔄 Type checking and validation
- 🔄 Symbol table management
- 🔄 Method body parsing
- 🔄 Error recovery and reporting
- 🔄 Optimization passes

## File Structure

```
compiler/
├── main.c              # Main entry point
├── lexer/              # Lexical analysis
│   ├── lexer.h
│   └── lexer.c
├── parser/             # Syntax analysis
│   ├── parser.h
│   └── parser.c
├── codegen/            # Code generation
│   ├── codegen.h
│   └── codegen.c
├── common/             # Shared components
│   └── opcodes.h
├── test/               # Test files
│   └── hello.arx
└── README.md           # This file
```

## Development

### Code Standards
- C99 standard with modern C features
- Consistent naming: snake_case for functions/variables
- Comprehensive error checking
- Clear, readable code with comments
- Defensive programming practices

### Testing
- Test with valid ARX programs
- Verify bytecode output format
- Test error handling and recovery
- Performance benchmarking

### Debugging
- Use `-debug` flag for verbose output
- Check AST construction
- Verify instruction generation
- Monitor memory usage

## License

Apache 2.0 - See LICENSE file in project root.

## Contributing

1. Follow the established code style
2. Add tests for new features
3. Update documentation
4. Ensure cross-platform compatibility
5. Test with various ARX programs
