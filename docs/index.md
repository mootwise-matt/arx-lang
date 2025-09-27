---
layout: default
title: Welcome to ARX
---

# ARX Programming Language

Welcome to **ARX**, a modern, Pascal-inspired programming language designed for the 21st century with a virtual machine target for cross-platform compatibility.

## 🚀 **What is ARX?**

ARX is an experimental programming language that combines the clarity and structure of Pascal with modern object-oriented features. It compiles to portable bytecode that runs on a custom virtual machine, making it perfect for:

- **Educational purposes** - Learn programming concepts with a clean, readable syntax
- **Cross-platform development** - Write once, run anywhere
- **System programming** - Bootstrapping and embedded systems
- **Research and experimentation** - Modern language design concepts

## ✨ **Key Features**

### ✅ **Fully Implemented**
- **Object-Oriented Programming** - Classes, inheritance, method overriding
- **Control Flow** - IF statements, FOR loops, WHILE loops
- **Logical Operators** - AND (`&&`), OR (`||`), NOT (`!`)
- **Comparison Operators** - `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Arithmetic Operations** - Complete expression evaluation
- **String Operations** - String literals, concatenation, output
- **Variable System** - Type-based declarations and assignments

### 🔄 **In Development**
- **Functions and Procedures** - Function declarations and calls
- **Enhanced OOP** - Constructors, method parameters
- **Arrays** - Array creation and manipulation
- **File I/O** - Input/output operations

## 🎯 **Quick Start**

### 1. **Hello World Example**
```arx
module HelloWorld;

class App
  procedure Main
  begin
    writeln('Hello, World!');
  end;
end;
```

### 2. **Object-Oriented Example**
```arx
module OODemo;

class Person
  string name;
  integer age;
  
  procedure init
  begin
    name = "John Doe";
    age = 25;
  end;
end;

class App
  procedure Main
  begin
    person := new Person;
    writeln('Person: ' + person.name);
  end;
end;
```

### 3. **Control Flow Example**
```arx
module ControlDemo;

class App
  procedure Main
  begin
    integer i;
    
    // FOR loop
    for i = 1 to 5 do begin
      writeln('Count: ' + i);
    end;
    
    // IF statement
    if i > 3 then begin
      writeln('i is greater than 3');
    end;
  end;
end;
```

## 📚 **Documentation**

- **[Language Syntax](syntax.md)** - Complete ARX language reference
- **[Examples](examples.md)** - Comprehensive code examples
- **[Architecture](architecture/overview.md)** - Compiler and VM design
- **[Implementation Status](implementation-status.md)** - Current development status

## 🛠️ **Building and Running**

### Prerequisites
- GCC or Clang
- Make (optional)

### Build Commands
```bash
# Build the compiler and VM
make

# Compile an ARX program
./arx program.arx

# Run the compiled program
./arxvm program.arxmod
```

### Test Examples
```bash
# Run all examples
./arx examples/01_hello_world.arx && ./arxvm examples/01_hello_world.arxmod
./arx examples/02_arithmetic.arx && ./arxvm examples/02_arithmetic.arxmod
./arx examples/03_control_statements.arx && ./arxvm examples/03_control_statements.arxmod
./arx examples/04_oo_features.arx && ./arxvm examples/04_oo_features.arxmod
./arx examples/05_logical_operators.arx && ./arxvm examples/05_logical_operators.arxmod
```

## 🎉 **Current Status**

**ARX is fully functional!** The core language features are implemented and tested:

- ✅ **Lexer** - Complete tokenization
- ✅ **Parser** - Full AST-based parsing
- ✅ **Code Generator** - Bytecode generation
- ✅ **Virtual Machine** - Complete execution engine
- ✅ **Object-Oriented Features** - Classes, inheritance, methods
- ✅ **Control Flow** - IF, FOR, WHILE statements
- ✅ **Logical Operators** - Complete boolean logic
- ✅ **String Operations** - Full string support

## 🤝 **Contributing**

ARX is an open-source project licensed under Apache 2.0. Contributions are welcome!

- **GitHub Repository**: [mootwise-matt/arx-lang](https://github.com/mootwise-matt/arx-lang)
- **Issues**: Report bugs or request features
- **Pull Requests**: Submit improvements and new features

## 📄 **License**

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

---

**Ready to explore ARX?** Check out the [examples](examples.md) or dive into the [language syntax](syntax.md)!
