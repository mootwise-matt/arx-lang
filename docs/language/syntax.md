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
- **Logical**: `&&` (AND), `||` (OR), `!` (NOT)
- **Assignment**: `=` (C-style assignment)
- **Object Creation**: `NEW ClassName` or `NEW ClassName(params)`
- **Method Calls**: `obj.method()` or `obj.method(params)`
- **Field Access**: `obj.field`
- **Array Access**: `arr[index]`

### Logical Operators
ARX supports C-style logical operators for boolean expressions:

- **Logical AND (`&&`)**: Returns true if both operands are true
  ```arx
  if age >= 18 && score >= 80 then
  begin
    writeln('Eligible');
  end;
  ```

- **Logical OR (`||`)**: Returns true if at least one operand is true
  ```arx
  if temperature > 30 || weather == "sunny" then
  begin
    writeln('Good weather');
  end;
  ```

- **Logical NOT (`!`)**: Negates a boolean expression
  ```arx
  if !(value > 0) then
  begin
    writeln('Value is zero or negative');
  end;
  ```

- **Complex Expressions**: Use parentheses for precedence
  ```arx
  if (age >= 18 && income >= 30000) || hasLicense then
  begin
    writeln('Eligible for loan');
  end;
  ```

### Assignment vs Comparison Operators
ARX uses C-style operators for clarity:
- **Assignment**: `=` - assigns a value to a variable
  ```arx
  integer x;
  x = 10;        // Assigns 10 to x
  string name;
  name = "John"; // Assigns "John" to name
  ```
- **Comparison**: `==` - compares two values for equality
  ```arx
  if x == 10 then     // Compares x with 10
  begin
    writeln("x equals 10");
  end;
  
  if name == "John" then  // Compares name with "John"
  begin
    writeln("Hello John!");
  end;
  ```

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