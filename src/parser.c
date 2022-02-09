#include "parser.h"

bool parse(const char* src)
{
    Lexer lexer = initLexer(src);
    int line = -1;
    advance(&lexer);

    consume(EOF_, "expect end of expression.");
}