#include <cassert>
#include <cstring>
#include <cstdio>
#include <utility>
#include "parser.h"


std::shared_ptr<ScopeNode> newScope(std::shared_ptr<ScopeNode> parent)
{
    auto n = std::make_shared<ScopeNode>();
    n->parentScope = parent;
    if(parent != nullptr) parent->childScopes.push_back(n);
    n->nodeType = NODE_SCOPE;
    return n;
}

bool typecmp(Ty a, Ty b)
{
    return a.compare(b.c_str()) == 0;
}

Ty KindFromType(const Ty t)
{
    Ty result;
    const char* type = "*";
    const char* arrow = "->";
    Lexer l = initLexer(t.c_str());
    Token next = scanToken(&l);
    while(next.type != EOF)
    {
        if(next.type == IDENTIFIER)
        {
            result.append(type).append(" ").append(arrow).append(" ");
        }
        next = scanToken(&l);
    }
    result.append(type);
    return result;
}

std::shared_ptr<node>expression(Parser *parser, std::shared_ptr<ScopeNode> scope);

std::shared_ptr<node>grouping(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>arrayIndex(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>unary(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>binary(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>assignment(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>literal(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>placeholder(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>list_init(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>lambda(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>identifier(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>field_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>array_constructor(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>array_index(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>function_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);

std::unordered_map<TokenType, ParseRule> rules = {
    {PAREN, {grouping, function_call, PRECEDENCE_POSTFIX, false}},
    {CLOSE_PAREN, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {BRACE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {CLOSE_BRACE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {BRACKET, {array_constructor, array_index, PRECEDENCE_POSTFIX, false}},
    {CLOSE_BRACKET, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {COMMA, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {DOT, {nullptr, field_call, PRECEDENCE_POSTFIX, false}},
    {COLON, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {MINUS, {unary, binary, PRECEDENCE_ADD, false}},
    {PLUS, {nullptr, binary, PRECEDENCE_ADD, false}},
    {AMPERSAND, {unary, binary, PRECEDENCE_BITWISE_AND, false}},
    {LEFT_ARROW, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {ARROW, {nullptr, field_call, PRECEDENCE_POSTFIX, false}},
    {EQUAL, {nullptr, assignment, PRECEDENCE_ASSIGNMENT, false}},
    {SEMICOLON, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {SLASH, {nullptr, binary, PRECEDENCE_MULTIPLY, false}},
    {STAR, {unary, binary, PRECEDENCE_MULTIPLY, false}},
    {BANG, {unary, nullptr, PRECEDENCE_PREFIX, false}},
    {BANG_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
    {EQUAL_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
    {LESS, {nullptr, binary, PRECEDENCE_COMPARISON, false}},
    {LESS_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
    {GREATER, {nullptr, binary, PRECEDENCE_COMPARISON, false}},
    {GREATER_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
    {TYPE, {list_init, nullptr, PRECEDENCE_NONE, false}},
    {LET, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {FUNC, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {TYPEVAR, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {IDENTIFIER, {identifier, nullptr, PRECEDENCE_NONE, false}},
    {OPERATOR, {unary, binary, PRECEDENCE_UNKNOWN, false}}, //add precedence information from preprocessor
    {UNDERSCORE, {placeholder, nullptr, PRECEDENCE_NONE, false}},
    {TYPEDEF, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {SWITCH, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {CASE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {CLASS, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {IMPLEMENT, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {USING, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {LAMBDA, {lambda, nullptr, PRECEDENCE_PREFIX, false}},
    {MUTABLE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {TRUE, {literal, nullptr, PRECEDENCE_NONE, false}},
    {FALSE, {literal, nullptr, PRECEDENCE_NONE, false}},
    {FLOAT, {literal, nullptr, PRECEDENCE_NONE, false}},
    {DOUBLE, {literal, nullptr, PRECEDENCE_NONE, false}},
    {CHAR, {literal, nullptr, PRECEDENCE_NONE, false}},
    {INT, {literal, nullptr, PRECEDENCE_NONE, false}},
    {HEX_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
    {OCT_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
    {BIN_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
    {STRING, {literal, nullptr, PRECEDENCE_NONE, false}},
    {IF, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {WHILE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {FOR, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {RETURN, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {ELSE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {AND, {nullptr, binary, PRECEDENCE_LOGICAL_AND, false}},
    {OR, {nullptr, binary, PRECEDENCE_LOGICAL_OR, false}},
    {NOT, {unary, nullptr, PRECEDENCE_PREFIX, false}},
    {BIT_OR, {nullptr, binary, PRECEDENCE_BITWISE_OR, false}},
    {SHIFT_LEFT, {nullptr, binary, PRECEDENCE_SHIFT, false}},
    {SHIFT_RIGHT, {nullptr, binary, PRECEDENCE_SHIFT, false}},
    {BREAK, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {NEWLINE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {ERROR, {nullptr, nullptr, PRECEDENCE_NONE, false}},
    {EOF_, {nullptr, nullptr, PRECEDENCE_NONE, false}}};

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

std::shared_ptr<node>statement(Parser *parser, std::shared_ptr<ScopeNode>scope);

std::shared_ptr<node>block_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope);

std::shared_ptr<node>declaration(Parser *parser, std::shared_ptr<ScopeNode>scope);

//this function generates strings to represnet arbitrary generic types, 
//using the form "a", "b"... "z", "aa", "ab"...
//returns the shortest unused string, and then generates the next one.
Ty newGenericType()
{
    static std::vector<uint8_t> v = {0};
    bool incremented = false;
    Ty result;
    result.reserve(v.size() + 1);
    result.push_back('\'');
    for(auto i : v)
    {
        result.push_back('a' + i);
    }

    for(auto it = v.rbegin(); it != v.rend(); it++)
    {
        if(*it != 25)
        {
            *it = (*it)+1;
            incremented = true;
            break;
        }
    }
    if (incremented == false)
    {
        std::fill(v.begin(), v.end(), 0);
        v.push_back(0);
    }
    return result;
}

Ty resolve_type_nogeneric(Parser *parser, bool isDeclared)
{
    Ty type;
    Parser state = *parser;
    size_t nested_parens = 0;
    bool first = true;
    bool exit = false;
    while(exit == false)
    {
        switch(parser->current.type)
        {
            case IDENTIFIER:
            case TYPE:
            {
                switch(parser->next.type)
                {
                    case TYPE:
                    case IDENTIFIER:
                    case OPERATOR:
                    case ARROW:
                    case STAR:
                    case BRACKET:
                    {
                        if(first == false) type.append(" ");
                        first = false;
                        type.append(parser->current.start, parser->current.length);
                        advance(parser);
                        break;
                    }
                    case CLOSE_PAREN:
                    {
                        if(nested_parens > 0)
                        {
                            if(first == false) type.append(" ");
                            first = false;
                            type.append(parser->current.start, parser->current.length);
                            advance(parser);
                        }
                        else
                        {
                            exit = true;
                        }
                        break;
                    }
                    case COMMA:
                    {
                        if(nested_parens > 0)
                        {
                            if(first == false) type.append(" ");
                            first = false;
                            type.append(parser->current.start, parser->current.length);
                            advance(parser);
                        }
                        else
                        {
                            exit = true;
                        }
                        break;
                    }
                    default:
                    {
                        exit = true;
                    }
                }
                break;
            }
            case COMMA:
            case ARROW:
            {
                switch(parser->next.type)
                {
                    case TYPE:
                    case IDENTIFIER:
                    case PAREN:
                    {
                        if(first == false) type.append(" ");
                        first = false;
                        type.append(parser->current.start, parser->current.length);
                        advance(parser);
                        break;
                    }
                    default:
                    {
                        exit = true;
                    }
                }
                break;
            }
            case STAR:
            {
                switch(parser->next.type)
                {
                    case TYPE:
                    case IDENTIFIER:
                    case OPERATOR:
                    case ARROW:
                    {
                        if(first == false) type.append(" ");
                        first = false;
                        type.append(parser->current.start, parser->current.length);
                        advance(parser);
                        break;
                    }
                    case CLOSE_PAREN:
                    {
                        if(nested_parens > 0)
                        {
                            if(first == false) type.append(" ");
                            first = false;
                            type.append(parser->current.start, parser->current.length);
                            advance(parser);
                        }
                        else
                        {
                            exit = true;
                        }
                        break;
                    }
                    case COMMA:
                    {
                        if(nested_parens > 0)
                        {
                            if(first == false) type.append(" ");
                            first = false;
                            type.append(parser->current.start, parser->current.length);
                            advance(parser);
                        }
                        else
                        {
                            exit = true;
                        }
                        break;
                    }
                    default:
                    {
                        exit = true;
                    }
                }
                break;
            }
            case BRACKET:
            {
                switch(parser->next.type)
                {
                    case CLOSE_BRACKET:
                    {
                        if(first == false) type.append(" ");
                        first = false;
                        type.append(parser->current.start, parser->current.length);
                        advance(parser);
                        switch(parser->next.type)
                        {
                            case ARROW:
                            {
                                if(first == false) type.append(" ");
                                first = false;
                                type.append(parser->current.start, parser->current.length);
                                advance(parser);
                                break;
                            }
                            case CLOSE_PAREN:
                            {
                                if(nested_parens > 0)
                                {
                                    if(first == false) type.append(" ");
                                    first = false;
                                    type.append(parser->current.start, parser->current.length);
                                    advance(parser);
                                }
                                else
                                {
                                    exit = true;
                                }
                                break;
                            }
                            case COMMA:
                            {
                                if(nested_parens > 0)
                                {
                                    if(first == false) type.append(" ");
                                    first = false;
                                    type.append(parser->current.start, parser->current.length);
                                    advance(parser);
                                }
                                else
                                {
                                    exit = true;
                                }
                                break;
                            }
                            default:
                            {
                                exit = true;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        exit = true;
                    }
                }
                break;
            }
            case PAREN:
            {
                switch(parser->next.type)
                {
                    case PAREN:
                    case TYPE:
                    case IDENTIFIER:
                    {
                        if(first == false) type.append(" ");
                        first = false;
                        type.append(parser->current.start, parser->current.length);
                        advance(parser);
                        nested_parens++;
                        break;
                    }
                    default:
                    {
                        exit = true;
                    }
                }
                break;
            }
            case CLOSE_PAREN:
            {
                switch(parser->next.type)
                {
                    case TYPE:
                    case IDENTIFIER:
                    case OPERATOR:
                    case ARROW:
                    case STAR:
                    case BRACKET:
                    {
                        if(first == false) type.append(" ");
                        first = false;
                        type.append(parser->current.start, parser->current.length);
                        advance(parser);
                        nested_parens--;
                        break;
                    }
                    case CLOSE_PAREN:
                    {
                        if(nested_parens > 0)
                        {
                            if(first == false) type.append(" ");
                            first = false;
                            type.append(parser->current.start, parser->current.length);
                            advance(parser);
                        }
                        else
                        {
                            exit = true;
                        }
                        break;
                    }
                    case COMMA:
                    {
                        if(nested_parens > 0)
                        {
                            if(first == false) type.append(" ");
                            first = false;
                            type.append(parser->current.start, parser->current.length);
                            advance(parser);
                        }
                        else
                        {
                            exit = true;
                        }
                        break;
                    }
                    default:
                    {
                        exit = true;
                    }
                }
                break;
            }
        }
    }
    if (isDeclared && (parser->current.type == TYPE || parser->current.type == IDENTIFIER || parser->current.type == BRACKET || parser->current.type == CLOSE_BRACKET || parser->current.type == STAR || parser->current.type == ARROW || parser->current.type == PAREN || parser->current.type == CLOSE_PAREN))
    {
        if(first == false) type.append(" ");
        type.append(parser->current.start, parser->current.length);
        advance(parser);
    }
    if(nested_parens != 0)
    {
        *parser = state;
        return "";
    }
    return type;
}

Ty resolve_type(Parser *parser, bool isDeclared)
{
    Ty type = resolve_type_nogeneric(parser, isDeclared);
    if(type.size() == 0) return newGenericType();
    return type;
}

static std::shared_ptr<node> var_decl(Parser *parser, Ty type, std::shared_ptr<ScopeNode> scope)
{
    auto result = std::make_shared<VariableDeclarationNode>();
    result->nodeType = NODE_VARIABLEDECL;
    result->type = type;
    result->identifier = parser->current;
    advance(parser);
    result->value = nullptr;
    if (parser->current.type == EQUAL)
    {
        advance(parser);
        result->value = expression(parser, scope);
    }
    consume(parser, SEMICOLON, "expected ';' after variable declaration!");
    return std::static_pointer_cast<node>(result);
}

static void op_decl(Parser* parser, std::shared_ptr<ScopeNode> scope)
{
    ParseRule p { nullptr, nullptr, PRECEDENCE_NONE, false};
    bool isPrefix = false;
    bool isInfix = false;
    bool isPostfix = false;
    while(parser->current.type != IDENTIFIER)
    {
        switch(parser->current.type)
        {
            case PREFIX: isPrefix = true; break;
            case INFIX: isInfix = true; break;
            case POSTFIX: isPostfix = true; break;
            default:
            {
                errorAtCurrent(parser, "invalid operator fixity!");
                return;
            }
        }
        advance(parser);
        if(parser->current.type != IDENTIFIER) consume(parser, COMMA, "expected ',' between fixity types!");
    }

    if(isPrefix) p.prefix = unary;
    if(isInfix) p.infix = binary;
    if(isPostfix) p.isPostfix = true;

    auto op = parser->current;
    advance(parser);
    consume(parser, INT, "expected precedence for operator declaration!");
    p.precedence = std::stoi(tokenToString(parser->previous));
    if(p.precedence > 9) error(parser, "precedence must be between 0 and 9!");
    scope->opRules.emplace(tokenToString(op), p);
    consume(parser, SEMICOLON, "expected ';' after operator declaration!");
}

static std::shared_ptr<node> func_decl(Parser *parser, Ty returnType, std::shared_ptr<ScopeNode>scope, Ty isImplOf)
{
    auto result = std::make_shared<FunctionDeclarationNode>();
    result->nodeType = NODE_FUNCTIONDECL;
    result->returnType = returnType;
    result->identifier = parser->current;
    auto fn = scope->functions.find(tokenToString(result->identifier));
    if (fn != scope->functions.end())
    {
        if (fn->second->body != nullptr) 
        {
            errorAtCurrent(parser, "redefinition of existing function!");
            return nullptr;
        }
    }

    advance(parser);
    consume(parser, PAREN, "expected '(' before parameter list!");
    while (parser->current.type != CLOSE_PAREN)
    {
        Parameter p;
        p.type = resolve_type(parser, false);
        p.identifier = parser->current;
        advance(parser);
        result->params.push_back(p);
        if (parser->current.type == COMMA)
            advance(parser);
    }
    if(result->params.size() == 0)
    {
        Parameter _void;
        _void.type = "Void";
        _void.identifier = {IDENTIFIER, "", 0, result->identifier.line};
        result->params.push_back(_void);
    }
    consume(parser, CLOSE_PAREN, "expected ')' after parameters!");
    if (parser->current.type == BRACE)
    {
        result->body = block_stmt(parser, scope);
        std::shared_ptr<ScopeNode> s = std::static_pointer_cast<BlockStatementNode>(result->body)->scope;
        for(size_t i = 0; i < result->params.size(); i++)
        {
            auto vd = std::make_shared<VariableDeclarationNode>();
            vd->nodeType = NODE_VARIABLEDECL;
            vd->identifier = result->params[i].identifier;
            vd->type = result->params[i].type;
            vd->value = std::make_shared<PlaceholderNode>();
            vd->value->nodeType = NODE_PLACEHOLDER;
            s->variables.insert(std::make_pair(tokenToString(result->params[i].identifier), vd)); 
        }
    }
    else
    {
        result->body = nullptr;
        consume(parser, SEMICOLON, "expected '{' or ';' after function parameters!");
    }

    if(isImplOf.size() == 0) 
    {
        scope->functions.insert(std::make_pair(tokenToString(result->identifier), result));
    }
    else 
    {
        //TODO:
        if(scope->functionImpls.find(tokenToString(result->identifier)) == scope->functionImpls.end())
        {
            scope->functionImpls.insert(std::make_pair(tokenToString(result->identifier), std::unordered_map<std::string, std::shared_ptr<FunctionDeclarationNode>>()));
        }

        auto impls = scope->functionImpls.at(tokenToString(result->identifier));

        if(impls.find(isImplOf) == impls.end())
        {
            impls.insert(std::make_pair(isImplOf, result));
        }
        //error?
    }

    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> struct_or_union(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto result = std::make_shared<RecordDeclarationNode>();
    result->nodeType = NODE_RECORDDECL;
    result->struct_or_union = RecordDeclarationNode::UNDEFINED;
    if (parser->previous.type == STRUCT)
    {
        result->struct_or_union = RecordDeclarationNode::IS_STRUCT;
    }
    else if (parser->previous.type == UNION)
    {
        result->struct_or_union = RecordDeclarationNode::IS_UNION;
    }
    result->typeDefined = resolve_type_nogeneric(parser, true);
    result->kind = "";
    {
        Lexer l = initLexer(result->typeDefined.c_str());

        scope->types.insert(std::make_pair(tokenToString(scanToken(&l)), result));
    }

    consume(parser, BRACE, "expected '{' after type name!");
    while (parser->current.type != CLOSE_BRACE)
    {
        auto ptype = resolve_type_nogeneric(parser, false);
        if(ptype.size() == 0 && result->struct_or_union == RecordDeclarationNode::IS_STRUCT)
        {
            errorAtCurrent(parser, "expected type for struct field!");
        }
        auto pidentifier = parser->current;
        Parameter p {ptype, pidentifier};
        scope->fields.insert(std::make_pair(tokenToString(pidentifier), std::make_pair(result->typeDefined, ptype)));
        advance(parser);
        result->fields.push_back(p);
        consume(parser, SEMICOLON, "expected ';' after field name!");
    }
    consume(parser, CLOSE_BRACE, "expected '}' after struct or union declaration!");
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> typedef_decl(Parser *parser, std::shared_ptr<ScopeNode>scope)
{
    auto result = std::make_shared<TypedefNode>();
    result->nodeType = NODE_TYPEDEF;
    result->typeDefined = resolve_type(parser, true);
    consume(parser, EQUAL, "expected '=' after defined type!");
    result->typeAliased = resolve_type(parser, true);
    if(result->typeDefined.size() == 0) assert(false);
    {
        Lexer l = initLexer(result->typeDefined.c_str());
        scope->typeAliases.insert(std::make_pair(tokenToString(scanToken(&l)), result));
    }
    consume(parser, SEMICOLON, "expected ';' after typedef");
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> class_decl(Parser *parser, std::shared_ptr<ScopeNode>scope)
{
    //TODO: figure out what's wrong with resolve_type
    auto result = std::make_shared<ClassDeclarationNode>();
    result->nodeType = NODE_CLASSDECL;
    advance(parser);
    result->className = resolve_type(parser, true);
    result->kind = ""; //this is calculated during static analysis!
    if (parser->current.type == COLON)
    {
        while (parser->current.type != BRACE)
        {
            auto constraint = resolve_type(parser, true);
            result->constraints.push_back(constraint);
            if (parser->current.type != BRACE)
            {
                consume(parser, COMMA, "expected ',' between class constraints!");
            }
        }
    }

    consume(parser, BRACE, "expected '{' after class declaration!");
    while (parser->current.type != CLOSE_BRACE)
    {
        auto type = resolve_type(parser, false);
        std::shared_ptr<node> func = func_decl(parser, type, scope, "");

        result->functions.push_back(func);
    }

    consume(parser, CLOSE_BRACE, "expected '}' after function declarations!");

    {
        Lexer l = initLexer(result->className.c_str());
        scope->classes.insert(std::make_pair(tokenToString(scanToken(&l)), result));
    }

    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> impl_decl(Parser* parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<ClassImplementationNode>();
    advance(parser);
    n->nodeType = NODE_CLASSIMPL;
    n->_class = parser->current;
    advance(parser);
    n->implemented = resolve_type(parser, true);
    consume(parser, BRACE, "expected '{' before function definitions!");
    while(parser->current.type != CLOSE_BRACE)
    {
        auto type = resolve_type(parser, false);
        auto fd = func_decl(parser, type, scope, n->implemented);
        n->functions.push_back(fd);
    }
    consume(parser, CLOSE_BRACE, "expected '}' after function definitions!");
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node> declaration(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    Parser state = *parser;
    switch (parser->current.type)
    {
    case TYPE:
    case PAREN:
    {
        auto type = resolve_type(parser, false);
        if(type.size() == 0) 
        {
            *parser = state;
            return statement(parser, scope);
        }
        switch (parser->next.type)
        {
        case PAREN:
            return func_decl(parser, type, scope, "");
        case EQUAL:
        case SEMICOLON:
            return var_decl(parser, type, scope);

        default: return statement(parser, scope);
        }
        break;
    }
    case LET:
    {
        advance(parser);
        switch (parser->next.type)
        {
        case PAREN:
            return func_decl(parser, newGenericType(), scope, "");
        default:
            return var_decl(parser, newGenericType(), scope);
        }
        break;
    }
    case STRUCT:
    case UNION:
        advance(parser);
        return struct_or_union(parser, scope);
    case TYPEDEF:
        advance(parser);
        return typedef_decl(parser, scope);
    case CLASS:
        return class_decl(parser, scope);
    case IMPLEMENT:
        return impl_decl(parser, scope);
    case PREFIX:
    case INFIX:
    case POSTFIX:
        op_decl(parser, scope);
        return nullptr;
    default:
        return statement(parser, scope);
    }
}

static std::shared_ptr<node> switch_stmt(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto result = std::make_shared<SwitchStatementNode>();
    result->nodeType = NODE_SWITCH;
    advance(parser);
    consume(parser, PAREN, "expected '(' after 'switch!'");
    result->switchExpr = expression(parser, scope);
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    consume(parser, BRACE, "expected '{' at the start of switch block!");
    do
    {
        consume(parser, CASE, "expect 'case' in switch block!");
        auto current = std::make_shared<CaseNode>();
        current->nodeType = NODE_CASE;
        current->caseExpr = expression(parser, scope);
        consume(parser, COLON, "expected ':' after expression!");
        current->caseStmt = statement(parser, scope);
        result->cases.push_back(current);
    } while (parser->current.type == CASE);

    consume(parser, CLOSE_BRACE, "expect '}' after switch block!");
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> for_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
{
    auto result = std::make_shared<ForStatementNode>();
    advance(parser);
    result->nodeType = NODE_FOR;
    result->initExpr = nullptr;
    result->condExpr = nullptr;
    result->incrementExpr = nullptr;
    result->loopStmt = nullptr;
    consume(parser, PAREN, "expected '(' after 'for!'");
    if (parser->current.type != SEMICOLON)
    {
        result->initExpr = expression(parser, scope); // TODO: change to 'declaration or expression'
    }
    consume(parser, SEMICOLON, "expected ';' after declaration/expression!");
    if (parser->current.type != SEMICOLON)
    {
        result->condExpr = expression(parser, scope);
    }
    consume(parser, SEMICOLON, "expected ';' after condition expression!");
    if (parser->current.type != CLOSE_PAREN)
    {
        result->incrementExpr = expression(parser, scope);
    }
    consume(parser, CLOSE_PAREN, "expected ')' after iteration expression!");
    result->loopStmt = statement(parser, scope);
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> if_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
{
    auto result = std::make_shared<IfStatementNode>();
    advance(parser);
    result->nodeType = NODE_IF;
    consume(parser, PAREN, "expected '(' after 'if!'");
    result->branchExpr = expression(parser, scope);
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    result->thenStmt = statement(parser, scope);
    if (parser->current.type == ELSE)
    {
        advance(parser);
        result->elseStmt = statement(parser, scope);
    }
    else
    {
        result->elseStmt = nullptr;
    }
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> while_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
{
    auto result = std::make_shared<WhileStatementNode>();
    advance(parser);
    result->nodeType = NODE_WHILE;
    consume(parser, PAREN, "expected '(' after 'while!'");
    result->loopExpr = expression(parser, scope);
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    result->loopStmt = statement(parser, scope);
    return std::static_pointer_cast<node>(result);
}

std::shared_ptr<node> block_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
{

    auto result = std::make_shared<BlockStatementNode>();
    advance(parser);
    result->nodeType = NODE_BLOCK;
    std::shared_ptr<ScopeNode>next = newScope(scope);
    result->scope = next;
    while (parser->current.type != CLOSE_BRACE)
    {
        result->declarations.push_back(declaration(parser, next));
    }
    consume(parser, CLOSE_BRACE, "expect '}' at end of block!");
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> return_stmt(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto result = std::make_shared<ReturnStatementNode>();
    advance(parser);
    result->nodeType = NODE_RETURN;
    result->returnExpr = expression(parser, scope);
    consume(parser, SEMICOLON, "expected ';' after return expression!");
    return std::static_pointer_cast<node>(result);
}

std::shared_ptr<node>statement(Parser *parser, std::shared_ptr<ScopeNode>scope)
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
            return return_stmt(parser, scope);
        case BREAK:
        {
            advance(parser);
            std::shared_ptr<node> result = std::make_shared<node>();
            result->nodeType = NODE_BREAK;
            consume(parser, SEMICOLON, "expected ';' after 'break!'");
            return result;
        }
        case CONTINUE:
        {
            advance(parser);
            std::shared_ptr<node> result = std::make_shared<node>();
            result->nodeType = NODE_CONTINUE;
            consume(parser, SEMICOLON, "expected ';' after 'continue!'");
            return result;
        }
        default:
        {
            if(parser->current.type == SEMICOLON){ advance(parser); return nullptr; }
            std::shared_ptr<node>result = expression(parser, scope);
            consume(parser, SEMICOLON, "expected ';' after expression!");
            return result;
        }
    }
}

static ParseRule *getRule(TokenType type)
{
    return &rules.at(type);
}

static uint8_t getOperatorPrecedence(Token op, std::shared_ptr<ScopeNode> scope)
{
    auto opName = std::string{"("}.append(tokenToString(op)).append(")");
    auto s = scope;
    while(s != nullptr)
    {
        if(s->opRules.find(opName) != s->opRules.end())
        {
            return s->opRules.at(opName).precedence;
        }
        s = s->parentScope;
    }
    return 0;
}

static std::shared_ptr<node>parsePrecedence(Parser *parser, uint8_t prec, std::shared_ptr<ScopeNode> scope)
{
    advance(parser);
    ParseRule *p = getRule(parser->previous.type);
    if (p->prefix == nullptr)
    {
        error(parser, "expected an expression!");
        return nullptr;
    }

    std::shared_ptr<node> result = p->prefix(parser, scope);

    while (prec <= (parser->current.type == OPERATOR ? getOperatorPrecedence(parser->current, scope) : getRule(parser->current.type)->precedence))
    {
        advance(parser);
        p = getRule(parser->previous.type);
        if(parser->previous.type == OPERATOR)
        {
            auto s = scope;
            while(s != nullptr)
            {
                auto op = std::string("(").append(tokenToString(parser->previous)).append(")");
                if(s->opRules.find(op) != s->opRules.end())
                {
                    p = &s->opRules.at(op);
                }
                s = s->parentScope;
            }
        }
        result = p->infix(parser, scope, result);
    }
    return result;
}

std::shared_ptr<node>expression(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    return parsePrecedence(parser, PRECEDENCE_ASSIGNMENT, scope);
}

std::shared_ptr<node>literal(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<LiteralNode>();
    n->nodeType = NODE_LITERAL;
    n->value = parser->previous;
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>placeholder(Parser* parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<PlaceholderNode>();
    n->nodeType = NODE_PLACEHOLDER;
    return n;
}

std::shared_ptr<node> list_init(Parser* parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<ListInitNode>();
    n->nodeType = NODE_LISTINIT;
    auto front = tokenToString(parser->previous);
    if(parser->current.type != BRACE)
    {
        auto rest = resolve_type(parser, true);
        front.append(rest);
    }
    n->type = front;
    
    consume(parser, BRACE, "expected '{' before list initialization!");
    while(parser->current.type != CLOSE_BRACE)
    {
        auto temp = expression(parser, scope);
        if(parser->current.type == COLON)
        {
            if(temp->nodeType != NODE_IDENTIFIER) error(parser, "expected an identifier before ':' in list initializer!");
            else
            {
                auto literal = std::static_pointer_cast<VariableNode>(temp)->variable;
                n->fieldNames.push_back(literal);
            }
            advance(parser);
            n->values.push_back(expression(parser, scope));
        }
        else
        {
            n->values.push_back(temp);
        }
        if(parser->current.type == COMMA)
        {
            advance(parser);
        }
    }
    consume(parser, CLOSE_BRACE, "expected '}' after list initialization!");
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>identifier(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<VariableNode>();
    n->nodeType = NODE_IDENTIFIER;
    n->variable = parser->previous;
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>lambda(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<LambdaNode>();
    n->nodeType = NODE_LAMBDA;
    n->returnType = newGenericType();
    consume(parser, PAREN, "expected '(' before lambda arguments!");
    while(parser->current.type != CLOSE_PAREN)
    {
        Parameter p;
        p.type = resolve_type(parser, false);
        p.identifier = parser->current;
        advance(parser);
        n->params.push_back(p);
        if (parser->current.type == COMMA)
            advance(parser);
    }
    consume(parser, CLOSE_PAREN, "expected ')' after lambda arguments!");
    n->body = block_stmt(parser, scope);
    std::shared_ptr<ScopeNode> s = std::static_pointer_cast<BlockStatementNode>(n->body)->scope;
        for(size_t i = 0; i < n->params.size(); i++)
        {
            auto vd = std::make_shared<VariableDeclarationNode>();
            vd->nodeType = NODE_VARIABLEDECL;
            vd->identifier = n->params[i].identifier;
            vd->type = n->params[i].type;
            vd->value = std::make_shared<PlaceholderNode>();
            vd->value->nodeType = NODE_PLACEHOLDER;
            s->variables.insert(std::make_pair(tokenToString(n->params[i].identifier), vd)); 
        }
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>grouping(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    std::shared_ptr<node> expr = expression(parser, scope);
    switch(parser->current.type)
    {
        case CLOSE_PAREN:
            advance(parser);
            return expr;
        case COMMA:
        {
            advance(parser);
            auto tuple = std::make_shared<TupleConstructorNode>();
            tuple->nodeType = NODE_TUPLE;
            tuple->values.push_back(expr);
            while(parser->current.type != CLOSE_PAREN)
            {
                tuple->values.push_back(expression(parser, scope));
                if(parser->current.type != CLOSE_PAREN)
                    consume(parser, COMMA, "expected ',' between tuple values!");
            }
            consume(parser, CLOSE_PAREN, "expected ')' after tuple constructor!");
            return tuple;
        }
    }
    error(parser, "Invalid expression!");
    return nullptr;
}

std::shared_ptr<node>unary(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<UnaryNode>();
    n->nodeType = NODE_UNARY;
    n->op = parser->previous;
    n->expression = expression(parser, scope);
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>binary(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
{
    ParseRule *rule = &rules[parser->previous.type];
    
    if(rule->precedence == PRECEDENCE_UNKNOWN)
    {
        auto s = scope;
        while(s != nullptr)
        {
            auto op = std::string("(").append(tokenToString(parser->previous)).append(")");
            if(s->opRules.find(op) != s->opRules.end())
            {
                rule = &s->opRules.at(op);
            }
            s = s->parentScope;
        }
    }
    if(rule->isPostfix)
    {
        ParseRule *next = &rules[parser->current.type];
        if(next->prefix == nullptr)
        {
            auto n = std::make_shared<UnaryNode>();
            n->nodeType = NODE_UNARY;
            n->expression = expression1;
            n->op = parser->previous;
            return std::static_pointer_cast<node>(n);
        }
    }
    auto n = std::make_shared<BinaryNode>();
    n->nodeType = NODE_BINARY;
    n->expression1 = expression1;
    n->op = parser->previous;
    n->expression2 = parsePrecedence(parser, Precedence(rule->precedence + 1), scope);

    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>assignment(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
{
    auto n = std::make_shared<AssignmentNode>();
    n->nodeType = NODE_ASSIGNMENT;
    n->variable = expression1;
    n->assignment = expression(parser, scope);
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>field_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
{
    auto n = std::make_shared<FieldCallNode>();
    n->nodeType = NODE_FIELDCALL;
    n->expr = expression1;
    n->op = parser->previous;
    n->field = parser->current;
    consume(parser, IDENTIFIER, "Expected an identifier!");
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>array_constructor(Parser *parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<ArrayConstructorNode>();
    n->nodeType = NODE_ARRAYCONSTRUCTOR;
    while (parser->current.type != CLOSE_BRACKET)
    {
        n->values.push_back(expression(parser, scope));
        if (parser->current.type != CLOSE_BRACKET)
        {
            consume(parser, COMMA, "expected ',' after expression!");
        }
    }
    consume(parser, CLOSE_BRACKET, "expected ']' after expression!");
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>function_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
{
    auto n = std::make_shared<FunctionCallNode>();
    n->nodeType = NODE_FUNCTIONCALL;
    n->called = expression1;
    while (parser->current.type != CLOSE_PAREN)
    {
        n->args.push_back(expression(parser, scope));
        if (parser->current.type != CLOSE_PAREN)
        {
            consume(parser, COMMA, "expected ',' after expression!");
        }
    }
    consume(parser, CLOSE_PAREN, "expected ')' after expression!");
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>array_index(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
{
    auto n = std::make_shared<ArrayIndexNode>();
    n->nodeType = NODE_ARRAYINDEX;
    n->array = expression1;
    n->index = expression(parser, scope);
    consume(parser, CLOSE_BRACKET, "expected ']' after expression!");
    return std::static_pointer_cast<node>(n);
}

void printNodes(std::shared_ptr<node> start, int depth)
{
    printf(">");
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }
    if(start == nullptr) return;
    switch (start->nodeType)
    {
        case NODE_PROGRAM:
        {
            auto n = std::static_pointer_cast<ProgramNode>(start);
            printf("\n");
            printNodes(std::static_pointer_cast<node>(n->globalScope), depth + 1);
            for (size_t i = 0; i < n->declarations.size(); i++)
            {
                printNodes(n->declarations[i], depth + 1);
            }
            break;
        }
        case NODE_SCOPE:
        {
            std::shared_ptr<ScopeNode> n = std::static_pointer_cast<ScopeNode>(start);
            //TODO: print each unordered_map's elements
            break;
        }
        case NODE_VARIABLEDECL:
        {
            auto n = std::static_pointer_cast<VariableDeclarationNode>(start);
            printf("Type: Variable Declaration; Name: %.*s Type: %s\n", n->identifier.length, n->identifier.start, n->type.c_str());
            printf("Value: \n");
            if (n->value != nullptr)
                printNodes(n->value, depth + 1);
            else
                printf("nullptr\n");
            break;
        }
        case NODE_FUNCTIONDECL:
        {
            auto n = std::static_pointer_cast<FunctionDeclarationNode>(start);
            printf("Type: Function Declaration; Name: %.*s, Return Type: %s\n", n->identifier.length, n->identifier.start, n->returnType.c_str());
            printf("Parameters: ");
            bool first = true;
            for (size_t i = 0; i < n->params.size(); i++)
            {
                if (first)
                    first = false;
                else
                    printf(", ");
                printf("%s %.*s", n->params[i].type.c_str(), n->params[i].identifier.length, n->params[i].identifier.start);
            }
            printf("\n");
            if (n->body != nullptr)
            {
                printf("Body:\n");
                printNodes(n->body, depth + 1);
            }
            break;
            
        }
        case NODE_TYPEDEF:
        {
            auto n = std::static_pointer_cast<TypedefNode>(start);
            printf("Type: Typedef; Aliased Type: %s Defined Type: %s\n", n->typeAliased.c_str(), n->typeDefined.c_str());
            break;
        }
        case NODE_RECORDDECL:
        {
            auto n = std::static_pointer_cast<RecordDeclarationNode>(start);
            printf("Type: %s; Name: %s\n", 
                n->struct_or_union == RecordDeclarationNode::IS_STRUCT ? "STRUCT" 
                : n->struct_or_union == RecordDeclarationNode::IS_UNION ? "UNION"
                : "UNDEFINED", n->typeDefined.c_str());
            bool first = false;
            printf("Fields: ");
            for (size_t i = 0; i < n->fields.size(); i++)
            {
                if (first)
                    first = false;
                else
                    printf(", ");
                printf("%s %.*s", n->fields[i].type.c_str(), n->fields[i].identifier.length, n->fields[i].identifier.start);
    
            }
            printf("\n");
            break;
        }
        case NODE_CLASSDECL:
        {
            auto n = std::static_pointer_cast<ClassDeclarationNode>(start);
            printf("Type: Class; Name: %s\n", n->className.c_str());
            if (n->constraints.size() > 0)
            {
                printf("Constraints: ");
                bool first = false;
                for (size_t i = 0; i < n->constraints.size(); i++)
                {
                    if (first)
                        first = false;
                    else
                        printf(", ");
                    
                    printf("%s",n->constraints[i].c_str());
                }
            }
            printf("Functions:\n");
            for (size_t i = 0; i < n->functions.size(); i++)
            {
                printNodes(n->functions[i], depth);
            }
            break;
        }
        case NODE_CLASSIMPL:
        {
            auto n = std::static_pointer_cast<ClassImplementationNode>(start);
            printf("Type: Class Implementation; Class Name: %s\n", tokenToString(n->_class).c_str());
            printf("Implemented Type: %s\n", n->implemented.c_str());
            printf("Function Specializations:\n");
            for(auto f : n->functions)
            {
                printNodes(f, depth + 1);
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
            auto n = std::static_pointer_cast<ReturnStatementNode>(start);
            printf("Type: Return;\n");
            printNodes(n->returnExpr, depth + 1);
            break;
        }
        case NODE_SWITCH:
        {
            auto n = std::static_pointer_cast<SwitchStatementNode>(start);
            printf("Type: Switch;\n");
            for (size_t i = 0; i < n->cases.size(); i++)
            {
                printNodes(std::static_pointer_cast<node>(n->cases[i]), depth + 1);
            }
            break;
        }
        case NODE_CASE:
        {
            auto n = std::static_pointer_cast<CaseNode>(start);
            printf("Case:\n");
            printNodes(n->caseExpr, depth + 1);
            printf("Result:\n");
            printNodes(n->caseStmt, depth + 1);
            break;
        }
        case NODE_FOR:
        {
            auto n = std::static_pointer_cast<ForStatementNode>(start);
            printf("Type: For Statement;\n");
            if (n->initExpr != nullptr)
            {
                printf("Initialization Expression: \n");
                printNodes(n->initExpr, depth + 1);
            }
            if (n->condExpr != nullptr)
            {
                printf("Conditional Expression: \n");
                printNodes(n->condExpr, depth + 1);
            }
            if (n->incrementExpr != nullptr)
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
            auto n = std::static_pointer_cast<IfStatementNode>(start);
            printf("Type: If Statement;\n");
            printf("Conditional Expression: \n");
            printNodes(n->branchExpr, depth + 1);
            printf("Then branch:\n");
            printNodes(n->thenStmt, depth + 1);
            if (n->elseStmt != nullptr)
            {
                printf("Else branch:\n");
                printNodes(n->elseStmt, depth + 1);
            }
            break;
        }
        case NODE_WHILE:
        {
            auto n = std::static_pointer_cast<WhileStatementNode>(start);
            printf("Type: While Statement;\n");
            printf("Loop Condition: \n");
            printNodes(n->loopExpr, depth + 1);
            printf("Loop Statement: \n");
            printNodes(n->loopStmt, depth + 1);
            break;
        }
        case NODE_BLOCK:
        {
            auto n = std::static_pointer_cast<BlockStatementNode>(start);
            printf("Type: Block Statement;\n");
            // printNodes((node*)n->scope, depth + 1);
            for (size_t i = 0; i < n->declarations.size(); i++)
            {
                printNodes(n->declarations[i], depth + 1);
            }
            break;
        }
        case NODE_LITERAL:
        {
            auto n = std::static_pointer_cast<LiteralNode>(start);
            printf("Type: Literal; Value: %.*s.\n", n->value.length, n->value.start);
            break;
        }
        case NODE_IDENTIFIER:
        {
            auto n = std::static_pointer_cast<VariableNode>(start);
            printf("Type: Identifier; Name: %.*s.\n", n->variable.length, n->variable.start);
            break;
        }
        case NODE_LISTINIT:
        {
            auto n = std::static_pointer_cast<ListInitNode>(start);
            printf("Type: List Init; Name: %s\n", n->type.c_str());
            printf("Values:\n");
            for(auto v : n->values)
            {
                printNodes(v, depth + 1);
            }
            break;
        }
        case NODE_TUPLE:
        {
            auto n = std::static_pointer_cast<TupleConstructorNode>(start);
            printf("Type: Tuple Constructor\n");
            printf("Values:\n");
            for(auto v : n->values)
            {
                printNodes(v, depth + 1);
            }
            break;
        }
        case NODE_UNARY:
        {
            auto n = std::static_pointer_cast<UnaryNode>(start);
            printf("Type: Unary Operation; Operator: %.*s.\n", n->op.length, n->op.start);
            printNodes(n->expression, depth + 1);
            break;
        }
        case NODE_BINARY:
        {
            auto n = std::static_pointer_cast<BinaryNode>(start);
            printf("Type: Binary Operation; Operator: %.*s.\n", n->op.length, n->op.start);
            printNodes(n->expression1, depth + 1);
            printNodes(n->expression2, depth + 1);
            break;
        }
        case NODE_ASSIGNMENT:
        {
            auto n = std::static_pointer_cast<AssignmentNode>(start);
            printf("Type: Assignment;\n");
            printNodes(n->variable, depth + 1);
            printNodes(n->assignment, depth + 1);
            break;
        }
        case NODE_FIELDCALL:
        {
            auto n = std::static_pointer_cast<FieldCallNode>(start);
            printf("Type: Field Call;\n");
            printf("Operation Type: %.*s.\n", n->op.length, n->op.start);
            printNodes(n->expr, depth + 1);
            for(int i = 0; i < depth; i++) printf(" ");
            printf("Field: %.*s\n", n->field.length, n->field.start);
            break;
        }
        case NODE_ARRAYCONSTRUCTOR:
        {
            auto n = std::static_pointer_cast<ArrayConstructorNode>(start);
            printf("Type: Array Constructor;\n");
            for (size_t i = 0; i < n->values.size(); i++)
            {
                printNodes(n->values[i], depth + 1);
            }
            break;
        }
        case NODE_FUNCTIONCALL:
        {
            auto n = std::static_pointer_cast<FunctionCallNode>(start);
            printf("Type: Function Call;\n");
            printNodes(n->called, depth + 1);
            for (size_t i = 0; i < n->args.size(); i++)
            {
                printNodes(n->args[i], depth + 1);
            }
            break;
        }
        case NODE_ARRAYINDEX:
        {
            auto n = std::static_pointer_cast<ArrayIndexNode>(start);
            printf("Type: Array Index;\n");
            printNodes(n->array, depth + 1);
            printNodes(n->index, depth + 1);
            break;
        }
        default:
        {
            printf("Unimplemented!");
            system("pause");
        }
    }
}

std::shared_ptr<ProgramNode> parse(const char *src)
{
    Lexer lexer = initLexer(src);
    Parser parser;
    parser.lexer = &lexer;
    parser.hadError = false;
    parser.panicMode = false;
    advance(&parser);
    advance(&parser);

    auto ast = std::make_shared<ProgramNode>();
    ast->nodeType = NODE_PROGRAM;
    ast->globalScope = newScope(nullptr);
    while (parser.current.type != EOF_)
    {
        auto dec = declaration(&parser, ast->globalScope);
        if(dec != nullptr)
        {
            ast->declarations.push_back(dec);
        }
    }
    ast->hadError = parser.hadError;
    //if (!ast->hadError)
    //{
    //    printNodes(std::static_pointer_cast<node>(ast), 0);
    //}
    return ast;
}