# ARX Language Examples

This directory contains comprehensive examples that demonstrate the ARX programming language features. Each example is complete and tested to ensure it compiles and executes correctly.

## Examples Overview

### 1. Hello World (`01_hello_world.arx`)
**Demonstrates**: Basic module structure, string literals, output, string concatenation

**Features**:
- Module declaration
- Class and procedure structure
- String variable declaration and assignment
- String literals
- Output with `writeln`
- String concatenation

**Usage**:
```bash
./arx examples/01_hello_world.arx
./arxvm examples/01_hello_world.arxmod
```

### 2. Arithmetic Operations (`02_arithmetic.arx`)
**Demonstrates**: Integer arithmetic, operator precedence, string concatenation

**Features**:
- Variable declarations (integer type)
- Basic arithmetic operations: `+`, `-`, `*`, `/`, `%`, `^`
- Operator precedence with parentheses
- Complex expressions
- String concatenation with arithmetic results

**Usage**:
```bash
./arx examples/02_arithmetic.arx
./arxvm examples/02_arithmetic.arxmod
```

### 3. Control Statements (`03_control_statements.arx`)
**Demonstrates**: Comparison operators, logical operators, expressions for control flow

**Features**:
- Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical operators: `and`, `or`, `not` (syntax ready)
- Arithmetic expressions for control flow
- Notes on planned control flow features

**Note**: IF/WHILE/FOR statements are planned but not yet implemented. This example shows the expressions that will be used in control flow.

**Usage**:
```bash
./arx examples/03_control_statements.arx
./arxvm examples/03_control_statements.arxmod
```

### 4. Object-Oriented Features (`04_oo_features.arx`)
**Demonstrates**: Classes, inheritance, constructors, methods, field access

**Features**:
- Class declarations with fields
- Class inheritance with `extends`
- Constructor methods (`init`)
- Getter and setter methods
- Object creation with `new`
- Method calls
- Field access
- Method overriding in derived classes

**Note**: Method parameters are planned but not yet implemented.

**Usage**:
```bash
./arx examples/04_oo_features.arx
./arxvm examples/04_oo_features.arxmod
```

## ARX Language Features Demonstrated

### ✅ Working Features
- **Module System**: Module declarations and organization
- **Classes**: Class definitions with fields and methods
- **Inheritance**: Class inheritance with `extends` keyword
- **Object Creation**: `new ClassName` syntax
- **Method Calls**: `object.method()` syntax
- **Field Access**: `object.field` syntax
- **Variable Declarations**: Type-based variable declarations
- **Arithmetic Operations**: All basic arithmetic operators
- **String Operations**: String literals and concatenation
- **Output**: `writeln` for program output
- **Expressions**: Complex arithmetic and string expressions

### 🔄 Planned Features (Not Yet Implemented)
- **Control Flow**: IF/WHILE/FOR statements
- **Boolean Literals**: `true` and `false` keywords
- **Method Parameters**: Parameters in method declarations and calls
- **Array Operations**: Array creation, indexing, and manipulation
- **Exception Handling**: Try-catch blocks and error handling

## Compilation and Execution

### Compiling Examples
```bash
# Compile any example
./arx examples/01_hello_world.arx

# Compile with debug output
./arx -debug examples/01_hello_world.arx
```

### Running Examples
```bash
# Execute compiled bytecode
./arxvm examples/01_hello_world.arxmod

# Execute with debug output
./arxvm -debug examples/01_hello_world.arxmod
```

### Testing All Examples
```bash
# Test compilation of all examples
for file in examples/*.arx; do
    echo "Testing: $file"
    ./arx "$file"
done

# Test execution of all examples
for file in examples/*.arxmod; do
    echo "Executing: $file"
    ./arxvm "$file"
done
```

## Language Syntax Reference

### Module Structure
```arx
module ModuleName;

class App
  procedure Main
  begin
    // Program code here
  end;
end;
```

### Variable Declarations
```arx
integer count;
string name;
boolean flag;
```

### Class Definitions
```arx
class Person
  string name;
  integer age;
  
  procedure init
  begin
    name = "Unknown";
    age = 0;
  end;
  
  procedure getName : string
  begin
    return name;
  end;
end;
```

### Inheritance
```arx
class Student extends Person
  string major;
  
  procedure init
  begin
    name = "Student";
    age = 18;
    major = "Undeclared";
  end;
end;
```

### Object Creation and Usage
```arx
Person person;
person = new Person;
writeln(person.getName());
writeln(person.name);
```

## Regression Testing

All examples are tested for regression after each change to the compiler or VM:

1. **Compilation Test**: Each example must compile without errors
2. **Execution Test**: Each example must execute and produce expected output
3. **Feature Test**: Each example must demonstrate its intended features correctly

If any example fails to compile or execute after a change, it indicates a regression that must be fixed before the change is accepted.

## Contributing

When adding new examples:

1. Follow the naming convention: `NN_description.arx`
2. Include comprehensive comments explaining the features demonstrated
3. Test compilation and execution thoroughly
4. Update this README with the new example
5. Ensure the example works with the current implementation status

## Implementation Status

For the most up-to-date information on ARX language implementation status, see:
- `docs/implementation-status.md` - Current implementation status
- `docs/language/syntax.md` - Language syntax reference
- `docs/language/examples.md` - Additional language examples
