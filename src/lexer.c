#include "lexer.h"

Lexer initLexer(const char* src)
{
    Lexer lexer;
    lexer.start = src;
    lexer.current = src;
    lexer.line = 1;
    return lexer;
}

inline static bool isDigit(char c, bool isBinary, bool isOctal, bool isHexadecimal)
{
    if(isBinary)
    {
        return (c >= '0' &&  c <= '1');
    }
    else if(isHexadecimal)
    {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    }
    else if(isOctal)
    {
        return (c >= '0' && c <= '7');
    }
    else
    {
        return (c >= '0' && c <= '9');
    }
}

inline static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
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
        case 'n': return matchKeyword(lexer, 1, 2, "ot", NOT);
        case 'o': return matchKeyword(lexer, 1, 1, "r", OR);
        case 'r': return matchKeyword(lexer, 1, 5, "eturn", RETURN);
        case 's': return matchKeyword(lexer, 1, 5, "witch", SWITCH);
        case 't': return matchKeyword(lexer, 1, 3, "ype", TYPE);
        case 'u': return matchKeyword(lexer, 1, 4, "sing", USING);
        case 'w': return matchKeyword(lexer, 1, 4, "hile", WHILE);
    }

    return IDENTIFIER;
}

inline static Token identifier(Lexer* lexer)
{
    while(isIdentifier(peek(lexer)) || isDigit(peek(lexer), false, false, false)) advance(lexer);
    return makeToken(lexer, identifierType(lexer)); //todo: replace with switch for kewords
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
            bin = true;
            advance(lexer);
        }
        else if(peek(lexer) == 'x')
        {
            hex = true;
            advance(lexer);
        }
        else
        {
            oct = true;
        }
    }
    while (isDigit(peek(lexer), false, false, hex)) advance(lexer);

    if(peek(lexer) == '.')
    {
        if(bin || hex)
        {
            advance(lexer);
            return errorToken(lexer, bin ? "binary notation is invalid for float constants!" :
                 "hex notation is invalid for float constants!");
        }
        advance(lexer);
        while(isDigit(peek(lexer), false, false, false)) advance(lexer);
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
            char* endOfNumber = lexer->current;
            lexer->current = lexer->start;
            while(isDigit(peek(lexer), false, oct, false)) advance(lexer);
            if(lexer->current != endOfNumber) 
            {
                lexer->current = endOfNumber;
                return errorToken(lexer, "not a valid octal integer!");
            }
            return makeToken(lexer, OCT_INT);
        }
        else if (bin)
        {
            char* endOfNumber = lexer->current;
            lexer->current = lexer->start;
            while(isDigit(peek(lexer), bin, false, false)) advance(lexer);
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
    //skipWhitespace(lexer);
    lexer->start = lexer->current;
    if(isAtEnd(lexer)) return makeToken(lexer, EOF_);
    skipWhitespace(lexer);
    char c = advance(lexer);
    if(isIdentifier(c)) return identifier(lexer);
    if(isDigit(c, false, false, false)) return number(lexer);

    switch(c)
    {
        case '(': return makeToken(lexer, PAREN);
        case ')': return makeToken(lexer, CLOSE_PAREN);
        case '{': return makeToken(lexer, BRACE);
        case '}': return makeToken(lexer, CLOSE_BRACE);
        case '[': return makeToken(lexer, BRACKET);
        case ']': return makeToken(lexer, CLOSE_BRACKET);
        case ';': return makeToken(lexer, SEMICOLON);
        case ',': return makeToken(lexer, COMMA);
        case '.': return makeToken(lexer, DOT);
        case '-': return makeToken(lexer, MINUS);
        case '+': return makeToken(lexer, PLUS);
        case '/': return makeToken(lexer, SLASH);
        case '*': return makeToken(lexer, STAR);
        case '&': return makeToken(lexer, 
                match(lexer, '&') ? AND :
                (match (lexer, '=') ? BIT_AND_EQUAL : AMPERSAND));
        case '|' : return makeToken(lexer,
                match(lexer, '|') ? OR :
                (match(lexer, '=') ? BIT_OR_EQUAL : BIT_OR));
        case '!': // != and !
            return makeToken( lexer,
                match(lexer, '=') ? BANG_EQUAL : BANG);
        case '=': // == and =
            return makeToken( lexer,
                match(lexer, '=') ? EQUAL_EQUAL : EQUAL);
        case '<': // <=, <<, and <
            return makeToken( lexer,
                match(lexer, '=') ? LESS_EQUAL :
                match(lexer, '<') ? 
                (match(lexer, '=') ? SHIFT_LEFT_EQUAL : SHIFT_LEFT) : LESS);
        case '>': // >=, >>, and >
            return makeToken( lexer,
                match(lexer, '=') ? GREATER_EQUAL :
                match(lexer, '>') ?
                (match(lexer, '=') ? SHIFT_RIGHT_EQUAL : SHIFT_RIGHT) : GREATER);
        case '"': return string(lexer);
        case '\n': 
            Token result = makeToken(lexer, NEWLINE);
            lexer->line++;
            return result;
    }
}