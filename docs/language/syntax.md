# ARX Language Syntax

## Basic Structure
```arx
module MyModule;

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
    writeln('Hello, World!');
  end;
end;
```

## Type System
- **Primitive Types**: `integer` (for numbers and boolean values)
- **Object Types**: `string` (for text), custom classes
- **Variable Declarations**: `TYPE variable;` (C-style) ✅ Working
- **Assignment**: `variable = expression;` (C-style) ✅ Working
- **Boolean Values**: Use `integer` (0 = false, 1 = true) ✅ Working

## Object-Oriented Features
- **Classes**: Define objects with fields and methods ✅ Working
- **Inheritance**: `class ChildClass extends ParentClass` ✅ Working
- **Fields**: Data members with types (`TYPE field;`) ✅ Working
- **Methods**: Functions and procedures ✅ Working
- **Method Calls**: `obj.method()` ✅ Working
- **Encapsulation**: Fields are private - access only through methods ✅ Working
- **Object Creation**: `new ClassName` ✅ Working
- **Method Overriding**: Child classes can override parent methods ✅ Working

### Encapsulation Rules
ARX enforces strict encapsulation for object-oriented programming:

- **Private Fields**: All class fields are private and cannot be accessed directly from outside the class
- **Method Access**: Fields can only be accessed through public methods (getters/setters)
- **Internal Access**: Within a class, methods can access fields directly using field names
- **No Direct Field Access**: Expressions like `obj.field` are not allowed - use `obj.getField()` instead

```arx
class Person
  string name;    // Private field
  integer age;    // Private field
  
  function getName: string
  begin
    return name;  // OK: internal access within class
  end;
  
  function getAge: integer
  begin
    return age;   // OK: internal access within class
  end;
end;

class App
  procedure Main
  begin
    Person person1;
    person1 = new Person;
    writeln(person1.getName()); // OK: access through method
    // writeln(person1.name);   // ERROR: direct field access forbidden
  end;
end;
```

## Control Flow
- **IF Statements**: `if condition then begin ... end;` ✅ Working
- **ELSEIF**: `elseif condition then begin ... end;` ✅ Working
- **ELSE**: `else begin ... end;` ✅ Working
- **FOR Loops**: `for variable = start to end do begin ... end;` ✅ Working
- **WHILE Loops**: `while condition do begin ... end;` ✅ Working
- **Procedures**: `procedure name begin ... end;` ✅ Working

## Expressions
- **Arithmetic**: `+`, `-`, `*`, `/`, `^`, `%` ✅ Working
- **Comparison**: `==`, `!=`, `<`, `<=`, `>`, `>=` ✅ Working
- **Logical**: `&&` (AND), `||` (OR), `!` (NOT) ✅ Working
- **Assignment**: `=` (C-style assignment) ✅ Working
- **Object Creation**: `new ClassName` ✅ Working
- **Method Calls**: `obj.method()` ✅ Working
- **String Concatenation**: `"Hello" + "World"` ✅ Working
- **Parenthesized Expressions**: `(a + b) * c` ✅ Working

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
- **Output**: `writeln(expression)` - outputs value and newline ✅ Working
- **String Literals**: `"Hello, World!"` - string constants ✅ Working
- **String Concatenation**: `"Hello" + "World"` - concatenate strings ✅ Working
- **Variable Integration**: `"Result: " + result` - concatenate string with variable ✅ Working
- **Arithmetic Expressions**: `result = a + b;` - arithmetic operations with variables ✅ Working
- **Complex Expressions**: `result = (a + b) * 2;` - parenthesized expressions ✅ Working

## Not Yet Implemented
- **Functions**: Function declarations and return statements
- **Method Parameters**: Parameters in method declarations and calls
- **Boolean Literals**: `true` and `false` keywords (use integers: 0/1)
- **Array Operations**: Array creation, indexing, and manipulation
- **File I/O**: Input/output operations with files
- **Exception Handling**: Try-catch blocks and error handling

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

### Encapsulation Example
```arx
class Person
  string name;
  integer age;
  
  function getName: string
  begin
    return name;
  end;
  
  function getAge: integer
  begin
    return age;
  end;
end;

class App
  procedure Main
  begin
    Person person1;
    person1 = new Person;
    writeln(person1.getName()); // Access through method
    writeln(person1.getAge());  // Access through method
    // person1.name; // ERROR: Direct field access not allowed
  end;
end;
```