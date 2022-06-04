#include <stdlib.h>
#include <cassert>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <deque>
#include <unordered_set>
#include "semant.h"
#include <algorithm>

//TODO: arrayIndex is an evil hack and should be replaced by defining a ([]) operator

static bool typesEqual(Ty type1, Ty type2)
{
   return type1.compare(type2) == 0;
}

static bool isFunctionType(Ty type)
{
    if(type.find("->") != std::string::npos) return true;
    else return false;
}

Ty returnTypeFromFunctionType(Ty type)
{
    Ty result;
    std::vector<Token> tokens;
    
    {
        Lexer l = initLexer(type.c_str());
        l.typeInfOnly = true;
        Token current = scanToken(&l);
        while(current.type != EOF_)
        {
            tokens.push_back(current);
            current = scanToken(&l);
        }
    }
    if(tokens.back().type == CLOSE_PAREN)
    {
        size_t depth = 1;
        for(auto it = tokens.rbegin() + 1; it != tokens.rend(); it++)
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
                        while(it.base() != tokens.end())
                        {
                            result.append(tokenToString(*it));
                            it++;
                        }
                        return result;
                    }
                    break;
                }
            }
        }
        if(depth > 0) {}//error: type is invalid!
    }
    result.push_back(type.back());
    return result;
}

Ty argsFromFunctionType(Ty type)
{
    Ty ret = returnTypeFromFunctionType(type);
    while(ret.length() != 0)
    {
        ret.pop_back();
        type.pop_back();
    }
    while(type.back() == ' ' || type.back() == '-' || type.back() == '>')
    {
        type.pop_back();
    }
    return type;
}

Ty firstParameterFromFunctionType(Ty type)
{
    Ty result;
    std::vector<Token> tokens;
    {
        Lexer l = initLexer(type.c_str());
        l.typeInfOnly = true;
        Token current = scanToken(&l);
        while(current.type != EOF_)
        {
            tokens.push_back(current);
            current = scanToken(&l);
        }
    }
    if(tokens.front().type == PAREN)
    {
        size_t depth = 1;
        for(auto it = tokens.begin() + 1; it != tokens.end(); it++)
        {
            switch(it->type)
            {
                case PAREN:
                {
                    depth += 1;
                    break;
                }
                case CLOSE_PAREN:
                {
                    depth -= 1;
                    if(depth == 0)
                    {
                        for(auto it2 = tokens.begin(); it2 != it; it2++)
                        {
                            result.append(tokenToString(*it2));
                        }
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
        result.append(tokenToString(tokens.front()));
    }
    return result;
}

std::vector<Ty> functionTypeSplit(Ty type)
{
    std::vector<Ty> result;
    Ty resultString;
    std::vector<Token> tokens;
    {
        Lexer l = initLexer(type.c_str());
        l.typeInfOnly = true;
        Token current = scanToken(&l);
        while(current.type != EOF_)
        {
            tokens.push_back(current);
            current = scanToken(&l);
        }
    }
    size_t depth = 0;
    size_t pos = 0;
    for(size_t i = 0; i < tokens.size(); i++)
    {
        auto token = tokens[i];
        switch(token.type)
        {
            case PAREN:
            {
                depth += 1;
                break;
            }
            case CLOSE_PAREN:
            {
                depth -= 1;
            }
            default:
            {
                if(depth == 0)
                {
                    for(auto j = pos; j <= i; j++)
                    {
                        resultString.append(tokenToString(tokens[j]));
                    }
                    pos = i + 1;
                    if(resultString.size() != 0) result.push_back(resultString);
                    resultString.clear();
                }
                break;
            }
        }
    }
    if(depth > 0) {}//error: type is invalid!
    return result;
}

Ty functionTypeFromFunction(std::shared_ptr<FunctionDeclarationNode> f)
{
    Ty result;
    const char* arrow = "->";
    for(auto param : f->params)
    {
        result.append(param.type).append(" ").append(arrow).append(" ");
    }
    result.append(f->returnType);
    return result;
}

std::shared_ptr<node> findInScope(std::string toFind, std::shared_ptr<ScopeNode> scope)
{
    Lexer l = initLexer(toFind.c_str());
    Token t = scanToken(&l);
    switch(t.type)
    {
        case TYPE:
        {
            auto s = scope;
            while(s != nullptr)
            {
                if(s->types.find(tokenToString(t)) != s->types.end())
                {
                    return s->types.at(tokenToString(t));
                }
                else if(s->typeAliases.find(tokenToString(t)) != s->typeAliases.end())
                {
                    return s->typeAliases.at(tokenToString(t));
                }
                else if(s->classes.find(tokenToString(t)) != s->classes.end())
                {
                    return s->classes.at(tokenToString(t));
                }
                else
                {
                    s = s->parentScope;
                }
            }
            break;
        }
        case IDENTIFIER:
        {
            auto s = scope;
            while(s != nullptr)
            {
                if(s->variables.find(tokenToString(t)) != s->variables.end())
                {
                    return s->variables.at(tokenToString(t));
                }
                else if(s->functions.find(tokenToString(t)) != s->functions.end())
                {
                    return s->functions.at(tokenToString(t));
                }
                else
                {
                    s = s->parentScope;
                }
            }
            break;
            break;
        }
    }
    return nullptr;
}

Ty typeInf(std::shared_ptr<node> n, std::shared_ptr<ScopeNode> currentScope)
{
    assert(n != nullptr);
    assert(currentScope != nullptr);
    switch(n->nodeType)
    {
        case NODE_TYPEDEF:
        {
            auto td = std::static_pointer_cast<TypedefNode>(n);
            currentScope->constraints.push_back(std::make_pair(td->typeDefined, td->typeAliased));
            return td->typeDefined;
        }
        case NODE_RECORDDECL:
        {
            return "";
        }
        case NODE_FUNCTIONDECL:
        {
            auto fd = std::static_pointer_cast<FunctionDeclarationNode>(n);
            //this style needs to be reworked into a loop that finds the declaration in a scope
            currentScope->nodeTVars.insert(std::make_pair(functionTypeFromFunction(fd), fd));
            if(fd->body) 
            {
                Ty bodyType = typeInf(fd->body, currentScope);
                currentScope->constraints.push_back(std::make_pair(fd->returnType, bodyType));
            }
            return functionTypeFromFunction(fd);
        }
        case NODE_VARIABLEDECL:
        {
            auto vd = std::static_pointer_cast<VariableDeclarationNode>(n);
            auto s = currentScope;
            while(s != nullptr)
            {
                if(s->variables.find(tokenToString(vd->identifier)) != s->variables.end())
                {
                    break;
                }
                else s = s->parentScope;
            }
            if(s == nullptr) currentScope->variables.insert(std::make_pair(tokenToString(vd->identifier), vd));
            currentScope->nodeTVars.insert(std::make_pair(vd->type, vd));
            if(vd->value) 
            {
                Ty valueType = typeInf(vd->value, currentScope);
                currentScope->constraints.push_back(std::make_pair(vd->type, valueType));
            }
            return vd->type;
        }
        case NODE_CLASSDECL:
        {
            return "";
        }
        case NODE_CLASSIMPL:
        {
            return "";
        }
        case NODE_FOR:
        {
            //TODO: for, if, while, switch should have their own scopes
            auto _for = std::static_pointer_cast<ForStatementNode>(n);
            if(_for->initExpr) typeInf(_for->initExpr, currentScope);
            if(_for->condExpr) typeInf(_for->condExpr, currentScope);
            if(_for->incrementExpr) typeInf(_for->incrementExpr, currentScope);
            if(_for->loopStmt) typeInf(_for->loopStmt, currentScope);
            return "";
        }
        case NODE_IF:
        {
            auto _if = std::static_pointer_cast<IfStatementNode>(n);
            typeInf(_if->branchExpr, currentScope);
            Ty t1, t2;
            if(_if->thenStmt) t1 = typeInf(_if->thenStmt, currentScope);
            if(_if->elseStmt) t2 = typeInf(_if->elseStmt, currentScope);
            if(t1.size() == 0) return t2;
            else if (t2.size() == 0) return t1;
            else 
            {
                currentScope->constraints.push_back(std::make_pair(t1, t2));
                return t1;
            }
        }
        case NODE_WHILE:
        {
            auto _while = std::static_pointer_cast<WhileStatementNode>(n);
            typeInf(_while->loopExpr, currentScope);
            if(_while->loopStmt) typeInf(_while->loopStmt, currentScope);
            return "";
        }
        case NODE_SWITCH:
        {
            auto sw = std::static_pointer_cast<SwitchStatementNode>(n);
            if(sw->switchExpr) typeInf(sw->switchExpr, currentScope);
            std::vector<Ty> caseRets;
            for(auto c : sw->cases)
            {
                Ty ret = typeInf(c, currentScope);
                if(ret != "")
                {
                    caseRets.push_back(ret);
                }
            }
            if(caseRets.size() != 0)
            {
                Ty t1 = caseRets.back();
                caseRets.pop_back();
                for(auto c : caseRets)
                {
                    currentScope->constraints.push_back(std::make_pair(t1, c));
                }
                return t1;
            }
            return "";
        }
        case NODE_CASE:
        {
            auto c = std::static_pointer_cast<CaseNode>(n);
            if(c->caseExpr) typeInf(c->caseExpr, currentScope);
            if(c->caseStmt) return typeInf(c->caseStmt, currentScope);
            return "";
        }
        case NODE_RETURN:
        {
            auto ret = std::static_pointer_cast<ReturnStatementNode>(n);
            if(ret->returnExpr) return typeInf(ret->returnExpr, currentScope);
            else return "Void";
        }
        case NODE_BREAK:
        {
            return "";
        }
        case NODE_CONTINUE:
        {
            return "";
        }
        case NODE_BLOCK:
        {
            auto b = std::static_pointer_cast<BlockStatementNode>(n);
            std::vector<Ty> returnTypes;
            for(auto dec : b->declarations)
            {
                switch(dec->nodeType)
                {
                    case NODE_RETURN:
                    case NODE_IF:
                    case NODE_FOR:
                    case NODE_WHILE:
                    case NODE_SWITCH:
                    {
                        returnTypes.push_back(typeInf(dec, b->scope));
                        break;
                    }
                    default:
                    {
                        typeInf(dec, b->scope);
                        break;
                    }
                }
            }
            if(returnTypes.size() > 1)
            {
                for(size_t i = 1; i < returnTypes.size(); i++)
                {
                    b->scope->constraints.push_back(std::make_pair(returnTypes[i-1], returnTypes[i]));
                }
            }
            if(returnTypes.size() > 0) return returnTypes[0];
            else return "";
        }
        case NODE_ASSIGNMENT:
        {
            auto an = std::static_pointer_cast<AssignmentNode>(n);
            Ty t1 = typeInf(an->variable, currentScope);
            Ty t2 = typeInf(an->assignment, currentScope);
            currentScope->constraints.push_back(std::make_pair(t1, t2));
            return t1;
        }
        case NODE_BINARY:
        {
            //TODO: treat operators like functions and add constraints for operator types
            auto bn = std::static_pointer_cast<BinaryNode>(n);
            Ty t1 = typeInf(bn->expression1, currentScope);
            Ty t2 = typeInf(bn->expression2, currentScope);
            currentScope->constraints.push_back(std::make_pair(t1, t2));
            return t1;
        }
        case NODE_UNARY:
        {
            //TODO: treat operators like functions and add constraints for operator types
            auto un = std::static_pointer_cast<UnaryNode>(n);
            Ty t1 = typeInf(un->expression, currentScope);
            return t1;
        }
        case NODE_FUNCTIONCALL:
        {
            auto fc = std::static_pointer_cast<FunctionCallNode>(n);
            Ty funcType = typeInf(fc->called, currentScope);
            Ty result;
            std::vector<Ty> argTypes;
            for(auto arg : fc->args)
            {
                argTypes.push_back(typeInf(arg, currentScope));
            }
            Ty closureType;
            for(size_t i = 0; i < argTypes.size(); i++)
            {
                if(argTypes[i].compare("%Void") != 0)
                {
                    closureType.append(argTypes[i]);
                    closureType.append(" -> ");
                }
                else
                {
                    Ty placeholder = newGenericType();
                    result.append(placeholder);
                    result.append(" -> ");
                    closureType.append(placeholder);
                    closureType.append(" -> ");
                }
            }
            Ty returnType = newGenericType();
            closureType.append(returnType);
            result.append(returnType);
            currentScope->constraints.push_back(std::make_pair(funcType, closureType));

            return result;
        }
        case NODE_FIELDCALL:
        {
            auto field = std::static_pointer_cast<FieldCallNode>(n);
            Ty t1 = typeInf(field->expr, currentScope);
            if (t1.length() != 0 && *t1.begin() >= 'A' && *t1.begin() <= 'Z')
            {
                auto record = findInScope(t1, currentScope);
                switch(record->nodeType)
                {
                    case NODE_RECORDDECL:
                    {
                        auto rd = std::static_pointer_cast<RecordDeclarationNode>(record);
                        for(auto f : rd->fields)
                        {
                            if(tokencmp(f.identifier, field->field))
                            {
                                Ty t2 = f.type;
                                currentScope->nodeTVars.insert(std::make_pair(t2, field));
                                return t2;
                            }
                        }
                        break;
                    }
                    case NODE_TYPEDEF:
                    {
                        //TODO: unpack typedef
                    }
                    default:
                    {
                        //error
                    
                    }
                    
                }
            }
            else if (t1.length() != 0 && *t1.begin() >= 'a' && *t1.begin() <= 'z')
            {
                Ty t2 = newGenericType();
                std::vector<std::pair<Ty, Ty>> cases;
                auto s = currentScope;
                while(s != nullptr)
                {
                    auto range = s->fields.equal_range(tokenToString(field->field));
                    cases.reserve(std::distance(range.first, range.second));
                    for(auto it = range.first; it != range.second; it++)
                    {
                        cases.push_back(it->second);
                    }
                    s = s->parentScope;
                }
                std::vector<Ty> parentCases;
                std::vector<Ty> fieldCases;
                for(auto c : cases)
                {
                    parentCases.push_back(c.first);
                    fieldCases.push_back(c.second);
                }
                currentScope->soft_constraints.push_back(std::make_pair(t1, parentCases));
                currentScope->soft_constraints.push_back(std::make_pair(t2, fieldCases));
                return t2;
            }
            return newGenericType();
        }
        case NODE_ARRAYCONSTRUCTOR:
        {
            auto arr = std::static_pointer_cast<ArrayConstructorNode>(n);
            Ty previous = "";
            for(auto v : arr->values)
            {
                Ty curr = typeInf(v, currentScope);
                if(!previous.empty())
                {
                    currentScope->constraints.emplace_back(previous, curr);
                }
                previous = curr;
            }
            return previous.append("[]");
        }
        case NODE_ARRAYINDEX:
        {
            auto index = std::static_pointer_cast<ArrayIndexNode>(n);
            auto arrayType = typeInf(index->array, currentScope);
            Ty indexedType = newGenericType();
            currentScope->constraints.emplace_back(std::string(indexedType).append("[]"), arrayType);
            return indexedType;
        }
        case NODE_IDENTIFIER:
        {
            auto var = std::static_pointer_cast<VariableNode>(n);
            auto node = findInScope(tokenToString(var->variable), currentScope);
            if(node != nullptr)
            {
                switch(node->nodeType)
                {
                    case NODE_VARIABLEDECL:
                    {
                        auto vd = std::static_pointer_cast<VariableDeclarationNode>(node);
                        return vd->type;
                    }
                    case NODE_FUNCTIONDECL:
                    {
                        auto fd = std::static_pointer_cast<FunctionDeclarationNode>(node);
                        return functionTypeFromFunction(fd);
                    }
                    default:
                    {
                        //error
                        return "";
                    }
                }
            }
            return "";
        }
        case NODE_LAMBDA:
        {
            auto l = std::static_pointer_cast<LambdaNode>(n);
            Ty ret = typeInf(l->body, currentScope);
            currentScope->constraints.push_back(std::make_pair(ret, l->returnType));
            Ty result;
            for(auto p : l->params)
            {
                result.append(p.type);
                result.append(" -> ");
            }
            result.append(l->returnType);
            currentScope->nodeTVars.insert(std::make_pair(result, n));
            return result;
        }
        case NODE_LITERAL:
        {
            auto lit = std::static_pointer_cast<LiteralNode>(n);
            switch(lit->value.type)
            {
                case TRUE:
                case FALSE: return "Bool";
	            case FLOAT: return "Float";
                case DOUBLE: return "Double";
                case CHAR: return "Char";
                case INT:
                case HEX_INT:
                case OCT_INT:
                case BIN_INT: return "Int";
                case STRING: return "String";
                default:
                    //error
                    return "";
            }
        }
        case NODE_LISTINIT:
        {
            auto init = std::static_pointer_cast<ListInitNode>(n);
            std::vector<Token> initType;
            {
                Lexer l = initLexer(init->type.c_str());
                Token t = scanToken(&l);
                while(t.type != EOF_)
                {
                    initType.push_back(t);
                    t = scanToken(&l);
                }
            }
            std::vector<Token> resultType;
            
            return "";
        }
        case NODE_TUPLE:
        {
            auto tuple = std::static_pointer_cast<TupleConstructorNode>(n);
            Ty t = "(";
            bool first = true;
            for(auto v : tuple->values)
            {
                if(first == false) t.append(", ");
                first = false;
                auto vType = typeInf(v, currentScope);
                t.append(vType);
            }
            t.append(")");
            currentScope->nodeTVars.insert(std::make_pair(t, n));
            return t;
        }
        case NODE_PLACEHOLDER:
        {
            return "%Void";
        }
        default:
        {
            system("pause");
            return "";
        }
    }
}

void ImplKinds(std::shared_ptr<node> n, std::shared_ptr<ScopeNode> currentScope)
{
    assert(n != nullptr);
    assert(currentScope != nullptr);
    switch(n->nodeType)
    {
        case NODE_CLASSIMPL:
        {
            auto ci = std::static_pointer_cast<ClassImplementationNode>(n);
            std::shared_ptr<ClassDeclarationNode> c = nullptr;
            std::shared_ptr<RecordDeclarationNode> r = nullptr;
            auto s = currentScope;
            while((c == nullptr || r == nullptr) && s != nullptr)
            {
                if(s->classes.find(tokenToString(ci->_class)) != s->classes.end())
                {
                    c = s->classes.at(tokenToString(ci->_class));
                }
                if(s->types.find(ci->implemented) != s->types.end())
                {
                    r = s->types.at(ci->implemented);
                }

                s = s->parentScope;
            }
            if(r == nullptr || c == nullptr)
            {
                system("pause");
                assert(false);
            }
            if(!typesEqual(c->kind, r->kind))
            {
                //error
                system("pause");
                assert(false);
            }
            break;
        }
        case NODE_RECORDDECL:
        {
            auto rd = std::static_pointer_cast<RecordDeclarationNode>(n);
            break;
        }
        case NODE_VARIABLEDECL:
        {
            auto vd = std::static_pointer_cast<VariableDeclarationNode>(n);
            ImplKinds(vd->value, currentScope);
            break;
        }
        case NODE_BLOCK:
        {
            auto block = std::static_pointer_cast<BlockStatementNode>(n);
            for(auto dec : block->declarations)
            {
                ImplKinds(dec, block->scope);
            }
            break;
        }
        case NODE_FUNCTIONDECL:
        {
            auto fd = std::static_pointer_cast<FunctionDeclarationNode>(n);
            if(fd->body) ImplKinds(fd->body, currentScope);
            break;
        }
        case NODE_IF:
        {
            auto _if = std::static_pointer_cast<IfStatementNode>(n);
            ImplKinds(_if->thenStmt, currentScope);
            if(_if->elseStmt != nullptr)
            {
                ImplKinds(_if->elseStmt, currentScope);
            }
            break;
        }
        case NODE_FOR:
        {
            auto _for = std::static_pointer_cast<ForStatementNode>(n);
            ImplKinds(_for->loopStmt, currentScope);
            break;
        }
        case NODE_WHILE:
        {
            auto _while = std::static_pointer_cast<WhileStatementNode>(n);
            ImplKinds(_while->loopStmt, currentScope);
            break;
        }
        case NODE_SWITCH:
        {
            auto sw = std::static_pointer_cast<SwitchStatementNode>(n);
            ImplKinds(sw->switchExpr, currentScope);
            for(auto c : sw->cases)
            {
                ImplKinds(c, currentScope);
            }
            break;
        }
        case NODE_CASE:
        {
            auto cn = std::static_pointer_cast<CaseNode>(n);
            ImplKinds(cn->caseExpr, currentScope);
            ImplKinds(cn->caseStmt, currentScope);
            break;
        }
        case NODE_RETURN:
        {
            auto r = std::static_pointer_cast<ReturnStatementNode>(n);
            ImplKinds(r->returnExpr, currentScope);
            break;
        }
        case NODE_ASSIGNMENT:
        {
            auto a = std::static_pointer_cast<AssignmentNode>(n);
            ImplKinds(a->assignment, currentScope);
            break;
        }
        case NODE_LAMBDA:
        {
            auto l = std::static_pointer_cast<LambdaNode>(n);
            ImplKinds(l->body, currentScope);
            break;
        }
        case NODE_FUNCTIONCALL:
        {
            auto fc = std::static_pointer_cast<FunctionCallNode>(n);
            for(auto arg : fc->args)
            {
                ImplKinds(arg, currentScope);
            }
            break;
        }
        case NODE_ARRAYCONSTRUCTOR:
        {
            auto ac = std::static_pointer_cast<ArrayConstructorNode>(n);
            for(auto value : ac->values)
            {
                ImplKinds(value, currentScope);
            }
            break;
        }
        case NODE_LISTINIT:
        {
            auto li = std::static_pointer_cast<ListInitNode>(n);
            for(auto value : li->values)
            {
                ImplKinds(value, currentScope);
            }
            break;
        }
        case NODE_FIELDCALL:
        {
            auto fc = std::static_pointer_cast<FieldCallNode>(n);
            ImplKinds(fc->expr, currentScope);
            break;
        }
        case NODE_BINARY:
        {
            auto bn = std::static_pointer_cast<BinaryNode>(n);
            ImplKinds(bn->expression1, currentScope);
            ImplKinds(bn->expression2, currentScope);
            break;
        }
        case NODE_UNARY:
        {
            auto un = std::static_pointer_cast<UnaryNode>(n);
            ImplKinds(un->expression, currentScope);
        }
        default:
            break;
    }
}

void RecordKinds(std::shared_ptr<node> n, std::shared_ptr<ScopeNode> currentScope)
{
    assert(n != nullptr);
    assert(currentScope != nullptr);
    switch(n->nodeType)
    {
        case NODE_CLASSIMPL:
        {
            break;
        }
        case NODE_RECORDDECL:
        {
            auto rd = std::static_pointer_cast<RecordDeclarationNode>(n);
            if(rd->kind.size() == 0)
            {
                Lexer l = initLexer(rd->typeDefined.c_str());
                Token t = scanToken(&l);
                const char* type = "*";
                const char* arrow = "->";

                while(t.type != EOF_)
                { 
                    if(t.type == IDENTIFIER)
                    {
                        rd->kind.append(type).append(" ").append(arrow).append(" ");
                    }
                    t = scanToken(&l);
                }
                rd->kind.append(type);
            }
            break;
        }
        case NODE_VARIABLEDECL:
        {
            auto vd = std::static_pointer_cast<VariableDeclarationNode>(n);
            RecordKinds(vd->value, currentScope);
            break;
        }
        case NODE_BLOCK:
        {
            auto block = std::static_pointer_cast<BlockStatementNode>(n);
            for(auto dec : block->declarations)
            {
                RecordKinds(dec, block->scope);
            }
            break;
        }
        case NODE_FUNCTIONDECL:
        {
            auto fd = std::static_pointer_cast<FunctionDeclarationNode>(n);
            if(fd->body) RecordKinds(fd->body, currentScope);
            break;
        }
        case NODE_IF:
        {
            auto _if = std::static_pointer_cast<IfStatementNode>(n);
            RecordKinds(_if->thenStmt, currentScope);
            if(_if->elseStmt != nullptr)
            {
                RecordKinds(_if->elseStmt, currentScope);
            }
            break;
        }
        case NODE_FOR:
        {
            auto _for = std::static_pointer_cast<ForStatementNode>(n);
            RecordKinds(_for->loopStmt, currentScope);
            break;
        }
        case NODE_WHILE:
        {
            auto _while = std::static_pointer_cast<WhileStatementNode>(n);
            RecordKinds(_while->loopStmt, currentScope);
            break;
        }
        case NODE_SWITCH:
        {
            auto sw = std::static_pointer_cast<SwitchStatementNode>(n);
            RecordKinds(sw->switchExpr, currentScope);
            for(auto c : sw->cases)
            {
                RecordKinds(c, currentScope);
            }
            break;
        }
        case NODE_CASE:
        {
            auto cn = std::static_pointer_cast<CaseNode>(n);
            RecordKinds(cn->caseExpr, currentScope);
            RecordKinds(cn->caseStmt, currentScope);
            break;
        }
        case NODE_RETURN:
        {
            auto r = std::static_pointer_cast<ReturnStatementNode>(n);
            RecordKinds(r->returnExpr, currentScope);
            break;
        }
        case NODE_ASSIGNMENT:
        {
            auto a = std::static_pointer_cast<AssignmentNode>(n);
            RecordKinds(a->assignment, currentScope);
            break;
        }
        case NODE_LAMBDA:
        {
            auto l = std::static_pointer_cast<LambdaNode>(n);
            RecordKinds(l->body, currentScope);
            break;
        }
        case NODE_FUNCTIONCALL:
        {
            auto fc = std::static_pointer_cast<FunctionCallNode>(n);
            for(auto arg : fc->args)
            {
                RecordKinds(arg, currentScope);
            }
            break;
        }
        case NODE_ARRAYCONSTRUCTOR:
        {
            auto ac = std::static_pointer_cast<ArrayConstructorNode>(n);
            for(auto value : ac->values)
            {
                RecordKinds(value, currentScope);
            }
            break;
        }
        case NODE_LISTINIT:
        {
            auto li = std::static_pointer_cast<ListInitNode>(n);
            for(auto value : li->values)
            {
                RecordKinds(value, currentScope);
            }
            break;
        }
        case NODE_FIELDCALL:
        {
            auto fc = std::static_pointer_cast<FieldCallNode>(n);
            RecordKinds(fc->expr, currentScope);
            break;
        }
        case NODE_BINARY:
        {
            auto bn = std::static_pointer_cast<BinaryNode>(n);
            RecordKinds(bn->expression1, currentScope);
            RecordKinds(bn->expression2, currentScope);
            break;
        }
        case NODE_UNARY:
        {
            auto un = std::static_pointer_cast<UnaryNode>(n);
            RecordKinds(un->expression, currentScope);
        }
        default:
            break;
    }
}

void ResolveTypeclasses(std::shared_ptr<node>n, std::shared_ptr<ScopeNode> currentScope)
{
    if(n == nullptr || currentScope == nullptr) return;
    switch(n->nodeType)
    {
        case NODE_CLASSDECL:
        {
            //assume single-type typeclasses, rewrite if that changes
            auto classdecl = std::static_pointer_cast<ClassDeclarationNode>(n);
            for(auto f : classdecl->functions)
            {
                ResolveTypeclasses(f, currentScope);
                if(f->nodeType != NODE_FUNCTIONDECL)
                {
                    //error
                    system("pause");
                    assert(false);
                }
                auto fd = std::static_pointer_cast<FunctionDeclarationNode>(f);
                Ty functionType = functionTypeFromFunction(fd);
                Token tvar;
                {
                    Lexer l = initLexer(classdecl->className.c_str());
                    tvar = scanToken(&l);
                }
                if(tvar.type != IDENTIFIER)
                {
                    //error
                    system("pause");
                    assert(false);
                }
                std::vector<int> argCounts;
                Lexer l = initLexer(functionType.c_str());
                Token current = scanToken(&l);
                while(current.type != EOF_)
                {
                    if(tokencmp(current, tvar))
                    {
                        int argcount = 0;
                        while(current.type != ARROW || current.type != EOF_)
                        {
                            current = scanToken(&l);
                            if(current.type == IDENTIFIER)
                            {
                                argcount++;
                            }
                        }
                        argCounts.push_back(argcount);
                    }
                }
                int args = -1;
                for(auto a : argCounts)
                {
                    if(args == -1) args = a;
                    else if(a != args)
                    {
                        //error
                        system("pause");
                        assert(false);
                    }
                }
                Ty kind;
                const char* type = "*";
                const char* arrow = "->";
                for(int j = 0; j < args; j++)
                {
                    kind.append(type).append(" ").append(arrow).append(" ");
                }
                kind.append(type);
                classdecl->kind = kind;
            }
            break;
        }
        case NODE_CLASSIMPL:
        {
            auto ci = std::static_pointer_cast<ClassImplementationNode>(n);
            for(auto f : ci->functions)
            {
                ResolveTypeclasses(f, currentScope);
            }
            break;
        }
        case NODE_VARIABLEDECL:
        {
            auto vd = std::static_pointer_cast<VariableDeclarationNode>(n);
            ResolveTypeclasses(vd->value, currentScope);
            break;
        }
        case NODE_BLOCK:
        {
            auto block = std::static_pointer_cast<BlockStatementNode>(n);
            for(auto dec : block->declarations)
            {
                ResolveTypeclasses(dec, block->scope);
            }
            break;
        }
        case NODE_FUNCTIONDECL:
        {
            auto fd = std::static_pointer_cast<FunctionDeclarationNode>(n);
            ResolveTypeclasses(fd->body, currentScope);
            break;
        }
        case NODE_IF:
        {
            auto _if = std::static_pointer_cast<IfStatementNode>(n);
            ResolveTypeclasses(_if->thenStmt, currentScope);
            if(_if->elseStmt != nullptr)
            {
                ResolveTypeclasses(_if->elseStmt, currentScope);
            }
            break;
        }
        case NODE_FOR:
        {
            auto _for = std::static_pointer_cast<ForStatementNode>(n);
            ResolveTypeclasses(_for->loopStmt, currentScope);
            break;
        }
        case NODE_WHILE:
        {
            auto _while = std::static_pointer_cast<WhileStatementNode>(n);
            ResolveTypeclasses(_while->loopStmt, currentScope);
            break;
        }
        case NODE_SWITCH:
        {
            auto sw = std::static_pointer_cast<SwitchStatementNode>(n);
            ResolveTypeclasses(sw->switchExpr, currentScope);
            for(auto c : sw->cases)
            {
                ResolveTypeclasses(c, currentScope);
            }
            break;
        }
        case NODE_CASE:
        {
            auto cn = std::static_pointer_cast<CaseNode>(n);
            ResolveTypeclasses(cn->caseExpr, currentScope);
            ResolveTypeclasses(cn->caseStmt, currentScope);
            break;
        }
        case NODE_RETURN:
        {
            auto r = std::static_pointer_cast<ReturnStatementNode>(n);
            ResolveTypeclasses(r->returnExpr, currentScope);
            break;
        }
        case NODE_ASSIGNMENT:
        {
            auto a = std::static_pointer_cast<AssignmentNode>(n);
            ResolveTypeclasses(a->assignment, currentScope);
            break;
        }
        case NODE_LAMBDA:
        {
            auto l = std::static_pointer_cast<LambdaNode>(n);
            ResolveTypeclasses(l->body, currentScope);
            break;
        }
        case NODE_FUNCTIONCALL:
        {
            auto fc = std::static_pointer_cast<FunctionCallNode>(n);
            for(auto arg : fc->args)
            {
                ResolveTypeclasses(arg, currentScope);
            }
            break;
        }
        case NODE_ARRAYCONSTRUCTOR:
        {
            auto ac = std::static_pointer_cast<ArrayConstructorNode>(n);
            for(auto value : ac->values)
            {
                ResolveTypeclasses(value, currentScope);
            }
            break;
        }
        case NODE_LISTINIT:
        {
            auto li = std::static_pointer_cast<ListInitNode>(n);
            for(auto value : li->values)
            {
                ResolveTypeclasses(value, currentScope);
            }
            break;
        }
        case NODE_FIELDCALL:
        {
            auto fc = std::static_pointer_cast<FieldCallNode>(n);
            ResolveTypeclasses(fc->expr, currentScope);
            break;
        }
        case NODE_BINARY:
        {
            auto bn = std::static_pointer_cast<BinaryNode>(n);
            ResolveTypeclasses(bn->expression1, currentScope);
            ResolveTypeclasses(bn->expression2, currentScope);
            break;
        }
        case NODE_UNARY:
        {
            auto un = std::static_pointer_cast<UnaryNode>(n);
            ResolveTypeclasses(un->expression, currentScope);
            break;
        }
        default:
            break;
    }
}

void printConstraints(std::weak_ptr<ScopeNode> scope)
{
    if(auto shared = scope.lock())
    {
        for(auto c : shared->constraints)
        {
            std::cout << "Constraint: " << c.first << " == " << c.second << "\n";
        }
        for(auto child : shared->childScopes)
        {
            printConstraints(child);
        }
    }
}

std::deque<std::pair<Ty, Ty>> flattenConstraints(std::shared_ptr<ScopeNode> scope)
{
    auto result = scope->constraints;
    for(auto child : scope->childScopes)
    {
        if(auto s = child.lock())
        {
            result.insert(result.end(), s->constraints.begin(), s->constraints.end());
        }
    }
    return result;
}

std::deque<std::pair<Ty, Ty>> resolveConstraints(std::deque<std::pair<Ty, Ty>> constraints)
{
    std::deque<std::pair<Ty, Ty>> result;
    while(constraints.size() > 0)
    {
        auto constraint = constraints.front();
        constraints.pop_front();
        if(constraint.first.compare(constraint.second) == 0) continue;
        if(isupper(constraint.first.front()) && isupper(constraint.second.front()))
        {
            printf("error: type %s is not equal to %s!", constraint.first.c_str(), constraint.second.c_str());
            return {};
        }
        else if (isFunctionType(constraint.first) && isFunctionType(constraint.second))
        {
            auto t1 = firstParameterFromFunctionType(constraint.first);
            auto t2 = firstParameterFromFunctionType(constraint.second);
            constraints.push_back(std::make_pair(t1, t2));
            t1 = constraint.first.substr(constraint.first.find(" -> ")+4, constraint.first.length());
            t2 = constraint.second.substr(constraint.second.find(" -> ")+4, constraint.second.length());
            constraints.push_back(std::make_pair(t1, t2));
        }
        else
        {
            auto t1 = constraint.first;
            auto t2 = constraint.second;
            Ty replacing, replaced;
            if(isupper(t1.front()))
            {
                replacing = t1;
                replaced = t2;
            }            
            else
            {
                replacing = t2;
                replaced = t1;
            }
            result.push_back(std::make_pair(replaced, replacing));

            for(auto& constraint : constraints)
            {
                auto first = functionTypeSplit(constraint.first);
                auto second = functionTypeSplit(constraint.second);
                for(auto i = 0; i < first.size(); i++)
                {
                    if(first[i].compare(replaced) == 0)
                    {
                        first[i] = replacing;
                    }
                }
                for(auto i = 0; i < second.size(); i++)
                {
                    if(second[i].compare(replaced) == 0)
                    {
                        second[i] = replacing;
                    }
                }
                Ty new1;
                Ty new2;
                bool f = true;
                for(auto str : first)
                {
                    if(!f) new1.append(" ");
                    f = false;
                    new1.append(str);
                }
                f = true;
                for(auto str : second)
                {
                    if(!f) new2.append(" ");
                    f = false;
                    new2.append(str);
                }
                constraint = std::make_pair(new1, new2);
            }
        }
    }
    return result;
}


std::shared_ptr<ProgramNode> analyze(const char *src)
{
    std::shared_ptr<ProgramNode> ast = parse(src);
    if(ast->hadError) return nullptr;
    
    for (auto dec : ast->declarations)
    {
        RecordKinds(dec, ast->globalScope);
    }
    for (auto dec : ast->declarations)
    {
        ResolveTypeclasses(dec, ast->globalScope);
    }
    for (auto dec : ast->declarations)
    {
        ImplKinds(dec, ast->globalScope);
    }
    for (auto dec : ast->declarations)
    {
        typeInf(dec, ast->globalScope);
    }
    if(!ast->hadError)
    {
        printConstraints(ast->globalScope);
        auto constraints = flattenConstraints(ast->globalScope);
        auto substitutions = resolveConstraints(constraints);
        if(substitutions.empty()) ast->hadError = true;
        for(auto substitution : substitutions)
        {
            std::cout << "Substitution: " << substitution.first << " => " << substitution.second << "\n";
        }

        return ast;
    }
    return nullptr;
}
