#include <cassert>
#include <cstring>
#include <cstdlib>
#include "lexer.h"

std::string tokenToString(Token t)
{
	return std::string(t.start, t.length);
}

bool tokencmp(Token a, Token b)
{
    if(a.length == b.length)
    {
        if(strncmp(a.start, b.start, a.length) == 0)
        {
            return true;
        }
    }
    
    return false;
}

Lexer initLexer(const char* src)
{
    Lexer lexer;
    lexer.start = src;
    lexer.current = src;
    lexer.line = 1;
    return lexer;
}

inline static bool isDigit(char c)
{
        return (c >= '0' && c <= '9');
}

inline static bool isOperator(char c)
{
    static const char ops[] = { '~', '!', '@', '#', '$', '%', '^', '&', '*', '-', '+', '=', '|', '[', ']', '<', '>', '?', '/', '\0'};
    for(size_t i = 0; ops[i] != '\0'; i++)
    {
        if(c == ops[i]) return true;
    }
    return false;
}

inline static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}

inline static bool isUppercase(char c)
{
    return (c >= 'A' && c <= 'Z');
}

inline static bool isIdentifier(char c)
{
    return isAlpha(c) || c == '_';
}

inline static bool isAtEnd(Lexer* lexer)
{
    return *lexer->current == '\0';
}

inline static char advance(Lexer* lexer)
{
    lexer->current++;
    return lexer->current[-1];
}

inline static char peek(Lexer* lexer)
{
    return *lexer->current;
}

inline static char peekNext(Lexer* lexer)
{
    if(isAtEnd(lexer)) return '\0';
    return lexer->current[1];
}

inline static bool match(Lexer* lexer, char expected)
{
    if (isAtEnd(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    return true;
}

inline static Token makeToken(Lexer* lexer, TokenType type)
{
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;
    return token;
}

inline static Token errorToken(Lexer* lexer, const char* msg)
{
    Token token;
    token.type = ERROR;
    token.start = msg;
    token.length = (int)strlen(msg);
    token.line = lexer->line;
    return token;
}

inline static void skipWhitespace(Lexer* lexer)
{
    for(;;)
    {
        char c = peek(lexer);
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '\n':
                lexer->line++;
                advance(lexer);
                break;
            case '/':
                if(peekNext(lexer) == '/')
                {
                    while(peek(lexer) != '\n' && !isAtEnd(lexer)) advance(lexer);
                }
                else if(peekNext(lexer) == '*')
                {
                    int nests = 1;
                    while(nests > 0)
                    {
                        if(peek(lexer) == '*' && !isAtEnd(lexer) && peekNext(lexer) == '/') 
                        {
                            nests--;
                            advance(lexer);
                        }
                        else if(peek(lexer) == '/' && !isAtEnd(lexer) && peekNext(lexer) == '*')
                        { 
                            nests++;
                            advance(lexer);
                        }
                        advance(lexer);
                    }
                }
            default: return;
        }
    }
}

inline static TokenType matchKeyword(Lexer* lexer, int start, int length, const char* rest, TokenType type)
{
    if(lexer->current - lexer->start == start + length && memcmp(lexer->start + start, rest, length) == 0)
    {
        return type;
    }

    return IDENTIFIER;
}

inline static TokenType identifierType(Lexer* lexer)
{
    switch(lexer->start[0])
    {
        case 'a': return matchKeyword(lexer, 1, 2, "nd", AND);
        case 'b': return matchKeyword(lexer, 1, 4, "reak", BREAK);
        case 'c':
            if(lexer->current - lexer->start > 1)
            {
                switch(lexer->start[1])
                {
                    case 'a': return matchKeyword(lexer, 2, 2, "se", CASE);
                    case 'l': return matchKeyword(lexer, 2, 3, "ass", CLASS);
                    case 'o': return matchKeyword(lexer, 2, 6, "ntinue", CONTINUE);
                }
            }
            break;
        case 'e': return matchKeyword(lexer, 1, 3, "lse", ELSE);
        case 'f':
             if(lexer->current - lexer->start > 1)
            {
                switch(lexer->start[1])
                {
                    case 'a': return matchKeyword(lexer, 2, 3, "lse", FALSE);
                    case 'o': return matchKeyword(lexer, 2, 1, "r", FOR);
                    //case 'u': return matchKeyword(lexer, 2, 2, "nc", FUNC);
                }
            }
            break;
        case 'i': 
            if(lexer->current - lexer->start > 1)
            {
                switch(lexer->start[1])
                {
                    case 'f': return matchKeyword(lexer, 2, 0, "", IF);
                    case 'm': return matchKeyword(lexer, 2, 7, "plement", IMPLEMENT);
                }
            }
            break;
        case 'l': 
            if(lexer->current - lexer->start > 1)
            {
                switch(lexer->start[1])
                {
                    case 'e': return matchKeyword(lexer, 2, 1, "t", LET);
                    case 'a': return matchKeyword(lexer, 2, 4, "mbda", LAMBDA);
                }
            }
            break;
        case 'm': return matchKeyword(lexer, 1, 6, "utable", MUTABLE);
        case 'n': return matchKeyword(lexer, 1, 2, "ot", NOT);
        case 'o': return matchKeyword(lexer, 1, 1, "r", OR);
        case 'r': return matchKeyword(lexer, 1, 5, "eturn", RETURN);
        case 's':
            if(lexer->current - lexer->start > 1)
            {
                switch(lexer->start[1])
                {
                    case 'w': return matchKeyword(lexer, 2, 4, "itch", SWITCH);
                    case 't': return matchKeyword(lexer, 2, 4, "ruct", STRUCT);
                }
            }
            break;
        case 't': return matchKeyword(lexer, 1, 6, "ypedef", TYPEDEF);
        case 'u':
            if(lexer->current - lexer->start > 1)
            {
                switch(lexer->start[1])
                { 
                    case 's': return matchKeyword(lexer, 2, 3, "ing", USING);
                    case 'n': return matchKeyword(lexer, 2, 3, "ion", UNION);
                }
            }
            break;
        case 'w': return matchKeyword(lexer, 1, 4, "hile", WHILE);
    }

    return IDENTIFIER;
}

inline static Token typeToken(Lexer* lexer)
{
    while(isIdentifier(peek(lexer)) || isDigit(peek(lexer))) advance(lexer);
    return makeToken(lexer, TYPE);
}

inline static Token identifier(Lexer* lexer)
{
    if(*lexer->start == '_')
    {
        const char* start = lexer->start;
        while(peek(lexer) == '_' ||isDigit(peek(lexer))) advance(lexer);
        if(lexer->current == lexer->start + 1 && !isAlpha(*lexer->current)) return makeToken(lexer, UNDERSCORE);
    }
    if(isUppercase(*lexer->start) || (*lexer->start == '_' && isUppercase(lexer->current[0])))
    {
        return typeToken(lexer);
    }
    while(isIdentifier(peek(lexer)) || isDigit(peek(lexer))) advance(lexer);
    return makeToken(lexer, identifierType(lexer)); //todo: replace with switch for kewords
}

inline static Token opName(Lexer* lexer)
{
    if(*lexer->current == '&' || *lexer->current == '*')
    {
        advance(lexer);
        if(*lexer->current != ')') return errorToken(lexer, "'&' and '*' are not valid at the start of custom operators!");
        advance(lexer);
        return makeToken(lexer, IDENTIFIER);
    }
    while(*lexer->current != ')' && isOperator(*lexer->current)) advance(lexer);
    if(*lexer->current != ')') 
    {
        advance(lexer);
        return errorToken(lexer, "missing ')' in operator name!");
    }

    advance(lexer);
    if(strncmp("(:)", lexer->start, 3) == 0 || strncmp("(->)", lexer->start, 4) == 0)
    {
        return errorToken(lexer, "cannot redefine '->'!");
    }
    return makeToken(lexer, IDENTIFIER);
}

inline static Token _operator(Lexer* lexer)
{
    while(isOperator(*lexer->current)) advance(lexer);
    return makeToken(lexer, OPERATOR);
}

inline static Token number(Lexer* lexer)
{
    bool oct = false;
    bool bin = false;
    bool hex = false;
    if(*lexer->start == '0')
    {
        if(peek(lexer) == 'b')
        {
            advance(lexer);
            while(peek(lexer) > '0' && peek(lexer) <= '1') advance(lexer);
        }
        else if(peek(lexer) == 'x')
        {
            advance(lexer);
             while(peek(lexer) > '0' && peek(lexer) <= '9'
             || (peek(lexer) > 'a' && peek(lexer) <= 'f')
             || (peek(lexer) > 'A' && peek(lexer) <= 'F')) advance(lexer);
        }
        else
        {
           while(peek(lexer) > '0' && peek(lexer) <= '7') advance(lexer);
        }
    }
    else
    {
        while (isDigit(peek(lexer))) advance(lexer);
    }

    if(peek(lexer) == '.')
    {
        advance(lexer);
        while(isDigit(peek(lexer))) advance(lexer);
        if(peek(lexer) == 'f')
        {
            if(bin)
            {
                advance(lexer);
                return errorToken(lexer, "binary notation is invalid for float constants!");
            }
            else
            {
                advance(lexer);
                return makeToken(lexer, FLOAT);
            }
        }
        else 
        {
            return makeToken(lexer, DOUBLE);
        }
    }
    else if(peek(lexer) == 'f')
    {
        if(bin)
        {
            advance(lexer);
            return errorToken(lexer, "binary notation is invalid for float constants!");
        }
        else if(hex)
        {
            advance(lexer);
        }
        else
        {
            advance(lexer);
            return makeToken(lexer, FLOAT);
        }
    }
    else
    {
        if(oct)
        {
            const char* endOfNumber = lexer->current;
            lexer->current = lexer->start;
            while(isDigit(peek(lexer))) advance(lexer);
            if(lexer->current != endOfNumber) 
            {
                lexer->current = endOfNumber;
                return errorToken(lexer, "not a valid octal integer!");
            }
            return makeToken(lexer, OCT_INT);
        }
        else if (bin)
        {
            const char* endOfNumber = lexer->current;
            lexer->current = lexer->start;
            while(isDigit(peek(lexer))) advance(lexer);
            if(lexer->current != endOfNumber) 
            {
                lexer->current = endOfNumber;
                return errorToken(lexer, "not a valid binary integer!");
            }
            return makeToken(lexer, BIN_INT);
        }
        else if (hex)
        {
            return makeToken(lexer, HEX_INT);
        }
        else
        {
            return makeToken(lexer, INT);
        }
    }
}
inline static Token character(Lexer* lexer)
{
    return makeToken(lexer, CHAR);
}
inline static Token string(Lexer* lexer)
{
    while(peek(lexer) != '"' && !isAtEnd(lexer))
    {
        if (peek(lexer) == '\n') lexer->line++;
        advance(lexer);
    }

    if (isAtEnd(lexer)) return errorToken(lexer, "unterminated string!");

    advance(lexer);
    return makeToken(lexer, STRING);
}

Token scanToken(Lexer* lexer)
{
    skipWhitespace(lexer);
    lexer->start = lexer->current;
    if(isAtEnd(lexer)) return makeToken(lexer, EOF_);
    char c = advance(lexer);
    if(isIdentifier(c)) return identifier(lexer);
    if(isDigit(c)) return number(lexer);

    switch(c)
    {
        case '(': return isOperator(*lexer->current) ? opName(lexer) : makeToken(lexer, PAREN);
        case ')': return makeToken(lexer, CLOSE_PAREN);
        case '{': return makeToken(lexer, BRACE);
        case '}': return makeToken(lexer, CLOSE_BRACE);
        case '[': return makeToken(lexer, BRACKET);
        case ']': return makeToken(lexer, CLOSE_BRACKET);
        case ';': return makeToken(lexer, SEMICOLON);
        case ',': return makeToken(lexer, COMMA);
        case '.': return makeToken(lexer, DOT);
        case ':': return makeToken(lexer, COLON);
        case '\'': return 
        lexer->typeInfOnly ?
         identifier(lexer) :
          character(lexer);
        case '*': return makeToken(lexer, STAR);
        case '&': return makeToken(lexer, AMPERSAND);
        case '=': return isOperator(*lexer->current) ? _operator(lexer) : makeToken(lexer,EQUAL);

        case '-':
        case '+':
        case '/':
        case '|':
        case '!':
        case '<':
        case '>':
        case '^':
        case '@':
        case '#':
        case '$':
        case '%': return _operator(lexer);
                
        case '"': return string(lexer);
        case '\n': 
            Token result = makeToken(lexer, NEWLINE);
            lexer->line++;
            return result;
    }
}