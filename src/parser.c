#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "memory.h"

ScopeNode *newScope(ScopeNode *parent)
{
    ScopeNode *n = (ScopeNode *)malloc(sizeof(ScopeNode));
    n->parentScope = (node *)parent;
    n->n.nodeType = NODE_SCOPE;
    initTable(&n->variables);
    initTable(&n->types);
    initTable(&n->functions);
    initTable(&n->classes);
    initTable(&n->typeAliases);
    initTable(&n->classImpls);
    return n;
}

void freeScope(ScopeNode *node)
{
    freeTable(&node->variables);
    freeTable(&node->types);
    freeTable(&node->functions);
    freeTable(&node->classes);
    freeTable(&node->typeAliases);
    freeTable(&node->classImpls);
    free(node);
}

bool typecmp(Ty* a, Ty* b)
{
    if(a->typeCapacity == b->typeCapacity)
    {
        for(size_t i = 0; i < a->typeCapacity; i++)
        {
            if(strncmp(a->type[i].start, b->type[i].start, a->type[i].length) != 0)
            {
                return false;
            }
            return true;
        }
    }
    return false;
}

typedef enum
{
    PRECEDENCE_NONE,
    PRECEDENCE_APPLY,
    PRECEDENCE_ASSIGNMENT,
    PRECEDENCE_BITWISE_OR,
    PRECEDENCE_BITWISE_AND,
    PRECEDENCE_LOGICAL_OR,
    PRECEDENCE_LOGICAL_AND,
    PRECEDENCE_EQUALITY,
    PRECEDENCE_COMPARISON,
    PRECEDENCE_SHIFT,
    PRECEDENCE_ADD,
    PRECEDENCE_MULTIPLY,
    PRECEDENCE_PREFIX,
    PRECEDENCE_POSTFIX,
    PRECEDENCE_PRIMARY
} Precedence;

typedef struct
{
    node *(*prefix)(Parser *parser);
    node *(*infix)(Parser *parser, node *n); // fix to take in a node*
    Precedence precedence;
} ParseRule;

node *expression(Parser *parser);

node *grouping(Parser *parser);
node *arrayIndex(Parser *parser, node *n);
node *unary(Parser *parser);
node *binary(Parser *parser, node *n);
node *assignment(Parser *parser, node *n);
node *literal(Parser *parser);
node *identifier(Parser *parser);
node *field_call(Parser *parser, node *n);
node *array_constructor(Parser *parser);
node *array_index(Parser *parser, node *n);
node *function_call(Parser *parser, node *n);

ParseRule rules[] = {
    [PAREN] = {grouping, function_call, PRECEDENCE_POSTFIX},
    [CLOSE_PAREN] = {NULL, NULL, PRECEDENCE_NONE},
    [BRACE] = {NULL, NULL, PRECEDENCE_NONE},
    [CLOSE_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
    [BRACKET] = {array_constructor, array_index, PRECEDENCE_POSTFIX},
    [CLOSE_BRACKET] = {NULL, NULL, PRECEDENCE_NONE},
    [COMMA] = {NULL, NULL, PRECEDENCE_NONE},
    [DOT] = {NULL, field_call, PRECEDENCE_POSTFIX},
    [MINUS] = {unary, binary, PRECEDENCE_ADD},
    [PLUS] = {NULL, binary, PRECEDENCE_ADD},
    [AMPERSAND] = {unary, binary, PRECEDENCE_BITWISE_AND},
    [ARROW] = {NULL, field_call, PRECEDENCE_POSTFIX},
    [LEFT_ARROW] = {NULL, NULL, PRECEDENCE_NONE},
    [EQUAL] = {NULL, assignment, PRECEDENCE_ASSIGNMENT},
    [SEMICOLON] = {NULL, NULL, PRECEDENCE_NONE},
    [SLASH] = {NULL, binary, PRECEDENCE_MULTIPLY},
    [STAR] = {unary, binary, PRECEDENCE_MULTIPLY},
    [BANG] = {unary, NULL, PRECEDENCE_PREFIX},
    [BANG_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [EQUAL_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [LESS] = {NULL, binary, PRECEDENCE_COMPARISON},
    [LESS_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [GREATER] = {NULL, binary, PRECEDENCE_COMPARISON},
    [GREATER_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [TYPE] = {NULL, NULL, PRECEDENCE_NONE},
    [IDENTIFIER] = {identifier, NULL, PRECEDENCE_NONE},
    [TYPEDEF] = {NULL, NULL, PRECEDENCE_NONE},
    [SWITCH] = {NULL, NULL, PRECEDENCE_NONE},
    [CASE] = {NULL, NULL, PRECEDENCE_NONE},
    [CLASS] = {NULL, NULL, PRECEDENCE_NONE},
    [IMPLEMENT] = {NULL, NULL, PRECEDENCE_NONE},
    [USING] = {NULL, NULL, PRECEDENCE_NONE},
    [MUTABLE] = {NULL, NULL, PRECEDENCE_NONE},
    [TRUE] = {literal, NULL, PRECEDENCE_NONE},
    [FALSE] = {literal, NULL, PRECEDENCE_NONE},
    [FLOAT] = {literal, NULL, PRECEDENCE_NONE},
    [DOUBLE] = {literal, NULL, PRECEDENCE_NONE},
    [CHAR] = {literal, NULL, PRECEDENCE_NONE},
    [INT] = {literal, NULL, PRECEDENCE_NONE},
    [HEX_INT] = {literal, NULL, PRECEDENCE_NONE},
    [OCT_INT] = {literal, NULL, PRECEDENCE_NONE},
    [BIN_INT] = {literal, NULL, PRECEDENCE_NONE},
    [STRING] = {literal, NULL, PRECEDENCE_NONE},
    [IF] = {NULL, NULL, PRECEDENCE_NONE},
    [WHILE] = {NULL, NULL, PRECEDENCE_NONE},
    [FOR] = {NULL, NULL, PRECEDENCE_NONE},
    [RETURN] = {NULL, NULL, PRECEDENCE_NONE},
    [ELSE] = {NULL, NULL, PRECEDENCE_NONE},
    [AND] = {NULL, binary, PRECEDENCE_LOGICAL_AND},
    [OR] = {NULL, binary, PRECEDENCE_LOGICAL_OR},
    [NOT] = {unary, NULL, PRECEDENCE_PREFIX},
    [BIT_OR] = {NULL, binary, PRECEDENCE_BITWISE_OR},
    [SHIFT_LEFT] = {NULL, binary, PRECEDENCE_SHIFT},
    [SHIFT_RIGHT] = {NULL, binary, PRECEDENCE_SHIFT},
    [BREAK] = {NULL, NULL, PRECEDENCE_NONE},
    [NEWLINE] = {NULL, NULL, PRECEDENCE_NONE},
    [ERROR] = {NULL, NULL, PRECEDENCE_NONE},
    [EOF_] = {NULL, NULL, PRECEDENCE_NONE}};

static void errorAt(Parser *parser, Token *token, const char *msg)
{
    if (parser->panicMode)
        return;
    parser->panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == EOF_)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == ERROR)
    {
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser->hadError = true;
}

static void errorAtNext(Parser *parser, const char *msg)
{
    errorAt(parser, &parser->next, msg);
}

static void errorAtCurrent(Parser *parser, const char *msg)
{
    errorAt(parser, &parser->current, msg);
}

static void error(Parser *parser, const char *msg)
{
    errorAt(parser, &parser->previous, msg);
}

static void advance(Parser *parser)
{
    parser->previous = parser->current;
    parser->current = parser->next;

    while (true)
    {
        parser->next = scanToken(parser->lexer);
        if (parser->next.type != ERROR)
            break;
        errorAtNext(parser, parser->next.start);
    }
}

static void consume(Parser *parser, TokenType type, const char *message)
{
    if (parser->current.type == type)
    {
        advance(parser);
        return;
    }

    errorAtCurrent(parser, message);
}

node *statement(Parser *parser, ScopeNode *scope);

node *block_stmt(Parser *parser, ScopeNode *scope);

node *declaration(Parser *parser, ScopeNode *scope);

Ty *resolve_type(Parser *parser, bool isDeclared)
{
    Ty *type = malloc(sizeof(Ty));
    size_t typeCount = 0;
    type->typeCapacity = GROW_CAPACITY(typeCount);
    type->type = GROW_ARRAY(Token, NULL, 0, type->typeCapacity);
    while (parser->next.type == TYPE || parser->next.type == IDENTIFIER || parser->next.type == BRACKET || parser->next.type == STAR || parser->next.type == ARROW)
    {
        if (typeCount > type->typeCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(type->typeCapacity);
            type->type = GROW_ARRAY(Token, type->type, type->typeCapacity, newCapacity);
            type->typeCapacity = newCapacity;
        }
        type->type[typeCount] = parser->current;
        typeCount++;
        advance(parser);
        while (parser->next.type == ARROW || parser->next.type == CLOSE_BRACKET)
        {
            if (typeCount > type->typeCapacity)
            {
                size_t newCapacity = GROW_CAPACITY(type->typeCapacity);
                type->type = GROW_ARRAY(Token, type->type, type->typeCapacity, newCapacity);
                type->typeCapacity = newCapacity;
            }
            type->type[typeCount] = parser->current;
            typeCount++;
            advance(parser);
        }
    }
    if (isDeclared)
    {
        if (typeCount > type->typeCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(type->typeCapacity);
            type->type = GROW_ARRAY(Token, type->type, type->typeCapacity, newCapacity);
            type->typeCapacity = newCapacity;
        }
        type->type[typeCount] = parser->current;
        typeCount++;
        advance(parser);
    }
    type->type = GROW_ARRAY(Token, type->type, type->typeCapacity, typeCount);
    type->typeCapacity = typeCount;
    return type;
}

static node *var_decl(Parser *parser, Ty *type)
{
    VariableDeclarationNode *result = malloc(sizeof(VariableDeclarationNode));
    result->n.nodeType = NODE_VARIABLEDECL;
    result->type = type;
    result->identifier = parser->current;
    advance(parser);
    result->value = NULL;
    if (parser->current.type == EQUAL)
    {
        advance(parser);
        result->value = expression(parser);
    }
    consume(parser, SEMICOLON, "expected ';' after variable declaration!");
    return (node *)result;
}

static node *func_decl(Parser *parser, Ty *returnType, ScopeNode *scope)
{
    FunctionDeclarationNode *result = malloc(sizeof(FunctionDeclarationNode));
    result->n.nodeType = NODE_FUNCTIONDECL;
    result->returnType = returnType;
    result->identifier = parser->current;
    char* t = tokenToString(result->identifier);
    node *fn = searchTable(&scope->functions, t);
    free(t);
    if (fn != NULL)
    {
        assert(fn->nodeType = NODE_FUNCTIONDECL);
        FunctionDeclarationNode *n = (FunctionDeclarationNode *)fn;
        if (n->body != NULL)
            errorAtCurrent(parser, "redefinition of existing function!");
    }

    advance(parser);
    result->paramCapacity = GROW_CAPACITY(0);
    result->paramCount = 0;
    result->params = GROW_ARRAY(Parameter, NULL, 0, result->paramCapacity);
    consume(parser, PAREN, "expected '(' before parameter list!");
    while (parser->current.type != CLOSE_PAREN)
    {
        if (result->paramCount >= result->paramCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(result->paramCapacity);
            result->params = GROW_ARRAY(Parameter, result->params, result->paramCapacity, newCapacity);
            result->paramCapacity = newCapacity;
        }
        Parameter p;
        p.type = resolve_type(parser, false);
        p.identifier = parser->current;
        advance(parser);
        result->params[result->paramCount] = p;
        result->paramCount++;
        if (parser->current.type == COMMA)
            advance(parser);
    }
    result->params = GROW_ARRAY(Parameter, result->params, result->paramCapacity, result->paramCount);
    result->paramCapacity = result->paramCount;
    consume(parser, CLOSE_PAREN, "expected ')' after parameters!");
    if (parser->current.type == BRACE)
    {
        result->body = block_stmt(parser, scope);
        ScopeNode* s = (ScopeNode*)((BlockStatementNode*)result->body)->scope;
        for(size_t i = 0; i < result->paramCount; i++)
        {
            VariableDeclarationNode* vd = malloc(sizeof(VariableDeclarationNode));
            vd->n.nodeType = NODE_VARIABLEDECL;
            vd->identifier = result->params[i].identifier;
            vd->type = result->params[i].type;
            vd->value = NULL;
            insertTable(&s->variables, tokenToString(result->params[i].identifier), vd); 
        }
    }
    else
    {
        result->body = NULL;
        consume(parser, SEMICOLON, "expected '{' or ';' after function parameters!");
    }

    insertTable(&scope->functions, tokenToString(result->identifier), result);

    return (node *)result;
}

static node *struct_or_union(Parser *parser, ScopeNode *scope)
{
    RecordDeclarationNode *result = malloc(sizeof(RecordDeclarationNode));
    result->n.nodeType = NODE_RECORDDECL;
    result->struct_or_union = UNDEFINED;
    if (parser->previous.type == STRUCT)
    {
        result->struct_or_union = IS_STRUCT;
    }
    else if (parser->previous.type == UNION)
    {
        result->struct_or_union = IS_UNION;
    }
    result->typeDefined = resolve_type(parser, true);

    insertTable(&scope->types, tokenToString(result->typeDefined->type[0]), result);

    consume(parser, BRACE, "expected '{' after type name!");
    size_t fieldCount = 0;
    result->fieldCapacity = GROW_CAPACITY(0);
    result->fields = GROW_ARRAY(Parameter, NULL, 0, result->fieldCapacity);
    while (parser->current.type != CLOSE_BRACE)
    {
        if (fieldCount >= result->fieldCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(result->fieldCapacity);
            result->fields = GROW_ARRAY(Parameter, result->fields, result->fieldCapacity, newCapacity);
            result->fieldCapacity = newCapacity;
        }
        Parameter p;
        p.type = resolve_type(parser, false);
        p.identifier = parser->current;
        advance(parser);
        result->fields[fieldCount] = p;
        fieldCount++;
        consume(parser, SEMICOLON, "expected ';' after field name!");
    }
    result->fields = GROW_ARRAY(Parameter, result->fields, result->fieldCapacity, fieldCount);
    result->fieldCapacity = fieldCount;
    consume(parser, CLOSE_BRACE, "expected '}' after struct or union declaration!");
    return (node *)result;
}

static node *typedef_decl(Parser *parser, ScopeNode *scope)
{
    TypedefNode *result = malloc(sizeof(TypedefNode));
    result->n.nodeType = NODE_TYPEDEF;
    result->typeAliased = resolve_type(parser, false);
    result->typeDefined = resolve_type(parser, true);
    char* t = typeToString(result->typeDefined);
    insertTable(&scope->typeAliases, t, result->typeAliased);

    return (node *)result;
}

static node *class_decl(Parser *parser, ScopeNode *scope)
{
    ClassDeclarationNode *result = malloc(sizeof(ClassDeclarationNode));
    result->n.nodeType = NODE_CLASSDECL;
    advance(parser);
    result->className = resolve_type(parser, true);
    result->functionCapacity = 0;
    result->functions = NULL;
    size_t constraintCount = 0;
    size_t functionCount = 0;

    if (parser->current.type == COLON)
    {
        result->constraintCapacity = GROW_CAPACITY(0);
        result->constraints = GROW_ARRAY(Ty *, NULL, 0, result->constraintCapacity);
        while (parser->current.type != BRACE)
        {
            Ty *constraint = resolve_type(parser, true);
            if (constraintCount >= result->constraintCapacity)
            {
                size_t newCapacity = GROW_CAPACITY(result->constraintCapacity);
                result->constraints = GROW_ARRAY(Ty *, result->constraints, result->constraintCapacity, newCapacity);
                result->constraintCapacity = newCapacity;
            }
            result->constraints[constraintCount] = constraint;
            constraintCount++;
            if (parser->current.type != BRACE)
            {
                consume(parser, COMMA, "expected ',' between class constraints!");
            }
        }
        if (result->constraintCapacity > constraintCount)
        {
            result->constraints = GROW_ARRAY(Ty *, result->constraints, result->constraintCapacity, constraintCount);
            result->constraintCapacity = constraintCount;
        }
    }
    else
    {
        result->constraintCapacity = 0;
        result->constraints = NULL;
    }

    consume(parser, BRACE, "expected '{' after class declaration!");
    result->functionCapacity = GROW_CAPACITY(0);
    result->functions = GROW_ARRAY(node *, NULL, 0, result->functionCapacity);
    while (parser->current.type != CLOSE_BRACE)
    {
        Ty *type = resolve_type(parser, false);
        node *func = func_decl(parser, type, scope);
        if (result->functionCapacity >= functionCount)
        {
            size_t newCapacity = GROW_CAPACITY(result->functionCapacity);
            result->functions = GROW_ARRAY(node *, result->functions, result->functionCapacity, newCapacity);
            result->functionCapacity = newCapacity;
        }

        result->functions[functionCount] = func;
        functionCount++;
    }

    if (result->functionCapacity > functionCount)
    {
        result->functions = GROW_ARRAY(node *, result->functions, result->functionCapacity, functionCount);
        result->functionCapacity = functionCount;
    }

    consume(parser, CLOSE_BRACE, "expected '}' after function declarations!");

    insertTable(&scope->classes, tokenToString(result->className->type[0]), result);

    return (node *)result;
}

node *declaration(Parser *parser, ScopeNode *scope)
{
    switch (parser->current.type)
    {
    case TYPE:
    {
        Ty *type = resolve_type(parser, false);
        switch (parser->next.type)
        {
        case PAREN:
            return func_decl(parser, type, scope);
        default:
            return var_decl(parser, type);
        }
        break;
    }
    case LET:
    {
        advance(parser);
        switch (parser->next.type)
        {
        case PAREN:
            return func_decl(parser, NULL, scope);
        default:
            return var_decl(parser, NULL);
        }
        break;
    }
    case STRUCT:
    case UNION:
        advance(parser);
        return struct_or_union(parser, scope);
    case TYPEDEF:
        return typedef_decl(parser, scope);
    case CLASS:
        return class_decl(parser, scope);
    default:
        return statement(parser, scope);
    }
}

static node *switch_stmt(Parser *parser, ScopeNode *scope)
{
    SwitchStatementNode *result = malloc(sizeof(SwitchStatementNode));
    result->n.nodeType = NODE_SWITCH;
    advance(parser);
    consume(parser, PAREN, "expected '(' after 'switch!'");
    result->switchExpr = expression(parser);
    result->caseCount = 0;
    result->caseCapacity = GROW_CAPACITY(0);
    result->cases = GROW_ARRAY(CaseNode *, NULL, 0, result->caseCapacity);
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    consume(parser, BRACE, "expected '{' at the start of switch block!");

    do
    {
        consume(parser, CASE, "expect 'case' in switch block!");
        if (result->caseCount >= result->caseCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(result->caseCapacity);
            result->cases = GROW_ARRAY(CaseNode *, result->cases, result->caseCapacity, newCapacity);
            result->caseCapacity = newCapacity;
        }
        CaseNode *current = malloc(sizeof(CaseNode));
        current->n.nodeType = NODE_CASE;
        current->caseExpr = expression(parser);
        consume(parser, COLON, "expected ':' after expression!");
        current->caseStmt = statement(parser, scope);
        result->cases[result->caseCount] = current;
        result->caseCount++;
    } while (parser->current.type == CASE);

    consume(parser, CLOSE_BRACE, "expect '}' after switch block!");
    result->cases = GROW_ARRAY(CaseNode *, result->cases, result->caseCapacity, result->caseCount);
    result->caseCapacity = result->caseCount;
    return (node *)result;
}

static node *for_stmt(Parser *parser, ScopeNode *scope)
{
    ForStatementNode *result = malloc(sizeof(ForStatementNode));
    advance(parser);
    result->n.nodeType = NODE_FOR;
    result->initExpr = NULL;
    result->condExpr = NULL;
    result->incrementExpr = NULL;
    result->loopStmt = NULL;
    consume(parser, PAREN, "expected '(' after 'for!'");
    if (parser->current.type != SEMICOLON)
    {
        result->initExpr = expression(parser); // TODO: change to 'declaration or expression'
    }
    consume(parser, SEMICOLON, "expected ';' after declaration/expression!");
    if (parser->current.type != SEMICOLON)
    {
        result->condExpr = expression(parser);
    }
    consume(parser, SEMICOLON, "expected ';' after condition expression!");
    if (parser->current.type != CLOSE_PAREN)
    {
        result->incrementExpr = expression(parser);
    }
    consume(parser, CLOSE_PAREN, "expected ')' after iteration expression!");
    result->loopStmt = statement(parser, scope);
    return (node *)result;
}

static node *if_stmt(Parser *parser, ScopeNode *scope)
{
    IfStatementNode *result = malloc(sizeof(IfStatementNode));
    advance(parser);
    result->n.nodeType = NODE_IF;
    consume(parser, PAREN, "expected '(' after 'if!'");
    result->branchExpr = expression(parser);
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    result->thenStmt = statement(parser, scope);
    if (parser->current.type == ELSE)
    {
        advance(parser);
        result->elseStmt = statement(parser, scope);
    }
    else
    {
        result->elseStmt = NULL;
    }
    return (node *)result;
}

static node *while_stmt(Parser *parser, ScopeNode *scope)
{
    WhileStatementNode *result = malloc(sizeof(WhileStatementNode));
    advance(parser);
    result->n.nodeType = NODE_WHILE;
    consume(parser, PAREN, "expected '(' after 'while!'");
    result->loopExpr = expression(parser);
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    result->loopStmt = statement(parser, scope);
    return (node *)result;
}

node *block_stmt(Parser *parser, ScopeNode *scope)
{

    BlockStatementNode *result = malloc(sizeof(BlockStatementNode));
    advance(parser);
    result->n.nodeType = NODE_BLOCK;
    ScopeNode *next = newScope(scope);
    result->scope = next;
    result->declarationCount = 0;
    result->declarationCapacity = GROW_CAPACITY(0);
    result->declarations = GROW_ARRAY(node *, NULL, 0, result->declarationCapacity);
    while (parser->current.type != CLOSE_BRACE)
    {
        node *current = declaration(parser, scope);
        if (result->declarationCount >= result->declarationCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(result->declarationCapacity);
            result->declarations = GROW_ARRAY(node *, result->declarations, result->declarationCapacity, newCapacity);
            result->declarationCapacity = newCapacity;
        }
        result->declarations[result->declarationCount] = current;
        result->declarationCount++;
    }
    result->declarations = GROW_ARRAY(node *, result->declarations, result->declarationCapacity, result->declarationCount);
    result->declarationCapacity = result->declarationCount;
    consume(parser, CLOSE_BRACE, "expect '}' at end of block!");
    return (node *)result;
}

static node *return_stmt(Parser *parser)
{
    ReturnStatementNode *result = malloc(sizeof(ReturnStatementNode));
    advance(parser);
    result->n.nodeType = NODE_RETURN;
    result->returnExpr = expression(parser);
    consume(parser, SEMICOLON, "expected ';' after return expression!");
    return (node *)result;
}

node *statement(Parser *parser, ScopeNode *scope)
{
    switch (parser->current.type)
    {
        case SWITCH:
            return switch_stmt(parser, scope);
        case FOR:
            return for_stmt(parser, scope);
        case IF:
            return if_stmt(parser, scope);
        case WHILE:
            return while_stmt(parser, scope);
        case BRACE:
            return block_stmt(parser, newScope(scope));
        case RETURN:
            return return_stmt(parser);
        case BREAK:
        {
            advance(parser);
            node *result = malloc(sizeof(node));
            result->nodeType = NODE_BREAK;
            consume(parser, SEMICOLON, "expected ';' after 'break!'");
            return result;
        }
        case CONTINUE:
        {
            advance(parser);
            node *result = malloc(sizeof(node));
            result->nodeType = NODE_CONTINUE;
            consume(parser, SEMICOLON, "expected ';' after 'continue!'");
            return result;
        }
        default:
        {
            if(parser->current.type == SEMICOLON){ advance(parser); return NULL; }
            node *result = expression(parser);
            consume(parser, SEMICOLON, "expected ';' after expression!");
            return result;
        }
    }
}

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

static node *parsePrecedence(Parser *parser, Precedence prec)
{
    advance(parser);
    ParseRule *p = getRule(parser->previous.type);
    if (p->prefix == NULL)
    {
        error(parser, "expected an expression!");
        return NULL;
    }

    node *result = p->prefix(parser);

    while (prec <= getRule(parser->current.type)->precedence)
    {
        advance(parser);
        p = getRule(parser->previous.type);
        result = p->infix(parser, result);
    }
    return result;
}

node *expression(Parser *parser)
{
    return parsePrecedence(parser, PRECEDENCE_ASSIGNMENT);
}

node *literal(Parser *parser)
{
    LiteralNode *n = malloc(sizeof(LiteralNode));
    n->n.nodeType = NODE_LITERAL;
    n->value = parser->previous;
    return (node *)n;
}

node *identifier(Parser *parser)
{
    VariableNode *n = malloc(sizeof(VariableNode));
    n->n.nodeType = NODE_IDENTIFIER;
    n->variable = parser->previous;
    return (node *)n;
}

node *grouping(Parser *parser)
{
    node *result = expression(parser);
    consume(parser, CLOSE_PAREN, "expected a ')' after expression!");
    return result;
}

node *unary(Parser *parser)
{
    UnaryNode *n = malloc(sizeof(UnaryNode));
    n->n.nodeType = NODE_UNARY;
    n->op = parser->previous;
    n->expression = expression(parser);
    return (node *)n;
}

node *binary(Parser *parser, node *expression1)
{
    BinaryNode *n = malloc(sizeof(BinaryNode));
    n->n.nodeType = NODE_BINARY;
    n->expression1 = expression1;
    n->op = parser->previous;
    ParseRule *rule = &rules[parser->previous.type];
    n->expression2 = parsePrecedence(parser, rule->precedence + 1);

    return (node *)n;
}

node *assignment(Parser *parser, node *expression1)
{
    AssignmentNode *n = malloc(sizeof(AssignmentNode));
    n->n.nodeType = NODE_ASSIGNMENT;
    n->variable = expression1;
    n->assignment = expression(parser);
    return (node *)n;
}

node *field_call(Parser *parser, node *expression1)
{
    FieldCallNode *n = malloc(sizeof(FieldCallNode));
    n->n.nodeType = NODE_FIELDCALL;
    n->expr = expression1;
    n->op = parser->previous;
    n->field = parser->current;
    consume(parser, IDENTIFIER, "Expected an identifier!");
    return (node *)n;
}

node *array_constructor(Parser *parser)
{
    ArrayConstructorNode *n = malloc(sizeof(ArrayConstructorNode));
    n->n.nodeType = NODE_ARRAYCONSTRUCTOR;
    n->count = 0;
    n->capacity = GROW_CAPACITY(0);
    n->values = GROW_ARRAY(node *, NULL, 0, n->capacity);
    while (parser->current.type != CLOSE_BRACKET)
    {
        node *value = expression(parser);
        if (n->count >= n->capacity)
        {
            size_t newCapacity = GROW_CAPACITY(n->capacity);
            n->values = GROW_ARRAY(node *, &n->values, n->capacity, newCapacity);
            n->capacity = newCapacity;
        }
        n->values[n->count] = value;
        n->count++;
        if (parser->current.type != CLOSE_BRACKET)
        {
            consume(parser, COMMA, "expected ',' after expression!");
        }
    }
    n->values = GROW_ARRAY(node *, n->values, n->capacity, n->count);
    n->capacity = n->count;
    consume(parser, CLOSE_BRACKET, "expected ']' after expression!");
    return (node *)n;
}

node *function_call(Parser *parser, node *expression1)
{
    FunctionCallNode *n = malloc(sizeof(FunctionCallNode));
    n->n.nodeType = NODE_FUNCTIONCALL;
    n->called = expression1;
    n->argCount = 0;
    n->argCapacity = GROW_CAPACITY(0);
    n->args = GROW_ARRAY(node *, NULL, 0, n->argCapacity);
    while (parser->current.type != CLOSE_PAREN)
    {
        node *arg = expression(parser);
        if (n->argCount >= n->argCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(n->argCapacity);
            n->args = GROW_ARRAY(node *, n->args, n->argCapacity, newCapacity);
            n->argCapacity = newCapacity;
        }
        n->args[n->argCount] = arg;
        n->argCount++;
        if (parser->current.type != CLOSE_PAREN)
        {
            consume(parser, COMMA, "expected ',' after expression!");
        }
    }
    n->args = GROW_ARRAY(node *, n->args, n->argCapacity, n->argCount);
    n->argCapacity = n->argCount;
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    return (node *)n;
}

node *array_index(Parser *parser, node *expression1)
{
    ArrayIndexNode *n = malloc(sizeof(ArrayIndexNode));
    n->n.nodeType = NODE_ARRAYINDEX;
    n->array = expression1;
    n->index = expression(parser);
    consume(parser, CLOSE_BRACKET, "expected ']' after expression!");
    return (node *)n;
}

char *typeToString(Ty *type)
{
    if(type == NULL) return "type unknown";
    size_t length = 0;
    for (size_t i = 0; i < type->typeCapacity; i++)
    {
        switch (type->type[i].type)
        {
        case BRACKET:
        {
            length += type->type[i].length;
            break;
        }
        default:
        {
            length += type->type[i].length + 1;
            break;
        }
        }
    }
    char *str = malloc(length);
    memset(str, ' ', length);
    str[length - 1] = '\0';
    size_t curr = 0;
    for (size_t i = 0; i < type->typeCapacity; i++)
    {
        strncpy(str + curr, type->type[i].start, type->type[i].length);
        switch (type->type[i].type)
        {
        case BRACKET:
        {
            curr += type->type[i].length;
            break;
        }
        default:
        {
            curr += type->type[i].length + 1;
            break;
        }
        }
    }
    return str;
}

static void printType(void *type)
{
    if(type == NULL) return;
    char* t = typeToString((Ty *)type);
    printf("%s\n", t);
    free(t);
}

static void printFunctionID(void *v)
{
    FunctionDeclarationNode *n = (FunctionDeclarationNode *)v;
    char* t = typeToString(n->returnType);
    printf("Type: Function Declaration; Name: %.*s, Return Type: %s\n", n->identifier.length, n->identifier.start, t);
    free(t);
}

static void printNodeFromTable(void *v)
{
    node *n = (node *)v;
    printNodes(n, 0);
}

void printNodes(node *start, int depth)
{
    printf(">");
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }
    if(start == NULL) return;
    switch (start->nodeType)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *n = (ProgramNode *)start;
        printf("\n");
        printNodes((node *)n->globalScope, depth + 1);
        for (size_t i = 0; i < n->declarationCapacity; i++)
        {
            printNodes(n->declarations[i], depth + 1);
        }
        break;
    }
    case NODE_SCOPE:
    {
        ScopeNode *n = (ScopeNode *)start;
        applyTable(&n->functions, &printFunctionID);
        applyTable(&n->classes, &printNodeFromTable);
        applyTable(&n->types, &printNodeFromTable);
        applyTable(&n->typeAliases, &printType);
        break;
    }
    case NODE_VARIABLEDECL:
    {
        VariableDeclarationNode *n = (VariableDeclarationNode *)start;
        char* t = typeToString(n->type);
        printf("Type: Variable Declaration; Name: %.*s Type: %s\n", n->identifier.length, n->identifier.start, t);
        free(t);
        printf("Value: \n");
        if (n->value != NULL)
            printNodes(n->value, depth + 1);
        else
            printf("NULL\n");
        break;
    }
    case NODE_FUNCTIONDECL:
    {
        FunctionDeclarationNode *n = (FunctionDeclarationNode *)start;
        char* t = typeToString(n->returnType);
        printf("Type: Function Declaration; Name: %.*s, Return Type: %s\n", n->identifier.length, n->identifier.start, t);
        free(t);
        printf("Parameters: ");
        bool first = true;
        for (size_t i = 0; i < n->paramCapacity; i++)
        {
            if (first)
                first = false;
            else
                printf(", ");
            printf("%s %.*s", typeToString(n->params[i].type), n->params[i].identifier.length, n->params[i].identifier.start);
        }
        printf("\n");
        if (n->body != NULL)
        {
            printf("Body:\n");
            printNodes(n->body, depth + 1);
        }
        break;
        
    }
    case NODE_TYPEDEF:
    {
        TypedefNode *n = (TypedefNode *)start;
        char* a = typeToString(n->typeAliased);
        char* b = typeToString(n->typeDefined);
        printf("Type: Typedef; Aliased Type: %s Defined Type: %s\n", a, b);
        free(a);
        free(b);
        break;
    }
    case NODE_RECORDDECL:
    {
        RecordDeclarationNode *n = (RecordDeclarationNode *)start;
        char* t = typeToString(n->typeDefined);
        printf("Type: %s; Name: %s\n", 
            n->struct_or_union == IS_STRUCT ? "STRUCT" 
            : n->struct_or_union == IS_UNION ? "UNION"
            : "UNDEFINED", t);
        free(t);
        bool first = true;
        printf("Fields: ");
        for (size_t i = 0; i < n->fieldCapacity; i++)
        {
            if (first)
                first = false;
            else
                printf(", ");
            char* t = typeToString(n->fields[i].type);
            printf("%s %.*s", t, n->fields[i].identifier.length, n->fields[i].identifier.start);
            free(t);
        }
        printf("\n");
        break;
    }
    case NODE_CLASSDECL:
    {
        ClassDeclarationNode *n = (ClassDeclarationNode *)start;
        char* t = typeToString(n->className);
        printf("Type: Class; Name: %s\n", typeToString(n->className));
        free(t);
        if (n->constraintCapacity > 0)
            printf("Constraints: ");
        bool first = true;
        for (size_t i = 0; i < n->constraintCapacity; i++)
        {
            if (first)
                first = false;
            else
                printf(", ");
            t =  typeToString(n->constraints[i]);
            printf("%s",t);
            free(t);
        }

        printf("Functions:\n");
        for (size_t i = 0; i < n->functionCapacity; i++)
        {
            printNodes(n->functions[i], depth);
        }
        break;
    }
    case NODE_CONTINUE:
    {
        printf("Type: Continue;\n");
        break;
    }
    case NODE_BREAK:
    {
        printf("Type: Break;\n");
        break;
    }
    case NODE_RETURN:
    {
        ReturnStatementNode *n = (ReturnStatementNode *)start;
        printf("Type: Return;\n");
        printNodes(n->returnExpr, depth + 1);
        break;
    }
    case NODE_SWITCH:
    {
        SwitchStatementNode *n = (SwitchStatementNode *)start;
        printf("Type: Switch;\n");
        for (size_t i = 0; i < n->caseCount; i++)
        {
            printNodes((node *)n->cases[i], depth + 1);
        }
        break;
    }
    case NODE_CASE:
    {
        CaseNode *n = (CaseNode *)start;
        printf("Case:\n");
        printNodes(n->caseExpr, depth + 1);
        printf("Result:\n");
        printNodes(n->caseStmt, depth + 1);
        break;
    }
    case NODE_FOR:
    {
        ForStatementNode *n = (ForStatementNode *)start;
        printf("Type: For Statement;\n");
        if (n->initExpr != NULL)
        {
            printf("Initialization Expression: \n");
            printNodes(n->initExpr, depth + 1);
        }
        if (n->condExpr != NULL)
        {
            printf("Conditional Expression: \n");
            printNodes(n->condExpr, depth + 1);
        }
        if (n->incrementExpr != NULL)
        {
            printf("Increment Expression: \n");
            printNodes(n->incrementExpr, depth + 1);
        }
        printf("Looping Statement: \n");
        printNodes(n->loopStmt, depth + 1);
        break;
    }
    case NODE_IF:
    {
        IfStatementNode *n = (IfStatementNode *)start;
        printf("Type: If Statement;\n");
        printf("Conditional Expression: \n");
        printNodes(n->branchExpr, depth + 1);
        printf("Then branch:\n");
        printNodes(n->thenStmt, depth + 1);
        if (n->elseStmt != NULL)
        {
            printf("Else branch:\n");
            printNodes(n->elseStmt, depth + 1);
        }
        break;
    }
    case NODE_WHILE:
    {
        WhileStatementNode *n = (WhileStatementNode *)start;
        printf("Type: While Statement;\n");
        printf("Loop Condition: \n");
        printNodes(n->loopExpr, depth + 1);
        printf("Loop Statement: \n");
        printNodes(n->loopStmt, depth + 1);
        break;
    }
    case NODE_BLOCK:
    {
        BlockStatementNode *n = (BlockStatementNode *)start;
        printf("Type: Block Statement;\n");
        // printNodes((node*)n->scope, depth + 1);
        for (size_t i = 0; i < n->declarationCount; i++)
        {
            printNodes(n->declarations[i], depth + 1);
        }
        break;
    }
    case NODE_LITERAL:
    {
        LiteralNode *n = (LiteralNode *)start;
        printf("Type: Literal; Value: %.*s.\n", n->value.length, n->value.start);
        break;
    }
    case NODE_IDENTIFIER:
    {
        VariableNode *n = (VariableNode *)start;
        printf("Type: Identifier; Name: %.*s.\n", n->variable.length, n->variable.start);
        break;
    }
    case NODE_UNARY:
    {
        UnaryNode *n = (UnaryNode *)start;
        printf("Type: Unary Operation; Operator: %.*s.\n", n->op.length, n->op.start);
        printNodes(n->expression, depth + 1);
        break;
    }
    case NODE_BINARY:
    {
        BinaryNode *n = (BinaryNode *)start;
        printf("Type: Binary Operation; Operator: %.*s.\n", n->op.length, n->op.start);
        printNodes(n->expression1, depth + 1);
        printNodes(n->expression2, depth + 1);
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *n = (AssignmentNode *)start;
        printf("Type: Assignment;\n");
        printNodes(n->variable, depth + 1);
        printNodes(n->assignment, depth + 1);
        break;
    }
    case NODE_FIELDCALL:
    {
        FieldCallNode *n = (FieldCallNode *)start;
        printf("Type: Field Call;\n");
        printf("Operation Type: %.*s.\n", n->op.length, n->op.start);
        printNodes(n->expr, depth + 1);
        for(int i = 0; i < depth; i++) printf(" ");
        printf("Field: %.*s\n", n->field.length, n->field.start);
        break;
    }
    case NODE_ARRAYCONSTRUCTOR:
    {
        ArrayConstructorNode *n = (ArrayConstructorNode *)start;
        printf("Type: Array Constructor;\n");
        for (size_t i = 0; i < n->count; i++)
        {
            printNodes(n->values[i], depth + 1);
        }
        break;
    }
    case NODE_FUNCTIONCALL:
    {
        FunctionCallNode *n = (FunctionCallNode *)start;
        printf("Type: Function Call;\n");
        printNodes(n->called, depth + 1);
        for (size_t i = 0; i < n->argCount; i++)
        {
            printNodes(n->args[i], depth + 1);
        }
        break;
    }
    case NODE_ARRAYINDEX:
    {
        ArrayIndexNode *n = (ArrayIndexNode *)start;
        printf("Type: Array Index;\n");
        printNodes(n->array, depth + 1);
        printNodes(n->index, depth + 1);
        break;
    }
    default:
    {
        assert(false);
    }
    }
}

ProgramNode *parse(const char *src)
{
    Lexer lexer = initLexer(src);
    Parser parser;
    parser.lexer = &lexer;
    parser.hadError = false;
    parser.panicMode = false;
    advance(&parser);
    advance(&parser);

    ProgramNode *ast = malloc(sizeof(ProgramNode));
    ast->n.nodeType = NODE_PROGRAM;
    ast->globalScope = newScope(NULL);
    size_t declarationCount = 0;
    ast->declarationCapacity = GROW_CAPACITY(0);
    ast->declarations = GROW_ARRAY(node *, NULL, 0, ast->declarationCapacity);
    while (parser.current.type != EOF_)
    {
        if (declarationCount >= ast->declarationCapacity)
        {
            size_t newCapacity = GROW_CAPACITY(ast->declarationCapacity);
            ast->declarations = GROW_ARRAY(node *, ast->declarations, ast->declarationCapacity, newCapacity);
            ast->declarationCapacity = newCapacity;
        }
        node* dec = declaration(&parser, ast->globalScope);
        if(dec != NULL)
        {
            ast->declarations[declarationCount] = dec;
            declarationCount++;
        }
    }
    if (ast->declarationCapacity > declarationCount)
    {
        ast->declarations = GROW_ARRAY(node *, ast->declarations, ast->declarationCapacity, declarationCount);
        ast->declarationCapacity = declarationCount;
    }
    ast->hadError = parser.hadError;
    if (!ast->hadError)
    {
        //printNodes((node *)ast, 0);
    }
    return ast;
}