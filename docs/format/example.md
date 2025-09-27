# ARX Module Format Example

This document demonstrates the ARX module format with a practical example.

## Example Program

Consider this simple ARX program:

```arx
MODULE HelloWorld;

CLASS Person
    name: STRING;
    age: INTEGER;
    
    PROCEDURE init(n: STRING; a: INTEGER);
    FUNCTION getName(): STRING;
END;

CLASS App
    PROCEDURE Main();
        writeln("Hello, World!");
    END;
END;
```

## Compilation Process

1. **Compile the program:**
   ```bash
   arx hello.arx
   ```

2. **Inspect the generated module:**
   ```bash
   arxmod_info hello.arxmod
   ```

## Generated Module Structure

### Header Information
```
=== ARX Module Information ===
Magic: ARXMOD
Version: 1
Flags: 0x00000000
Header size: 64 bytes
TOC offset: 64
TOC size: 200 bytes
Data offset: 264
Data size: 18 bytes
App name length: 0
App data size: 0 bytes
Total file size: 282 bytes
```

### Section Details
```
=== ARX Module Sections ===
Section 0: CODE
  Offset: 0
  Size: 18 bytes
  Flags: 0x00000000

Section 1: STRINGS
  Offset: 0
  Size: 0 bytes
  Flags: 0x00000000

Section 2: SYMBOLS
  Offset: 0
  Size: 0 bytes
  Flags: 0x00000000

Section 3: DEBUG
  Offset: 0
  Size: 0 bytes
  Flags: 0x00000000

Section 4: APP
  Offset: 0
  Size: 0 bytes
  Flags: 0x00000000
```

## File Layout

```
Offset  Size    Description
------  ----    -----------
0       64      Header
64      200     Table of Contents (5 sections × 40 bytes)
264     18      CODE section (bytecode instructions)
282     -       End of file
```

## Bytecode Analysis

The CODE section contains 18 bytes of bytecode instructions:

```
Instruction 0: LIT    0
Instruction 1: HALT   0
```

This represents a minimal program that:
1. Loads literal value 0
2. Halts execution

## Validation

The module can be validated:

```bash
arxmod_info -validate hello.arxmod
# Output: 'hello.arxmod' is a valid ARX module file
```

## Hex Dump

For debugging, you can view the raw header:

```bash
arxmod_info -hex hello.arxmod
```

This shows the binary structure of the file header for format verification.

## Future Enhancements

As the compiler evolves, the module will include:

- **STRINGS section**: String literals like "Hello, World!"
- **SYMBOLS section**: Class and method definitions
- **DEBUG section**: Source line mapping for debugging
- **APP section**: Application metadata and configuration

## Tools Integration

The ARX module format integrates with:

- **Compiler**: Generates .arxmod files from .arx source
- **Virtual Machine**: Executes .arxmod files
- **Debugger**: Uses debug information for source mapping
- **Linker**: Combines multiple modules
- **Package Manager**: Manages module dependencies
