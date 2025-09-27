/*
 * ARX Lexer - Tokenizes ARX source code
 * Fresh implementation with modern C practices
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Token types
typedef enum
{
    TOK_NONE    = 0,
    TOK_EOF,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_SEMICOL,
    TOK_COLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_STAR,
    TOK_PLUS,
    TOK_MINUS,
    TOK_EQUAL,
    TOK_NEQ,                // !=
    TOK_AND,                // &&
    TOK_OR,                 // ||
    TOK_PERIOD,
    TOK_COMMA,
    TOK_EXCLAMATION,
    TOK_QUESTION,
    TOK_ASSIGN,             // :=    
    TOK_EOL,
    TOK_LEQ,                // <=
    TOK_GEQ,                // >=
    TOK_LESS,               // <
    TOK_GREATER,            // >
    TOK_SLASH,              // /
    TOK_CARET,              // ^ (exponentiation)
    TOK_PERCENT,            // % (modulo)
    TOK_HASH,               // #
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_DOUBLEPERIOD,
    TOK_LBRACE,             // {
    TOK_RBRACE,             // }
    
    // Keywords (start at 100)
    TOK_PROGRAM = 100,
    TOK_BEGIN,
    TOK_END,
    TOK_VAR,
    TOK_WHILE,
    TOK_DO,
    TOK_PROCEDURE,
    TOK_CALL,
    TOK_CONST,
    TOK_IF,
    TOK_THEN,
    TOK_ODD,
    TOK_ELSE,
    TOK_SHR,
    TOK_SHL,
    TOK_SAR,
    TOK_FOR,
    TOK_TO,
    TOK_DOWNTO,
    TOK_INTEGER,
    TOK_BOOLEAN,
    TOK_CHAR,
    TOK_STRING,
    TOK_OF,
    TOK_ARRAY,
    TOK_WRITELN,
    TOK_CLASS,
    TOK_EXTENDS,
    TOK_NEW,
    TOK_FUNCTION,
    TOK_RETURN,
    TOK_SELF,
    TOK_SQRT,
    TOK_REAL,
    TOK_APP,
    TOK_MODULE,
    TOK_IMPORT,
    TOK_PUBLIC,
    TOK_PRIVATE,
    TOK_PROTECTED,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL
} token_t;

// Lexer state
typedef enum
{
    LS_IDLE = 0,
    LS_INTEGER,
    LS_IDENT,
    LS_STRING,
    LS_LINECOMMENT,
    LS_BLOCKCOMMENT
} lexstate_t;

// Lexer context
typedef struct
{
    char    *src;           // pointer to the source code
    size_t  src_len;        // length of source code
    char    *tokstart;      // pointer to start of current token string
    int64_t toklen;         // length of current token
    token_t token;          // current token type
    lexstate_t state;       // analyser state
    int64_t linenum;        // current line number
    uint64_t number;        // value of integer literal
    char string_quote;      // quote character that started current string
    char string_content[256]; // content of current string literal
    size_t pos;             // current position in source
} lexer_context_t;

// Function prototypes
bool lexer_init(lexer_context_t *context, char *source, size_t source_len);
bool lexer_next(lexer_context_t *context);
void lexer_cleanup(lexer_context_t *context);

// Utility functions
const char* token_to_string(token_t token);
bool is_keyword(const char* str, size_t len);
token_t keyword_to_token(const char* str, size_t len);
bool is_whitespace(char c);
bool is_alpha(char c);
bool is_digit(char c);
bool is_alnum(char c);
