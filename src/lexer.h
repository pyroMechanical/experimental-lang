#ifndef lexer_header
#define lexer_header

#include "core.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

typedef enum {
    PAREN, CLOSE_PAREN,
	BRACE, CLOSE_BRACE,
	BRACKET, CLOSE_BRACKET,
	COMMA, DOT, COLON, MINUS, PLUS,
	AMPERSAND, LEFT_ARROW, ARROW,
	EQUAL, SEMICOLON, SLASH, STAR,
	BANG, BANG_EQUAL, EQUAL_EQUAL,
	LESS, LESS_EQUAL,
	GREATER, GREATER_EQUAL,
	TYPE,
	IDENTIFIER, TYPEDEF,
	SWITCH, CASE, CLASS,
	IMPLEMENT, USING,
	MUTABLE,
	//Literals
	TRUE, FALSE,
	FLOAT, DOUBLE, CHAR,
    INT, HEX_INT, OCT_INT, BIN_INT,
    STRING,
	//Keywords
	IF, WHILE, FOR,
	RETURN, ELSE,
	AND, OR, NOT, BIT_OR,
	SHIFT_LEFT, SHIFT_RIGHT,
	BREAK, CONTINUE,
	STRUCT, UNION,
	NEWLINE, //special token for resolving newline semicolons
	ERROR, EOF_
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

Lexer initLexer(const char* src);

Token scanToken(Lexer* lexer);

#endif