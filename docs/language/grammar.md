# ARX Language Grammar (EBNF)

## Module Structure
Module ::= "module" Ident ";" { ImportDecl } { ClassDecl | TypeDecl | FnDecl | ProcDecl }
ImportDecl ::= "import" Ident { "." Ident } ";"

## Classes & Members
ClassDecl ::= Visibility? "class" Ident [ Inherit ] ClassBody
Inherit ::= ":" Ident { "," Ident }
ClassBody ::= "begin" { MemberDecl } "end" ";"
MemberDecl ::= FieldDecl | MethodDecl | ConstructorDecl
FieldDecl ::= Visibility? Ident ":" Type ";"
MethodDecl ::= Visibility? (FunctionDecl | ProcedureDecl)
ConstructorDecl ::= Visibility? "constructor" Ident ParamList ";"

## Self References
SelfRef ::= "self" [ "." ( FieldAccess | MethodCall ) ]
FieldAccess ::= Ident
- **self**: Refers to the current object instance within a method
- **self.field**: Accesses a field of the current object
- **self.method()**: Calls a method on the current object
- **Scope**: Can only be used within class methods, not in standalone functions

## Functions & Procedures
FnDecl ::= Visibility? "fn" Ident ParamList [ ":" Type ] Block
FunctionDecl ::= "function" Ident ParamList ":" Type Block
ProcedureDecl ::= "procedure" Ident ParamList Block
ProcDecl ::= ProcedureDecl
ParamList ::= "(" [ Param { "," Param } ] ")"
Param ::= Ident ":" Type

## Types
TypeDecl ::= "type" Ident "=" TypeDef ";"
TypeDef ::= StructType | EnumType | AliasType
StructType ::= "struct" "begin" { FieldDecl } "end"
EnumType ::= "enum" "begin" { Ident [ "=" IntLiteral ] ";" } "end"
AliasType ::= Ident

## Statements
Block ::= "begin" { Stmt } "end"
Stmt ::= VarDecl | AssignStmt | ReturnStmt | IfStmt | WhileStmt | ForStmt | MatchStmt | ExprStmt | Block
VarDecl ::= Type Ident ";"
AssignStmt ::= Ident "=" Expr ";"
ReturnStmt ::= "return" [ Expr ] ";"
IfStmt ::= "if" Expr "then" Stmt [ "else" Stmt ]
WhileStmt ::= "while" Expr "do" Stmt
ForStmt ::= "for" Ident ":=" Expr "to" Expr "do" Stmt
MatchStmt ::= "match" Expr "of" { CaseClause } "end"
CaseClause ::= (Literal ":" Stmt) | ("else" ":" Stmt)
ExprStmt ::= Expr ";"

## Expressions
Expr ::= Assignment
Assignment ::= OrExpr [ AssignOp Assignment ]
OrExpr ::= AndExpr { "||" AndExpr }
AndExpr ::= RelExpr { "&&" RelExpr }
RelExpr ::= AddExpr { RelOp AddExpr }
RelOp ::= "==" | "!=" | "<" | "<=" | ">" | ">="
AddExpr ::= MulExpr { ("+" | "-") MulExpr }
MulExpr ::= Unary { ("" | "/" | "%") Unary }
Unary ::= Primary | ("!" | "-" | "~") Unary
Primary ::= Ident | Literal | "(" Expr ")" | MethodCall | SelfRef
SelfRef ::= "self" [ "." ( FieldAccess | MethodCall ) ]
FieldAccess ::= Ident
MethodCall ::= Ident "(" [ ArgList ] ")" | SelfRef "(" [ ArgList ] ")"
ArgList ::= Expr { "," Expr }

