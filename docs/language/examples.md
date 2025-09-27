# ARX Language Examples

## Basic Program Structure

### Simple Module
```arx
module HelloWorld;

class App
  procedure Main
  begin
    WriteLn('Hello, World!');
  end;
end;
```

### String Concatenation
```arx
module StringConcat;

class App
  procedure Main
  begin
    WriteLn('Hello' + 'World');
    WriteLn('ARX' + ' ' + 'Language');
  end;
end;
```

### Variable Integration
```arx
module VariableIntegration;

class App
  procedure Main
  begin
    integer result;
    result = 25;
    writeln('Result: ' + result);
  end;
end;
```

### Full Arithmetic Integration
```arx
module ArithmeticIntegration;

class App
  procedure Main
  begin
    integer a;
    integer b;
    integer result;
    
    a = 10;
    b = 5;
    
    result = a + b;
    writeln('Addition: ' + result);
    
    result = a * b;
    writeln('Multiplication: ' + result);
    
    result = (a + b) * 2;
    writeln('Complex: ' + result);
  end;
end;
```

### Module with Multiple Classes
```arx
module Calculator;

class App
  procedure Main
  begin
    calc := new Calculator;
    result := calc.add(5, 3);
    WriteLn('Result: ', result);
  end;
end;

class Calculator
  function add(a: integer; b: integer): integer
  begin
    return a + b;
  end;
end;
```

## Object-Oriented Features

### Class Definition with Fields
```arx
module PersonDemo;

class App
  procedure Main
  begin
    person := new Person;
    person.setName('John');
    person.setAge(30);
    WriteLn('Name: ', person.getName());
    WriteLn('Age: ', person.getAge());
  end;
end;

class Person
  name: string;
  age: integer;
  
  procedure init
  begin
    name := '';
    age := 0;
  end;
  
  procedure setName(n: string)
  begin
    name := n;
  end;
  
  procedure setAge(a: integer)
  begin
    age := a;
  end;
  
  function getName(): string
  begin
    return name;
  end;
  
  function getAge(): integer
  begin
    return age;
  end;
end;
```

### Inheritance and Method Overriding
```arx
module InheritanceDemo;

class App
  procedure Main
  begin
    animal := new Animal;
    dog := new Dog;
    
    animal.makeSound();  // Outputs: "Some sound"
    dog.makeSound();     // Outputs: "Woof!" (overridden)
    dog.eat();           // Outputs: "Eating" (inherited)
  end;
end;

class Animal
  name: string;
  
  procedure init
  begin
    name := 'Animal';
  end;
  
  procedure makeSound
  begin
    WriteLn('Some sound');
  end;
  
  procedure eat
  begin
    WriteLn('Eating');
  end;
end;

class Dog extends Animal
  breed: string;
  
  procedure init
  begin
    name := 'Dog';
    breed := 'Unknown';
  end;
  
  procedure makeSound
  begin
    WriteLn('Woof!');
  end;
end;
```

### Constructor Parameters
```arx
module ConstructorDemo;

class App
  procedure Main
  begin
    // Default constructor
    person1 := new Person;
    
    // Constructor with parameters
    person2 := new Person('Alice', 25);
    
    WriteLn('Person 1: ', person1.getName());
    WriteLn('Person 2: ', person2.getName());
  end;
end;

class Person
  name: string;
  age: integer;
  
  procedure init
  begin
    name := 'Unknown';
    age := 0;
  end;
  
  procedure init(n: string; a: integer)
  begin
    name := n;
    age := a;
  end;
  
  function getName(): string
  begin
    return name;
  end;
end;
```

## Assignment vs Comparison Operators

ARX uses C-style operators for clarity and consistency:

### Assignment Operator (`=`)
```arx
module AssignmentDemo;

class App
  procedure Main
  begin
    integer x;
    string name;
    
    // Assignment: assigns values to variables
    x = 10;           // Assigns 10 to x
    name = "Alice";   // Assigns "Alice" to name
    
    writeln('x = ' + x);
    writeln('name = ' + name);
  end;
end;
```

### Comparison Operator (`==`)
```arx
module ComparisonDemo;

class App
  procedure Main
  begin
    integer x;
    string name;
    
    x = 10;
    name = "Alice";
    
    // Comparison: compares values for equality
    if x == 10 then
    begin
      writeln('x equals 10');
    end;
    
    if name == "Alice" then
    begin
      writeln('Hello Alice!');
    end;
    
    if x == 5 then
    begin
      writeln('This will not print');
    else
      writeln('x is not equal to 5');
    end;
  end;
end;
```

### Combined Example
```arx
module AssignmentComparisonDemo;

class App
  procedure Main
  begin
    integer score;
    string grade;
    
    // Assignment
    score = 85;
    
    // Comparison in IF statement
    if score == 85 then
    begin
      grade = "B+";  // Assignment
    elseif score == 90 then
      grade = "A-";  // Assignment
    else
      grade = "C";   // Assignment
    end;
    
    writeln('Score: ' + score);
    writeln('Grade: ' + grade);
  end;
end;
```

## Expression Examples

### Arithmetic Expressions
```arx
module MathDemo;

class App
  procedure Main
  begin
    a := 10;
    b := 5;
    
    sum := a + b;
    diff := a - b;
    product := a * b;
    quotient := a / b;
    
    WriteLn('Sum: ', sum);
    WriteLn('Difference: ', diff);
    WriteLn('Product: ', product);
    WriteLn('Quotient: ', quotient);
  end;
end;
```

### Logical Expressions
```arx
module LogicDemo;

class App
  procedure Main
  begin
    x := 10;
    y := 5;
    
    if (x > y) then
    begin
      WriteLn('x is greater than y');
    end;
    
    if (x == 10 && y == 5) then
    begin
      WriteLn('Both conditions are true');
    end;
  end;
end;
```

### Object Method Calls
```arx
module MethodCallDemo;

class App
  procedure Main
  begin
    calculator := new Calculator;
    
    result1 := calculator.add(10, 20);
    result2 := calculator.multiply(5, 6);
    
    WriteLn('Addition result: ', result1);
    WriteLn('Multiplication result: ', result2);
  end;
end;

class Calculator
  function add(a: integer; b: integer): integer
  begin
    return a + b;
  end;
  
  function multiply(a: integer; b: integer): integer
  begin
    return a * b;
  end;
end;
```

## Complex Examples

### Bank Account System
```arx
module BankAccount;

class App
  procedure Main
  begin
    account := new BankAccount(1000);
    
    WriteLn('Initial balance: ', account.getBalance());
    
    account.deposit(500);
    WriteLn('After deposit: ', account.getBalance());
    
    account.withdraw(200);
    WriteLn('After withdrawal: ', account.getBalance());
  end;
end;

class BankAccount
  balance: integer;
  
  procedure init(initialBalance: integer)
  begin
    balance := initialBalance;
  end;
  
  procedure deposit(amount: integer)
  begin
    balance := balance + amount;
  end;
  
  procedure withdraw(amount: integer)
  begin
    if (amount <= balance) then
    begin
      balance := balance - amount;
    end
    else
    begin
      WriteLn('Insufficient funds');
    end;
  end;
  
  function getBalance(): integer
  begin
    return balance;
  end;
end;
```

### Shape Hierarchy
```arx
module ShapeDemo;

class App
  procedure Main
  begin
    circle := new Circle(5);
    rectangle := new Rectangle(4, 6);
    
    WriteLn('Circle area: ', circle.getArea());
    WriteLn('Rectangle area: ', rectangle.getArea());
  end;
end;

class Shape
  procedure init
  begin
    // Base class constructor
  end;
  
  function getArea(): integer
  begin
    return 0;  // Override in subclasses
  end;
end;

class Circle extends Shape
  radius: integer;
  
  procedure init(r: integer)
  begin
    radius := r;
  end;
  
  function getArea(): integer
  begin
    return 3 * radius * radius;  // Simplified pi = 3
  end;
end;

class Rectangle extends Shape
  width: integer;
  height: integer;
  
  procedure init(w: integer; h: integer)
  begin
    width := w;
    height := h;
  end;
  
  function getArea(): integer
  begin
    return width * height;
  end;
end;
```

## Current Implementation Status

### ✅ Working Features
- Module and class declarations
- Field declarations with types
- Method declarations (procedures and functions)
- Class inheritance with `extends`
- NEW expressions with and without parameters
- Basic expression parsing
- Constructor parameter parsing
- Method overriding

### 🔄 Partially Working Features
- String literals (parsed but not fully integrated with VM)
- Method calls (parsed but not executed)
- Field access (parsed but not executed)
- Complex expressions (parsed but not fully generated)

### ❌ Not Yet Implemented
- Control flow statements (if-then-else, while loops)
- Array operations
- Exception handling
- Advanced OO features (interfaces, generics)

## Compilation and Execution

### Compiling ARX Programs
```bash
# Compile ARX source to bytecode
./arx -debug -show-bytecode program.arx

# View generated bytecode
./arxmod_info -sections program.arxmod
./arxmod_info -hex program.arxmod
```

### Running ARX Programs
```bash
# Execute ARX bytecode
./arxvm program.arxmod

# Execute with debug output
./arxvm -debug program.arxmod
```

## Best Practices

### Code Organization
- Use descriptive module names
- Group related classes in the same module
- Use meaningful class and method names
- Follow Pascal-style naming conventions

### Object-Oriented Design
- Use inheritance to model "is-a" relationships
- Override methods in subclasses when behavior differs
- Use constructors to initialize object state
- Keep methods focused on single responsibilities

### Error Handling
- Validate input parameters in methods
- Use meaningful error messages
- Handle edge cases appropriately
- Test with various input values
