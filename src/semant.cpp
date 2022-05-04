#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include "semant.h"

static void error(Token *token, const char *msg)
{
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
}

static bool typesEqual(Ty type1, Ty type2)
{
    if(type1.size() != type2.size()) 
    {
        error(&type1[0], "type mismatch!");
        return false;
    }

    for(size_t i = 0; i < type1.size(); i++)
    {
        Token t1 = type1[i];
        Token t2 = type2[i];

        if(t1.length != t2.length || strncmp(t1.start, t2.start, t1.length) != 0)
        {
            error(&type1[0], "type mismatch!");
            return false;
        }
    }
   
    return true;
}

static bool isFunctionType(Ty type)
{
    return std::any_of(type.begin(), type.end(), [](Token t){return t.length == 2 && strncmp("->", t.start, 2) == 0;});
}

Ty resolveExpressionType(std::shared_ptr<node> expr, std::shared_ptr<ScopeNode> currentScope);

static Ty blockReturnType(std::shared_ptr<BlockStatementNode> block)
{
    Ty type = {};
    for(auto declaration : block->declarations)
    {
        switch(declaration->nodeType)
        {
            case NODE_RETURN:
            {
                auto ret = std::static_pointer_cast<ReturnStatementNode>(declaration);
                Ty returnType = resolveExpressionType(ret->returnExpr, block->scope);
                if(type.size() == 0)
                {
                    type = returnType;
                }
                if(returnType.size() == 0)
                {
                    assert(false);
                    //error() //unsure how to handle this ;~;
                }
                else if(!typecmp(type, returnType))
                {
                    error(&returnType.front(), "Type does not match previous return types!");
                }
            }
            default: continue;
        }
    }

    return type;
}

Ty returnTypeFromFunctionType(Ty type)
{
    Ty result;
    if(type.back().type == CLOSE_PAREN)
    {
        size_t depth = 1;
        for(auto it = type.rbegin() + 1; it != type.rend(); it++)
        {
            switch(it->type)
            {
                case CLOSE_PAREN:
                {
                    depth += 1;
                    break;
                }
                case PAREN:
                {
                    depth -= 1;
                    if(depth == 0)
                    {
                        result.insert(result.end(), it.base(), type.end() - 1);
                        return result;
                    }
                    break;
                }
            }
        }
        if(depth > 0) {}//error: type is invalid!
    }
    else
    {
        result.push_back(type.back());
        return result;
    }
}

size_t argsFromFunctionType(Ty type)
{
    size_t result = 0;
    size_t paren_depth = 0;
    for(auto t : type)
    {
        switch(t.type)
        {
            case PAREN:
            {
                paren_depth++;
                break;
            }
            case CLOSE_PAREN:
            {
                paren_depth--;
                break;
            }
            case ARROW:
            {
                if(paren_depth == 0) result++;
                break;
            }
        }
    }
    return result;
}

Ty functionVariableReturnType(std::shared_ptr<node> expr, std::shared_ptr<ScopeNode> currentScope)
{
    switch(expr->nodeType)
    {
        case NODE_IDENTIFIER:
        {
            //either another variable or a function; check function case first then recurse
            auto id = std::static_pointer_cast<VariableNode>(expr);
            auto s = currentScope;
            while(s != nullptr)
            {
                auto vd = s->variables.find(tokenToString(id->variable));
                if(vd != s->variables.end())
                {
                    if(isFunctionType(vd->second->type))
                    {
                        if(vd->second->value != nullptr)
                        {
                            return functionVariableReturnType(vd->second->value, currentScope);
                        }
                        else
                        {
                            return returnTypeFromFunctionType(vd->second->type);
                        }
                    }
                    else
                    {
                        //error: "cannot call a non-function type!"
                    }
                }
                else
                {
                    auto fd = s->functions.find(tokenToString(id->variable));
                    if(fd != s->functions.end())
                    {
                        return fd->second->returnType;
                    }
                }
            }
            //error: "function not found!"
        }
        case NODE_LAMBDA:
        {
            auto l = std::static_pointer_cast<LambdaNode>(expr);
            return l->returnType;
        }
        case NODE_FIELDCALL:
        {
            auto fc = std::static_pointer_cast<FieldCallNode>(expr);
            auto id = fc->field;
            Ty type = resolveExpressionType(fc->expr, currentScope);
            if(type.size() != 0)
            {            
                auto typeName = tokenToString(type[0]);
                auto s = currentScope;
                while(s != nullptr)
                {
                    if(s->types.find(typeName) != s->types.end())
                    {
                        auto t = s->types.at(typeName);
                        for(auto fields : t->fields)
                        {
                            if(tokencmp(id, fields.identifier))
                            {
                                if(isFunctionType(fields.type))
                                {
                                    return returnTypeFromFunctionType(fields.type);
                                }
                                else
                                {
                                    error(&id, "cannot call a non-function type!");
                                    return {};
                                }
                            }
                        }
                    }
                }
                error(&id, "not a valid field!");
                return {};
            }
            error(&id, "invalid type!");
            return {};
        }
        case NODE_FUNCTIONCALL:
        {

            return returnTypeFromFunctionType(resolveExpressionType(expr, currentScope));
        }
        case NODE_PLACEHOLDER:
        {
            return returnTypeFromFunctionType(resolveExpressionType(expr, currentScope));
        }
    }
}

Ty resolveExpressionType(std::shared_ptr<node> expr, std::shared_ptr<ScopeNode> currentScope)
{
    switch(expr->nodeType)
    {
        case NODE_LITERAL:
        {
            Ty type;
            auto n =  std::static_pointer_cast<LiteralNode>(expr);
            switch(n->value.type)
            {
                case TRUE:
                case FALSE:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Bool";
                    t.length = 4;
                    t.line = n->value.line;
                    type.push_back(t);
                    return type;
                }
                case INT:
                case HEX_INT:
                case OCT_INT:
                case BIN_INT:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Int";
                    t.length = 3;
                    t.line = n->value.line; 
                    type.push_back(t);
                    return type;
                }
                case FLOAT:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Float";
                    t.length = 5;
                    t.line = n->value.line;
                    type.push_back(t);
                    return type;
                }
                case DOUBLE:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Double";
                    t.length = 6;
                    t.line = n->value.line;
                    type.push_back(t);
                    return type;
                }
                case CHAR:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Char";
                    t.length = 4;
                    t.line = n->value.line;
                    type.push_back(t);
                    return type;
                }
                case STRING:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "String";
                    t.length = 6;
                    t.line = n->value.line;
                    type.push_back(t);
                    return type;
                }
                default:
                {
                    printf("unknown literal type: %d", n->value.type);
                    return {};
                }
            }
        }
        case NODE_IDENTIFIER:
        {
            auto v = std::static_pointer_cast<VariableNode>(expr);
            auto s = currentScope;
            while(s != nullptr)
            {
                auto t = tokenToString(v->variable);
                auto n = s->variables.find(t);
                if(n != s->variables.end())
                {
                    if(n->second->type.size() != 0)
                    {
                        return n->second->type;
                    }
                    else
                    {
                        error(&v->variable, "variable type not known!"); //possibly-redundant error
                        return {};
                    }
                }
                else if(s->parentScope != nullptr)
                {
                    s = s->parentScope;
                }
                else{
                    error(&v->variable, "variable not defined!");
                    return {};
                }
            }
            return {};
        }
        case NODE_ASSIGNMENT:
        {
            auto a = std::static_pointer_cast<AssignmentNode>(expr);
            if(a->variable != nullptr)
            {
                Ty varType = resolveExpressionType(a->variable, currentScope);
                if(varType.size() != 0)
                {
                    return varType;
                }
                else
                {
                    if(a->assignment != nullptr)
                    {
                        Ty aType = resolveExpressionType(a->assignment, currentScope);
                        return aType;
                    }
                }
            }
            else 
            {
                printf("and you may ask yourself \"how did I get here?\"");
                return {};
            }
            return {};
        }
        case NODE_UNARY:
        {
            //TODO: make this not hard-coded and dependent on standard library definitions
            auto u = std::static_pointer_cast<UnaryNode>(expr);
            return resolveExpressionType(u->expression, currentScope);
        }
        case NODE_BINARY:
        {
            //TODO: make this not hard-coded and dependent on standard library definitions
            auto b = std::static_pointer_cast<BinaryNode>(expr);
            Ty type1 = resolveExpressionType(b->expression1, currentScope);
            Ty type2 = resolveExpressionType(b->expression2, currentScope);
            if(typesEqual(type1, type2))
            {
                return type1;
            }
            else
            { 
                std::cout << typeToString(type1) << ", " << typeToString(type2);
                return {};
            }
        }
        case NODE_FIELDCALL:
        {
            auto fc = std::static_pointer_cast<FieldCallNode>(expr);
            Ty type = resolveExpressionType(fc->expr, currentScope);
            if(type.size() != 0)
            {
                auto s = currentScope;
                while(s != nullptr)
                {
                    std::string t = tokenToString(type.front());
                    auto rd = s->types.find(t);
                    if(rd != s->types.end())
                    {
                        for(size_t i = 0; i < rd->second->fields.size(); i++)
                        {
                            Token field = rd->second->fields[i].identifier;
                            if(tokencmp(field, fc->field))
                            {
                                if(type.size() == 1)
                                {
                                    return rd->second->fields[i].type;
                                }
                                else
                                {
                                    Ty result;
                                    for(auto t : rd->second->fields[i].type)
                                    {
                                        if(t.type == IDENTIFIER)
                                        {
                                            for(auto s : rd->second->typeDefined)
                                            {
                                                if(tokencmp(t, s))
                                                {
                                                    result.push_back(s);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            result.push_back(t);
                                        }
                                    }   
                                    printf("%s", typeToString(result));
                                    return result;
                                }
                            }
                        }
                    }
                    else
                    {
                        s = s->parentScope;
                    }
                }
            }
            return {};
        }
        case NODE_FUNCTIONCALL:
        {
            auto fc = std::static_pointer_cast<FunctionCallNode>(expr);
            
            auto calledType = resolveExpressionType(fc->called, currentScope);

            Ty result;
            size_t k = 0;
            auto ret = returnTypeFromFunctionType(calledType);
            size_t argCount = argsFromFunctionType(calledType);
            for(int i = 0; i < argCount; i++) // TODO: fix this; replace i < calledType.size - ret.size() with i < <some function that finds the number of arguments by type>
            {
                auto arg = fc->args[i];
                switch(arg->nodeType)
                {
                    case NODE_LITERAL:
                    {
                        auto n = std::static_pointer_cast<LiteralNode>(arg);
                        if(n->value.type == UNDERSCORE)
                        {
                            while(calledType[k].type != ARROW)
                            {
                                result.push_back(calledType[k]);
                                k++;
                            }
                            result.push_back(calledType[k]);
                            k++;
                        }
                        else
                        {
                            while(calledType[k].type != ARROW)
                            {
                                k++;
                            }
                            k++;
                        }
                        break;
                    }
                    default: 
                    {
                        while(calledType[k].type != ARROW)
                        {
                            k++;
                        }
                        k++;
                        break;
                    }
                }
            }
            
            result.insert(result.end(), ret.begin(), ret.end());
            return result;
         }
        case NODE_ARRAYCONSTRUCTOR:
        {
            auto arr = std::static_pointer_cast<ArrayConstructorNode>(expr);
            std::vector<Ty> valueTypes;
            for(auto v : arr->values)
            {
                auto nextType = resolveExpressionType(v, currentScope);
                if(valueTypes.size() == 0 || typesEqual(valueTypes.back(), nextType)) valueTypes.push_back(nextType);
                else return {};
            }
            auto result = valueTypes.back();
            int line = result.back().line;
            result.push_back({BRACKET, "[", 1, line});
            result.push_back({CLOSE_BRACKET, "]", 1, line});
            return result;
        }
        case NODE_ARRAYINDEX:
        {
            auto index = std::static_pointer_cast<ArrayIndexNode>(expr);
            auto arrType = resolveExpressionType(index->array, currentScope);
            if(strncmp(resolveExpressionType(index->index, currentScope).front().start, "Int", 3) != 0)
            {
                error(&resolveExpressionType(index->index, currentScope).front(), "Index type is not 'Int'!");
                return {};
            }
            if(arrType.back().type == CLOSE_BRACKET)
            {
                arrType.resize(arrType.size() - 2);
                return arrType;
            }
            else
            {
                error(&arrType.front(), "not a valid array type!");
            }
        }
        case NODE_LAMBDA:
        {
            auto n = std::static_pointer_cast<LambdaNode>(expr);
            n->returnType = blockReturnType(std::static_pointer_cast<BlockStatementNode>(n->body));
            Ty result = {};
            for(auto p : n->params)
            {
                if(isFunctionType(p.type))
                {
                    result.push_back({PAREN, "(", 1, p.type[0].line});
                    result.insert(result.end(), p.type.begin(), p.type.end());
                    result.push_back({CLOSE_PAREN, ")", 1, result.back().line});
                }
                else
                {
                    result.insert(result.end(), p.type.begin(), p.type.end());
                }
                result.push_back({LEFT_ARROW, "->", 2, result.back().line});
            }
            result.insert(result.end(), n->returnType.begin(), n->returnType.end());
            return result;
        }
        case NODE_LISTINIT:
        {
            //TODO: extend this to work when only the base type is given, so long as the remaining types can be deduced
            auto listinit = std::static_pointer_cast<ListInitNode>(expr);
            if(listinit->type.size() == 0)
            {
                //error
                return {};
            }
            else
            {
                auto s = currentScope;
                while(s != nullptr)
                {
                    if(s->types.find(tokenToString(listinit->type.front())) != s->types.end())
                    {
                        auto t = s->types.at(tokenToString(listinit->type.front()));
                        if(t->fields.size() != listinit->values.size())
                        {
                            //error
                            return {};
                        }
                        Ty result;
                        std::unordered_map<std::string, Token> typeParams;
                        if(t->typeDefined.size() > 1)
                        {
                            for(size_t j = 0; j < t->typeDefined.size(); j++)
                            {
                                auto tk = t->typeDefined[j];
                                switch(tk.type)
                                {
                                    case TYPE:
                                    {
                                        result.push_back(tk);
                                        break;
                                    }
                                    case IDENTIFIER:
                                    {
                                        result.push_back(listinit->type[j]);
                                        typeParams.insert({std::string(tk.start, tk.length), listinit->type[j]});
                                    }
                                }
                            }
                        }
                        else
                        {
                            result = t->typeDefined;
                        }
                        for(size_t i = 0; i < t->fields.size(); i++)
                        {
                            auto valueType = resolveExpressionType(listinit->values[i], currentScope);
                            for(size_t j = 0; j < t->fields[i].type.size(); j++)
                            {
                                switch(t->fields[i].type[j].type)
                                {
                                    case IDENTIFIER:
                                    {
                                        if(typeParams.find(std::string(t->fields[i].type[j].start, t->fields[i].type[j].length)) != typeParams.end())
                                        {
                                            auto p = typeParams.at(std::string(t->fields[i].type[j].start, t->fields[i].type[j].length));
                                            if(!tokencmp(p, valueType[j]))
                                            {
                                                //error
                                                return {};
                                            }
                                        }
                                        else
                                        {
                                            //error
                                            return {};
                                        }
                                        break;
                                    }
                                    default:
                                    {
                                        if(!tokencmp(t->fields[i].type[j],valueType[j]))
                                        {
                                            //error
                                            return {};
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        return result;
                    }
                    else
                    {
                        s = s->parentScope;
                    }
                }
                //error
                return {};
            }
        }
        default:
        {
            printf("not an expression, or expression type not implemented!");
            return {};
        }
    }
}

bool verifyTypes(std::shared_ptr<node> n, std::shared_ptr<ScopeNode> currentScope, Ty expectedType)
{
    switch (n->nodeType)
    {
        case NODE_LIBRARY:
        {
            return false;
            break;
        }
        case NODE_IMPORT:
        {
            return false;
            break;
        }
        case NODE_TYPEDEF:
        {
            return false;
            break;
        }
        case NODE_RECORDDECL:
        {
            return false;
            break;
        }
        case NODE_FUNCTIONDECL:
        {
            auto fd = std::static_pointer_cast<FunctionDeclarationNode>(n);
            if(fd->returnType.size() == 0)
            {
                if(fd->body == nullptr)
                {
                    error(&fd->identifier, "cannot resolve type of undefined function!");
                    return true;
                }
                else
                {
                    fd->returnType = blockReturnType(std::static_pointer_cast<BlockStatementNode>(fd->body));
                    if(fd->returnType.size() == 0)
                    {
                        error(&fd->identifier, "Unable to resolve return type!");
                        return true;
                    }
                    return false;
                }
            }
            else
            {
                return verifyTypes(fd->body, currentScope, fd->returnType);
            }
        }
        case NODE_VARIABLEDECL:
        {
            auto vd = std::static_pointer_cast<VariableDeclarationNode>(n);
            if(vd->type.size() == 0) 
            {
                if(vd->value == nullptr)
                {
                    error(&vd->identifier, "cannot resolve type of undefined variable!");
                    return true;
                }
                else
                {
                    vd->type = resolveExpressionType(vd->value, currentScope);
                    if(vd->type.size() == 0) //if still nullptr, error out
                    {
                        error(&vd->identifier, "cannot resolve type of declaration!");
                        return true;
                    }
                }
            }
            currentScope->variables.insert(std::make_pair(tokenToString(vd->identifier), vd));
            return false;
        }
        case NODE_CLASSDECL:
        {
            return false;
            break;
        }
        case NODE_FOR:
        {
            return false;
            break;
        }
        case NODE_IF:
        {
            return false;
            break;
        }
        case NODE_WHILE:
        {
            return false;
            break;
        }
        case NODE_SWITCH:
        {
            return false;
            break;
        }
        case NODE_CASE:
        {
            return false;
            break;
        }
        case NODE_RETURN:
        {
            auto rn = std::static_pointer_cast<ReturnStatementNode>(n);
            if(typecmp(expectedType, resolveExpressionType(rn->returnExpr, currentScope)) == true)
            {
                return false;
            }
            else
            {
                Token t = {RETURN, "return", 6, 0};
                error(&t, "return type does not match function type!");
                return true;
            }
        }
        case NODE_CONTINUE:
        {
            return false;
            break;
        }
        case NODE_BLOCK:
        {
            auto bn = std::static_pointer_cast<BlockStatementNode>(n);
            bool hadError = false;
            for(size_t i = 0; i < bn->declarations.size(); i++)
            {
                hadError = verifyTypes(bn->declarations[i], bn->scope, expectedType);
            }
            return hadError;
        }
        default:
        {
            printf("unimplemented!");
            return true;
        }
    }
    return false;
}

std::shared_ptr<ProgramNode> analyze(const char *src)
{
    std::shared_ptr<ProgramNode> ast = parse(src);
    if(ast->hadError) return nullptr;
    for (size_t i = 0; i < ast->declarations.size(); i++)
    {
        ast->hadError |= verifyTypes(ast->declarations[i], ast->globalScope, {});
    }
    if(!ast->hadError)
    {
        for(auto variable : ast->globalScope->variables)
        {
            std::cout << tokenToString(variable.second->identifier) << ": " << typeToString(variable.second->type) << "\n";
        }
        return ast;
    }
    return nullptr;
}
