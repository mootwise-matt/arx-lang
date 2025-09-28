# ARX
A Pascal inspired compiler modernised for the 21st Century with a virtual machine target to bootstrap systems.

## ⚠️ Important: Archive Folder
The `archive/` folder contains the **OLD VERSION** of this project and is **FOR REFERENCE ONLY**. Do not use archive code for active development.

## Current Development
This project is starting fresh with a clean architecture. See `docs/` for current documentation and `PROJECT_STATUS.md` for development approach.

## Quick Start
1. Read the documentation in `docs/`
2. Follow the development guidelines
3. **Ignore** the archive folder for new development
4. Use archive only for historical context


## Current Implementation Status
- ✅ **Complete Lexer**: Token recognition for all language constructs
- ✅ **Full Parser**: Expression parsing, statement parsing, class parsing with AST
- ✅ **AST-Based Architecture**: Complete Abstract Syntax Tree implementation
- ✅ **Object-Oriented Features**: Classes, inheritance, method overriding, NEW expressions, method calls, field access
- ✅ **Code Generation**: Complete bytecode generation for all operations
- ✅ **ARX Module Format**: Complete .arxmod file format with sections
- ✅ **Virtual Machine**: Full VM with instruction execution and string operations
- ✅ **String Operations**: String concatenation, integer-to-string conversion, output
- ✅ **Variable System**: Variable declarations, assignments, references, and symbol table
- ✅ **Arithmetic Operations**: Complete arithmetic expression evaluation with variables
- ✅ **Complex Expressions**: Parenthesized expressions, multiple operators, type conversion
- ✅ **Dynamic Values**: All values computed at runtime in VM (no hardcoded values)
- ✅ **C-Style Syntax**: Variable declarations (`TYPE var;`) and assignments (`var = expr;`)
- ✅ **CODE Section Loading**: VM successfully loads and executes bytecode
- ✅ **FOR Loops**: Complete implementation with proper termination and body execution
- ✅ **WHILE Loops**: Basic implementation working with simple conditions
- ✅ **Label Resolution**: Two-pass compilation with proper jump address resolution
- ✅ **Verified Working**: All examples execute perfectly with full functionality

**🎉 CORE FUNCTIONALITY COMPLETE!** The ARX programming language toolchain is now fully functional with control flow statements.

See [Implementation Status](docs/implementation-status.md) for detailed information.

## Introduction
ARX cross-compiler was developed to be able to write programs for an HD6309-based 8-bit computer. At the time of writing, the only way to take advantage of the HD6309 capabilities was to write directly in assembly langauge.

The compiler is not aimed to produce the fastest code or smallest code, but to allow to bring up a system in a short amount of time. This is accomplished by generating byte code for a virtual machine instead of native HD6309 code. A small hand-crafted virtual machine executes the byte code on the 8-bit CPU.

Porting the compiler to another CPU is straightforward. Only the virtual machine needs to be implemented on the target CPU; the byte code remains the same.

ARX implements a subset of the Pascal langauge. For instance, it does not support floating-point numbers or pointers.

## Building ARX

### Quick Build
```bash
# Build everything and install binaries to root
./build.sh
```

### Manual Build
```bash
# Build in build directory
cd build
make

# Binaries are automatically installed to root directory
```

### Important Notes
- **Always use root binaries** (`./arx`, `./arxvm`) for running examples
- **Build directory binaries** are intermediate - don't use them directly
- **After any build**, root binaries are automatically updated

See [Build Process Documentation](docs/development/build-process.md) for detailed information.

ARX can be built on Windows, Linux, and macOS using the unified build system.

### Quick Start
```bash
make              # Build both compiler and virtual machine
make test         # Test the complete workflow
make clean        # Clean build artifacts
```

### Dependencies
- GCC or Clang
- Make (optional, for build automation)
- CMake (optional, for advanced builds)

### Build Targets
- `make arx` - Build only the ARX compiler
- `make arxvm` - Build only the ARX virtual machine  
- `make test` - Test normal execution
- `make test-debug` - Test with debug output
- `make clean` - Remove all build artifacts

### Manual Build
If you prefer to build manually:
```bash
# Build compiler
gcc -std=c99 -Wall -Wextra -I./common -o arx src/main.c src/parser.c src/lexer.c src/symtbl.c src/typestack.c

# Build virtual machine
gcc -std=c99 -Wall -Wextra -I./common -DTARGET_X86_64=1 -o arxvm virtualmachine/arxvm.c virtualmachine/vm.c
```

## Ready-made binaries
At this time, there are no ready-made binaries available.
