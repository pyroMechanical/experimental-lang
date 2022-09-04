#ifndef lexer_header
#define lexer_header

#include <string>

namespace pilaf {
	struct Lexer {
		const char* start;
		const char* current;
		int line;
		bool typeInfOnly = false;
	};

	enum class TokenTypes {
		PAREN, CLOSE_PAREN,
		BRACE, CLOSE_BRACE,
		BRACKET, CLOSE_BRACKET,
		COMMA, DOT, EXCLUSIVE_RANGE, INCLUSIVE_RANGE, COLON,
		AMPERSAND, ARROW,
		EQUAL, SEMICOLON, STAR,
		BANG, BANG_EQUAL, EQUAL_EQUAL,
		LESS, LESS_EQUAL,
		GREATER, GREATER_EQUAL,
		TYPE, LET, FN, UNSAFE,
		IDENTIFIER, OPERATOR, UNDERSCORE, TYPEDEF,
		SWITCH, CASE, CLASS,
		IMPLEMENT, USING, LAMBDA,
		MUTABLE, MODULE,
		//Literals
		_TRUE, _FALSE,
		FLOAT, DOUBLE, CHAR,
		INT, HEX_INT, OCT_INT, BIN_INT,
		STRING,
		//Keywords
		IF, WHILE, FOR,
		INFIX, PREFIX, POSTFIX,
		RETURN, ELSE,
		AND, OR, NOT, BIT_OR,
		SHIFT_LEFT, SHIFT_RIGHT,
		BREAK, CONTINUE,
		STRUCT, UNION, PUBLIC,
		NEWLINE, //special token for resolving newline semicolons
		_ERROR, _EOF
	};

	struct Token {
		TokenTypes type;
		const char* start;
		int length;
		int line;
	};

	std::string tokenToString(Token t);

	bool tokencmp(Token a, Token b);

	Lexer initLexer(const char* src);

Token scanToken(Lexer* lexer);
}
#endif