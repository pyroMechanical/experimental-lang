#include "parser.h"

void parse(const char* src)
{
    Lexer lexer = initLexer(src);
    int line = -1;
    for(;;)
    {
        Token token = scanToken(&lexer);
        if(token.type == (int)EOF_) 
        {
            break;
        }
        if(token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        }
        else
        {
            printf("    | ");
        }
        
        printf("'%.*s'\n", token.length, token.start);
    }
}