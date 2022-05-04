#include <cassert>
#include <cstring>
#include <cstdio>
#include <utility>
#include "parser.h"


std::shared_ptr<ScopeNode> newScope(std::shared_ptr<ScopeNode> parent)
{
    auto n = std::make_shared<ScopeNode>();
    n->parentScope = parent;
    n->nodeType = NODE_SCOPE;
    return n;
}

bool typecmp(Ty a, Ty b)
{
    if(a.size() == b.size())
    {
        for(size_t i = 0; i < a.size(); i++)
        {
            if(strncmp(a[i].start, b[i].start, a[i].length) != 0)
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
    std::shared_ptr<node>(*prefix)(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>(*infix)(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node> n);
    Precedence precedence;
} ParseRule;

std::shared_ptr<node>expression(Parser *parser, std::shared_ptr<ScopeNode> scope);

std::shared_ptr<node>grouping(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>arrayIndex(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>unary(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>binary(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>assignment(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>literal(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>list_init(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>lambda(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>identifier(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>field_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>array_constructor(Parser *parser, std::shared_ptr<ScopeNode> scope);
std::shared_ptr<node>array_index(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
std::shared_ptr<node>function_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);

std::unordered_map<TokenType, ParseRule> rules = {
    {PAREN, {grouping, function_call, PRECEDENCE_POSTFIX}},
    {CLOSE_PAREN, {nullptr, nullptr, PRECEDENCE_NONE}},
    {BRACE, {nullptr, nullptr, PRECEDENCE_NONE}},
    {CLOSE_BRACE, {nullptr, nullptr, PRECEDENCE_NONE}},
    {BRACKET, {array_constructor, array_index, PRECEDENCE_POSTFIX}},
    {CLOSE_BRACKET, {nullptr, nullptr, PRECEDENCE_NONE}},
    {COMMA, {nullptr, nullptr, PRECEDENCE_NONE}},
    {DOT, {nullptr, field_call, PRECEDENCE_POSTFIX}},
    {COLON, {nullptr, nullptr, PRECEDENCE_NONE}},
    {MINUS, {unary, binary, PRECEDENCE_ADD}},
    {PLUS, {nullptr, binary, PRECEDENCE_ADD}},
    {AMPERSAND, {unary, binary, PRECEDENCE_BITWISE_AND}},
    {LEFT_ARROW, {nullptr, nullptr, PRECEDENCE_NONE}},
    {ARROW, {nullptr, field_call, PRECEDENCE_POSTFIX}},
    {EQUAL, {nullptr, assignment, PRECEDENCE_ASSIGNMENT}},
    {SEMICOLON, {nullptr, nullptr, PRECEDENCE_NONE}},
    {SLASH, {nullptr, binary, PRECEDENCE_MULTIPLY}},
    {STAR, {unary, binary, PRECEDENCE_MULTIPLY}},
    {BANG, {unary, nullptr, PRECEDENCE_PREFIX}},
    {BANG_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY}},
    {EQUAL_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY}},
    {LESS, {nullptr, binary, PRECEDENCE_COMPARISON}},
    {LESS_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY}},
    {GREATER, {nullptr, binary, PRECEDENCE_COMPARISON}},
    {GREATER_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY}},
    {TYPE, {list_init, nullptr, PRECEDENCE_NONE}},
    {LET, {nullptr, nullptr, PRECEDENCE_NONE}},
    {IDENTIFIER, {identifier, nullptr, PRECEDENCE_NONE}},
    {UNDERSCORE, {literal, nullptr, PRECEDENCE_NONE}},
    {TYPEDEF, {nullptr, nullptr, PRECEDENCE_NONE}},
    {SWITCH, {nullptr, nullptr, PRECEDENCE_NONE}},
    {CASE, {nullptr, nullptr, PRECEDENCE_NONE}},
    {CLASS, {nullptr, nullptr, PRECEDENCE_NONE}},
    {IMPLEMENT, {nullptr, nullptr, PRECEDENCE_NONE}},
    {USING, {nullptr, nullptr, PRECEDENCE_NONE}},
    {LAMBDA, {lambda, nullptr, PRECEDENCE_PREFIX}},
    {MUTABLE, {nullptr, nullptr, PRECEDENCE_NONE}},
    {TRUE, {literal, nullptr, PRECEDENCE_NONE}},
    {FALSE, {literal, nullptr, PRECEDENCE_NONE}},
    {FLOAT, {literal, nullptr, PRECEDENCE_NONE}},
    {DOUBLE, {literal, nullptr, PRECEDENCE_NONE}},
    {CHAR, {literal, nullptr, PRECEDENCE_NONE}},
    {INT, {literal, nullptr, PRECEDENCE_NONE}},
    {HEX_INT, {literal, nullptr, PRECEDENCE_NONE}},
    {OCT_INT, {literal, nullptr, PRECEDENCE_NONE}},
    {BIN_INT, {literal, nullptr, PRECEDENCE_NONE}},
    {STRING, {literal, nullptr, PRECEDENCE_NONE}},
    {IF, {nullptr, nullptr, PRECEDENCE_NONE}},
    {WHILE, {nullptr, nullptr, PRECEDENCE_NONE}},
    {FOR, {nullptr, nullptr, PRECEDENCE_NONE}},
    {RETURN, {nullptr, nullptr, PRECEDENCE_NONE}},
    {ELSE, {nullptr, nullptr, PRECEDENCE_NONE}},
    {AND, {nullptr, binary, PRECEDENCE_LOGICAL_AND}},
    {OR, {nullptr, binary, PRECEDENCE_LOGICAL_OR}},
    {NOT, {unary, nullptr, PRECEDENCE_PREFIX}},
    {BIT_OR, {nullptr, binary, PRECEDENCE_BITWISE_OR}},
    {SHIFT_LEFT, {nullptr, binary, PRECEDENCE_SHIFT}},
    {SHIFT_RIGHT, {nullptr, binary, PRECEDENCE_SHIFT}},
    {BREAK, {nullptr, nullptr, PRECEDENCE_NONE}},
    {NEWLINE, {nullptr, nullptr, PRECEDENCE_NONE}},
    {ERROR, {nullptr, nullptr, PRECEDENCE_NONE}},
    {EOF_, {nullptr, nullptr, PRECEDENCE_NONE}}};

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

Ty resolve_type(Parser *parser, bool isDeclared)
{
    Ty type;
    Parser state = *parser;
    size_t nested_brackets = 0;
    size_t nested_parens = 0;
    
    while (parser->next.type == TYPE || parser->next.type == IDENTIFIER || parser->next.type == BRACKET || parser->next.type == CLOSE_BRACKET || parser->next.type == STAR || parser->next.type == ARROW || parser->next.type == PAREN || parser->next.type == CLOSE_PAREN)
    {
        switch(parser->current.type)
        {
            case TYPE:
            case IDENTIFIER:
            {
                type.push_back(parser->current);
                advance(parser);
                break;
            }
            case BRACKET:
            {
                if(type.size() == 0)
                {
                    *parser = state;
                    return {};
                }
                else if (type.back().type == PAREN || type.back().type == BRACKET || type.back().type == ARROW)
                {
                    *parser = state;
                    return {};
                }
                else
                {
                    nested_brackets++;
                    type.push_back(parser->current);
                    advance(parser);
                }
                break;
            }
            case CLOSE_BRACKET:
            {
                nested_brackets--;
                type.push_back(parser->current);
                advance(parser);
                break;
            }
            case PAREN:
            {
                if(type.size() != 0)
                {
                    if(type.back().type != PAREN && type.back().type != ARROW)
                    {
                        *parser = state;
                        return {};
                    }
                }
                nested_parens++;
                type.push_back(parser->current);
                advance(parser);
                break;
            }
            case CLOSE_PAREN:
            {
                nested_parens--;
                type.push_back(parser->current);
                advance(parser);
                break;
            }
            case STAR:
            {
                if(type.size() == 0)
                {
                    *parser = state;
                    return {};
                }
                else if(type.back().type == PAREN || type.back().type == BRACE || type.back().type == ARROW) 
                {
                    *parser = state;
                    return {};
                }
                else
                {
                    type.push_back(parser->current);
                    advance(parser);
                }
                break;
            }
            case ARROW:
            {
                if(type.size() == 0)
                {
                    *parser = state;
                    return {};
                }
                else if (type.back().type == PAREN || type.back().type == BRACKET || type.back().type == ARROW)
                {
                    *parser = state;
                    return {};
                }
                else
                {
                    type.push_back(parser->current);
                    advance(parser);
                }
                break;
            }
        }
    }
    if (isDeclared && (parser->current.type == TYPE || parser->current.type == IDENTIFIER || parser->current.type == BRACKET || parser->current.type == CLOSE_BRACKET || parser->current.type == STAR || parser->current.type == ARROW || parser->current.type == PAREN || parser->current.type == CLOSE_PAREN))
    {
        type.push_back(parser->current);
        advance(parser);
    }
    if(nested_brackets != 0 || nested_parens != 0)
    {
        *parser = state;
        return {};
    }
    return type;
}

static std::shared_ptr<node>var_decl(Parser *parser, Ty type, std::shared_ptr<ScopeNode> scope)
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

static std::shared_ptr<node> func_decl(Parser *parser, Ty returnType, std::shared_ptr<ScopeNode>scope)
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
        }
        return nullptr;
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

    scope->functions.insert(std::make_pair(tokenToString(result->identifier), result));

    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node>struct_or_union(Parser *parser, std::shared_ptr<ScopeNode> scope)
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
    result->typeDefined = resolve_type(parser, true);

    scope->types.insert(std::make_pair(tokenToString(result->typeDefined[0]), result));

    consume(parser, BRACE, "expected '{' after type name!");
    while (parser->current.type != CLOSE_BRACE)
    {
        Parameter p;
        p.type = resolve_type(parser, false);
        p.identifier = parser->current;
        advance(parser);
        result->fields.push_back(p);
        consume(parser, SEMICOLON, "expected ';' after field name!");
    }
    consume(parser, CLOSE_BRACE, "expected '}' after struct or union declaration!");
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node>typedef_decl(Parser *parser, std::shared_ptr<ScopeNode>scope)
{
    auto result = std::make_shared<TypedefNode>();
    result->nodeType = NODE_TYPEDEF;
    result->typeAliased = resolve_type(parser, false);
    result->typeDefined = resolve_type(parser, true);
    auto t = typeToString(result->typeDefined);
    scope->typeAliases.insert(std::make_pair(t, result));

    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node>class_decl(Parser *parser, std::shared_ptr<ScopeNode>scope)
{

    //TODO: figure out what's wrong with resolve_type
    auto result = std::make_shared<ClassDeclarationNode>();
    result->nodeType = NODE_CLASSDECL;
    advance(parser);
    result->className = resolve_type(parser, true);
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
        std::shared_ptr<node> func = func_decl(parser, type, scope);

        result->functions.push_back(func);
    }

    consume(parser, CLOSE_BRACE, "expected '}' after function declarations!");

    scope->classes.insert(std::make_pair(tokenToString(result->className[0]), result));

    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node> impl_decl(Parser* parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<ClassImplementationNode>();
    n->nodeType = NODE_CLASSIMPL;
    n->_class = parser->current;
    advance(parser);
    n->implemented = resolve_type(parser, false);
    consume(parser, BRACE, "expected '{' before function definitions!");
    while(parser->current.type != CLOSE_BRACE)
    {
        n->functions.push_back(func_decl(parser, {}, scope));
    }
    return std::static_pointer_cast<node>(n);
}

std::shared_ptr<node>declaration(Parser *parser, std::shared_ptr<ScopeNode> scope)
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
            return func_decl(parser, type, scope);
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
            return func_decl(parser, {}, scope);
        default:
            return var_decl(parser, {}, scope);
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

static std::shared_ptr<node>switch_stmt(Parser *parser, std::shared_ptr<ScopeNode> scope)
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

static std::shared_ptr<node>for_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
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

static std::shared_ptr<node>if_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
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

static std::shared_ptr<node>while_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
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

std::shared_ptr<node>block_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
{

    auto result = std::make_shared<BlockStatementNode>();
    advance(parser);
    result->nodeType = NODE_BLOCK;
    std::shared_ptr<ScopeNode>next = newScope(scope);
    result->scope = next;
    while (parser->current.type != CLOSE_BRACE)
    {
        result->declarations.push_back(declaration(parser, scope));
    }
    consume(parser, CLOSE_BRACE, "expect '}' at end of block!");
    return std::static_pointer_cast<node>(result);
}

static std::shared_ptr<node>return_stmt(Parser *parser, std::shared_ptr<ScopeNode> scope)
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

static std::shared_ptr<node>parsePrecedence(Parser *parser, Precedence prec, std::shared_ptr<ScopeNode> scope)
{
    advance(parser);
    ParseRule *p = getRule(parser->previous.type);
    if (p->prefix == nullptr)
    {
        error(parser, "expected an expression!");
        return nullptr;
    }

    std::shared_ptr<node> result = p->prefix(parser, scope);

    while (prec <= getRule(parser->current.type)->precedence)
    {
        advance(parser);
        p = getRule(parser->previous.type);
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

std::shared_ptr<node> list_init(Parser* parser, std::shared_ptr<ScopeNode> scope)
{
    auto n = std::make_shared<ListInitNode>();
    n->nodeType = NODE_LISTINIT;
    auto front = Ty{parser->previous};
    if(parser->current.type != BRACE)
    {
        auto rest = resolve_type(parser, true);
        front.insert(front.end(), rest.begin(), rest.end());
    }
    n->type = front;
    
    consume(parser, BRACE, "expected '{' before list initialization!");
    while(parser->current.type != CLOSE_BRACE)
    {
        n->values.push_back(expression(parser, scope));
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
    n->returnType = {};
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
    std::shared_ptr<node>result = expression(parser, scope);
    consume(parser, CLOSE_PAREN, "expected a ')' after expression!");
    return result;
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
    auto n = std::make_shared<BinaryNode>();
    n->nodeType = NODE_BINARY;
    n->expression1 = expression1;
    n->op = parser->previous;
    ParseRule *rule = &rules[parser->previous.type];
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

std::string typeToString(Ty type)
{
    if(type.size() == 0) return "type unknown";
    std::string result;
    for (auto token : type)
    {
        switch (token.type)
        {
            case BRACKET:
            {
                result.append(tokenToString(token));
                break;
            }
            default:
            {
                result.append(tokenToString(token)).append(" ");
                break;
            }
        }
    }
    return result;
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
        printf("Type: Variable Declaration; Name: %.*s Type: %s\n", n->identifier.length, n->identifier.start, typeToString(n->type).c_str());
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
        printf("Type: Function Declaration; Name: %.*s, Return Type: %s\n", n->identifier.length, n->identifier.start, typeToString(n->returnType).c_str());
        printf("Parameters: ");
        bool first = true;
        for (size_t i = 0; i < n->params.size(); i++)
        {
            if (first)
                first = false;
            else
                printf(", ");
            printf("%s %.*s", typeToString(n->params[i].type).c_str(), n->params[i].identifier.length, n->params[i].identifier.start);
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
        printf("Type: Typedef; Aliased Type: %s Defined Type: %s\n", typeToString(n->typeAliased).c_str(), typeToString(n->typeDefined).c_str());
        break;
    }
    case NODE_RECORDDECL:
    {
        auto n = std::static_pointer_cast<RecordDeclarationNode>(start);
        printf("Type: %s; Name: %s\n", 
            n->struct_or_union == RecordDeclarationNode::IS_STRUCT ? "STRUCT" 
            : n->struct_or_union == RecordDeclarationNode::IS_UNION ? "UNION"
            : "UNDEFINED", typeToString(n->typeDefined).c_str());
        bool first = true;
        printf("Fields: ");
        for (size_t i = 0; i < n->fields.size(); i++)
        {
            if (first)
                first = false;
            else
                printf(", ");
            printf("%s %.*s", typeToString(n->fields[i].type).c_str(), n->fields[i].identifier.length, n->fields[i].identifier.start);

        }
        printf("\n");
        break;
    }
    case NODE_CLASSDECL:
    {
        auto n = std::static_pointer_cast<ClassDeclarationNode>(start);
        printf("Type: Class; Name: %s\n", typeToString(n->className).c_str());
        if (n->constraints.size() > 0)
        {
            printf("Constraints: ");
            bool first = true;
            for (size_t i = 0; i < n->constraints.size(); i++)
            {
                if (first)
                    first = false;
                else
                    printf(", ");
                
                printf("%s",typeToString(n->constraints[i]).c_str());
            }
        }
        printf("Functions:\n");
        for (size_t i = 0; i < n->functions.size(); i++)
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
        printf("Type: List Init; Name: %s\n", typeToString(n->type).c_str());
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
        assert(false);
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
    if (!ast->hadError)
    {
        //printNodes(std::static_pointer_cast<node>(ast), 0);
    }
    return ast;
}