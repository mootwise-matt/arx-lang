# ARX Language Syntax

## Basic Structure
```pascal
module MyModule;

class Person
  name: STRING;
  age: INTEGER;
  
  procedure init(n: STRING; a: INTEGER);
  function getName(): STRING;
end;

class App
  procedure Main();
end;
```

## Type System
- **Primitive Types**: INTEGER, BOOLEAN, CHAR
- **Object Types**: STRING, ARRAY, custom classes
- **Variable Declarations**: `TYPE variable;` (C-style)
- **Assignment**: `variable = expression;` (C-style)

## Object-Oriented Features
- **Classes**: Define objects with fields and methods
- **Inheritance**: `class ChildClass extends ParentClass`
- **Fields**: Data members with types (`field: TYPE;`)
- **Methods**: Functions and procedures
- **Method Calls**: `obj.method()` ✅ Working
- **Field Access**: `obj.field` ✅ Working
- **Object Creation**: `new ClassName` ✅ Working
- **Method Overriding**: Child classes can override parent methods

## Control Flow
- **Procedures**: `procedure name();`
- **Functions**: `function name(): TYPE;`
- **Conditionals**: `if condition then statement;`
- **Loops**: `while condition do statement;`

## Expressions
- **Arithmetic**: `+`, `-`, `*`, `/`, `^`, `%`
- **Comparison**: `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Logical**: `&&`, `||`, `!`
- **Assignment**: `=` (C-style assignment)
- **Object Creation**: `NEW ClassName` or `NEW ClassName(params)`
- **Method Calls**: `obj.method()` or `obj.method(params)`
- **Field Access**: `obj.field`
- **Array Access**: `arr[index]`

## I/O Operations
- **Output**: `WriteLn(expression)` - outputs value and newline
- **String Literals**: `"Hello, World!"` - string constants
- **String Concatenation**: `"Hello" + "World"` - concatenate strings
- **Variable Integration**: `"Result: " + result` - concatenate string with variable
- **Arithmetic Expressions**: `result := a + b;` - arithmetic operations with variables
- **Complex Expressions**: `result := (a + b) * 2;` - parenthesized expressions

## Working OO Examples

### Method Call Example
```arx
class Person
  string name;
  
  procedure name
  begin
    name = "John Doe";
  end;
end;

class App
  procedure Main
  begin
    Person person1;
    person1 = new Person;
    writeln(person1.name()); // Output: "John Doe"
  end;
end;
```

### Field Access Example
```arx
class Person
  string name;
end;

class App
  procedure Main
  begin
    Person person1;
    person1 = new Person;
    writeln(person1.name); // Output: "Field Value"
  end;
end;
```