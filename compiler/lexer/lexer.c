/*
 * ARX Lexer Implementation
 * Tokenizes ARX source code into tokens
 */

#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

// Keyword table
typedef struct {
    const char* keyword;
    token_t token;
} keyword_entry_t;

static const keyword_entry_t keywords[] = {
    {"program", TOK_PROGRAM},
    {"begin", TOK_BEGIN},
    {"end", TOK_END},
    {"var", TOK_VAR},
    {"while", TOK_WHILE},
    {"do", TOK_DO},
    {"procedure", TOK_PROCEDURE},
    {"call", TOK_CALL},
    {"const", TOK_CONST},
    {"if", TOK_IF},
    {"then", TOK_THEN},
    {"odd", TOK_ODD},
    {"else", TOK_ELSE},
    {"elseif", TOK_ELSEIF},
    {"shr", TOK_SHR},
    {"shl", TOK_SHL},
    {"sar", TOK_SAR},
    {"for", TOK_FOR},
    {"to", TOK_TO},
    {"downto", TOK_DOWNTO},
    {"integer", TOK_INTEGER},
    {"boolean", TOK_BOOLEAN},
    {"char", TOK_CHAR},
    {"string", TOK_STRING},
    {"of", TOK_OF},
    {"array", TOK_ARRAY},
    // writeln is now a procedure of the virtual 'system' object
    {"class", TOK_CLASS},
    {"extends", TOK_EXTENDS},
    {"new", TOK_NEW},
    {"function", TOK_FUNCTION},
    {"return", TOK_RETURN},
    {"self", TOK_SELF},
    {"sqrt", TOK_SQRT},
    {"real", TOK_REAL},
    {"app", TOK_APP},
    {"module", TOK_MODULE},
    {"import", TOK_IMPORT},
    {"public", TOK_PUBLIC},
    {"private", TOK_PRIVATE},
    {"protected", TOK_PROTECTED},
    {"true", TOK_TRUE},
    {"false", TOK_FALSE},
    {"null", TOK_NULL},
    {NULL, TOK_NONE}
};

bool lexer_init(lexer_context_t *context, char *source, size_t source_len)
{
    if (context == NULL || source == NULL) {
        printf("DEBUG: lexer_init failed - context=%p, source=%p\n", context, source);
        return false;
    }
    
    context->src = source;
    context->src_len = source_len;
    context->tokstart = source;
    context->toklen = 0;
    context->token = TOK_NONE;
    context->state = LS_IDLE;
    context->linenum = 1;
    context->number = 0;
    context->string_quote = 0;
    context->string_content[0] = '\0';
    context->pos = 0;
    
    if (debug_mode) {
        printf("Lexer initialized with %zu bytes of source\n", source_len);
    }
    
    return true;
}

bool lexer_next(lexer_context_t *context)
{
    if (context == NULL || context->src == NULL) {
        return false;
    }
    
    // Skip whitespace and comments
    while (context->pos < context->src_len) {
        char c = context->src[context->pos];
        
        if (is_whitespace(c)) {
            if (c == '\n') {
                context->linenum++;
            }
            context->pos++;
            continue;
        }
        
        // Handle line comments
        if (c == '/' && context->pos + 1 < context->src_len && 
            context->src[context->pos + 1] == '/') {
            context->pos += 2;
            while (context->pos < context->src_len && context->src[context->pos] != '\n') {
                context->pos++;
            }
            continue;
        }
        
        // Handle block comments
        if (c == '/' && context->pos + 1 < context->src_len && 
            context->src[context->pos + 1] == '*') {
            context->pos += 2;
            while (context->pos + 1 < context->src_len) {
                if (context->src[context->pos] == '*' && context->src[context->pos + 1] == '/') {
                    context->pos += 2;
                    break;
                }
                if (context->src[context->pos] == '\n') {
                    context->linenum++;
                }
                context->pos++;
            }
            continue;
        }
        
        break;
    }
    
    // Check for EOF
    if (context->pos >= context->src_len) {
        context->token = TOK_EOF;
        return true;
    }
    
    // Set token start
    context->tokstart = &context->src[context->pos];
    context->toklen = 0;
    
    char c = context->src[context->pos];
    
    // Handle identifiers and keywords
    if (is_alpha(c) || c == '_') {
        while (context->pos < context->src_len && 
               (is_alnum(context->src[context->pos]) || context->src[context->pos] == '_')) {
            context->pos++;
            context->toklen++;
        }
        
        // Check if it's a keyword
        context->token = keyword_to_token(context->tokstart, context->toklen);
        if (context->token == TOK_NONE) {
            context->token = TOK_IDENT;
        }
        
        if (debug_mode) {
            printf("Token: %s (%.*s)\n", token_to_string(context->token), 
                   (int)context->toklen, context->tokstart);
        }
        
        return true;
    }
    
    // Handle numbers
    if (is_digit(c)) {
        context->number = 0;
        while (context->pos < context->src_len && is_digit(context->src[context->pos])) {
            context->number = context->number * 10 + (context->src[context->pos] - '0');
            context->pos++;
            context->toklen++;
        }
        context->token = TOK_NUMBER;
        
        if (debug_mode) {
            printf("Token: NUMBER (%llu)\n", (unsigned long long)context->number);
        }
        
        return true;
    }
    
    // Handle string literals
    if (c == '"' || c == '\'') {
        context->string_quote = c;
        context->pos++; // Skip opening quote
        context->toklen = 0;
        
        while (context->pos < context->src_len && context->src[context->pos] != c) {
            if (context->src[context->pos] == '\n') {
                context->linenum++;
            }
            if (context->toklen < sizeof(context->string_content) - 1) {
                context->string_content[context->toklen] = context->src[context->pos];
                context->toklen++;
            }
            context->pos++;
        }
        
        if (debug_mode) {
            printf("Lexer: Found closing quote at position %zu, character: '%c'\n", 
                   context->pos, context->pos < context->src_len ? context->src[context->pos] : '?');
        }
        
        if (context->pos < context->src_len) {
            context->pos++; // Skip closing quote
        }
        
        context->string_content[context->toklen] = '\0';
        context->token = TOK_STRING;
        
        // Set tokstart and toklen for string literals
        context->tokstart = context->string_content;
        context->toklen = strlen(context->string_content);
        
        if (debug_mode) {
            printf("Token: STRING (\"%s\")\n", context->string_content);
        }
        
        return true;
    }
    
    // Handle operators and punctuation
    switch (c) {
        case ';':
            context->token = TOK_SEMICOL;
            context->pos++;
            context->toklen = 1;
            break;
            
        case ':':
            if (context->pos + 1 < context->src_len && context->src[context->pos + 1] == '=') {
                context->token = TOK_ASSIGN;
                context->pos += 2;
                context->toklen = 2;
            } else {
                context->token = TOK_COLON;
                context->pos++;
                context->toklen = 1;
            }
            break;
            
        case '(':
            context->token = TOK_LPAREN;
            context->pos++;
            context->toklen = 1;
            break;
            
        case ')':
            context->token = TOK_RPAREN;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '*':
            context->token = TOK_STAR;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '+':
            context->token = TOK_PLUS;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '-':
            context->token = TOK_MINUS;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '=':
            if (context->pos + 1 < context->src_len && context->src[context->pos + 1] == '=') {
                context->token = TOK_EQUAL;  // == operator
                context->pos += 2;
                context->toklen = 2;
            } else {
                context->token = TOK_ASSIGN;  // = assignment
                context->pos++;
                context->toklen = 1;
            }
            break;
            
        case '.':
            context->token = TOK_PERIOD;
            context->pos++;
            context->toklen = 1;
            break;
            
        case ',':
            context->token = TOK_COMMA;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '!':
            if (context->pos + 1 < context->src_len && context->src[context->pos + 1] == '=') {
                context->token = TOK_NEQ;
                context->pos += 2;
                context->toklen = 2;
            } else {
                context->token = TOK_EXCLAMATION;
                context->pos++;
                context->toklen = 1;
            }
            break;
            
        case '&':
            if (context->pos + 1 < context->src_len && context->src[context->pos + 1] == '&') {
                context->token = TOK_AND;  // && operator
                context->pos += 2;
                context->toklen = 2;
            } else {
                // Single & not supported, treat as error
                context->token = TOK_NONE;
                context->pos++;
                context->toklen = 1;
            }
            break;
            
        case '|':
            if (context->pos + 1 < context->src_len && context->src[context->pos + 1] == '|') {
                context->token = TOK_OR;   // || operator
                context->pos += 2;
                context->toklen = 2;
            } else {
                // Single | not supported, treat as error
                context->token = TOK_NONE;
                context->pos++;
                context->toklen = 1;
            }
            break;
            
        case '<':
            if (context->pos + 1 < context->src_len && context->src[context->pos + 1] == '=') {
                context->token = TOK_LEQ;  // <= operator
                context->pos += 2;
                context->toklen = 2;
            } else {
                context->token = TOK_LESS;  // < operator
                context->pos++;
                context->toklen = 1;
            }
            break;
            
        case '>':
            if (context->pos + 1 < context->src_len && context->src[context->pos + 1] == '=') {
                context->token = TOK_GEQ;  // >= operator
                context->pos += 2;
                context->toklen = 2;
            } else {
                context->token = TOK_GREATER;  // > operator
                context->pos++;
                context->toklen = 1;
            }
            break;
            
        case '?':
            context->token = TOK_QUESTION;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '/':
            context->token = TOK_SLASH;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '^':
            context->token = TOK_CARET;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '%':
            context->token = TOK_PERCENT;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '#':
            context->token = TOK_HASH;
            context->pos++;
            context->toklen = 1;
            break;
            
            
        case '[':
            context->token = TOK_LBRACKET;
            context->pos++;
            context->toklen = 1;
            break;
            
        case ']':
            context->token = TOK_RBRACKET;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '{':
            context->token = TOK_LBRACE;
            context->pos++;
            context->toklen = 1;
            break;
            
        case '}':
            context->token = TOK_RBRACE;
            context->pos++;
            context->toklen = 1;
            break;
            
        default:
            if (debug_mode) {
                printf("Error: Unknown character '%c' at line %lld\n", c, (long long)context->linenum);
            }
            context->token = TOK_NONE;
            context->pos++;
            context->toklen = 1;
            return false;
    }
    
    if (debug_mode) {
        printf("Token: %s\n", token_to_string(context->token));
    }
    
    return true;
}

void lexer_cleanup(lexer_context_t *context)
{
    if (context != NULL) {
        // Nothing to clean up for now
        memset(context, 0, sizeof(lexer_context_t));
    }
}

const char* token_to_string(token_t token)
{
    switch (token) {
        case TOK_NONE: return "NONE";
        case TOK_EOF: return "EOF";
        case TOK_IDENT: return "IDENT";
        case TOK_NUMBER: return "NUMBER";
        case TOK_SEMICOL: return "SEMICOL";
        case TOK_COLON: return "COLON";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_STAR: return "STAR";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_EQUAL: return "EQUAL";
        case TOK_NEQ: return "NEQ";
        case TOK_AND: return "AND";
        case TOK_OR: return "OR";
        case TOK_PERIOD: return "PERIOD";
        case TOK_COMMA: return "COMMA";
        case TOK_EXCLAMATION: return "EXCLAMATION";
        case TOK_QUESTION: return "QUESTION";
        case TOK_ASSIGN: return "ASSIGN";
        case TOK_EOL: return "EOL";
        case TOK_LEQ: return "LEQ";
        case TOK_GEQ: return "GEQ";
        case TOK_LESS: return "LESS";
        case TOK_GREATER: return "GREATER";
        case TOK_SLASH: return "SLASH";
        case TOK_CARET: return "CARET";
        case TOK_PERCENT: return "PERCENT";
        case TOK_HASH: return "HASH";
        case TOK_LBRACKET: return "LBRACKET";
        case TOK_RBRACKET: return "RBRACKET";
        case TOK_DOUBLEPERIOD: return "DOUBLEPERIOD";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_PROGRAM: return "PROGRAM";
        case TOK_BEGIN: return "BEGIN";
        case TOK_END: return "END";
        case TOK_VAR: return "VAR";
        case TOK_WHILE: return "WHILE";
        case TOK_DO: return "DO";
        case TOK_PROCEDURE: return "PROCEDURE";
        case TOK_CALL: return "CALL";
        case TOK_CONST: return "CONST";
        case TOK_IF: return "IF";
        case TOK_THEN: return "THEN";
        case TOK_ODD: return "ODD";
        case TOK_ELSE: return "ELSE";
        case TOK_SHR: return "SHR";
        case TOK_SHL: return "SHL";
        case TOK_SAR: return "SAR";
        case TOK_FOR: return "FOR";
        case TOK_TO: return "TO";
        case TOK_DOWNTO: return "DOWNTO";
        case TOK_INTEGER: return "INTEGER";
        case TOK_BOOLEAN: return "BOOLEAN";
        case TOK_CHAR: return "CHAR";
        case TOK_STRING: return "STRING";
        case TOK_OF: return "OF";
        case TOK_ARRAY: return "ARRAY";
        // TOK_WRITELN removed - writeln is now accessed via system.writeln()
        case TOK_CLASS: return "CLASS";
        case TOK_EXTENDS: return "EXTENDS";
        case TOK_NEW: return "NEW";
        case TOK_FUNCTION: return "FUNCTION";
        case TOK_RETURN: return "RETURN";
        case TOK_SELF: return "SELF";
        case TOK_SQRT: return "SQRT";
        case TOK_REAL: return "REAL";
        case TOK_APP: return "APP";
        case TOK_MODULE: return "MODULE";
        case TOK_IMPORT: return "IMPORT";
        case TOK_PUBLIC: return "PUBLIC";
        case TOK_PRIVATE: return "PRIVATE";
        case TOK_PROTECTED: return "PROTECTED";
        case TOK_TRUE: return "TRUE";
        case TOK_FALSE: return "FALSE";
        case TOK_NULL: return "NULL";
        default: return "UNKNOWN";
    }
}

bool is_keyword(const char* str, size_t len)
{
    return keyword_to_token(str, len) != TOK_NONE;
}

token_t keyword_to_token(const char* str, size_t len)
{
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strlen(keywords[i].keyword) == len && 
            strncmp(keywords[i].keyword, str, len) == 0) {
            return keywords[i].token;
        }
    }
    return TOK_NONE;
}

bool is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_alnum(char c)
{
    return is_alpha(c) || is_digit(c);
}
