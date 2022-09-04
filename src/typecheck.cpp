#include "typecheck.h"
#include <cassert>
#include <iostream>
namespace pilaf
{
    static bool typecheckError = false;
    //expects that the first argument is a basic type, second argument is an applied type

    std::shared_ptr<ScopeNode> namespaceScope(std::shared_ptr<node> expr, std::shared_ptr<ScopeNode> currentScope)
    {
        switch(expr->nodeType)
        {
            case NODE_NAMESPACE:
            {
                auto n = std::static_pointer_cast<NamespaceNode>(expr);
                if(currentScope->namespaces.find(tokenToString(n->name)) != currentScope->namespaces.end())
                {
                    auto scope = currentScope->namespaces.at(tokenToString(n->name));
                    auto result = namespaceScope(n->expr, scope);
                    if(result == nullptr) return scope;
                    else return result;
                }
            }
            default: return nullptr;
        }
    }

    std::shared_ptr<Ty> getGenericType(std::shared_ptr<Ty> name, std::shared_ptr<Ty> applied)
    {
        assert(name->type == Ty::TY_BASIC || name->type == Ty::TY_VAR);
        assert(applied->type == Ty::TY_APPLICATION);

        auto n = std::static_pointer_cast<TyBasic>(name);
        auto app = std::static_pointer_cast<TyAppl>(applied);
        for(auto v : app->vars)
        {
            assert(v->type == Ty::TY_BASIC || v->type == Ty::TY_VAR);
            if(typesEqual(name, v))
            {
                return applied;
            }
        }
        return nullptr;
    }

    bool hadError() { return typecheckError; }
    
    void error(size_t line, std::string_view printable, std::string_view highlighted, const char* msg)
    {
        typecheckError = true;
        size_t column = 0;
        std::cerr << msg << '\n';
        auto pos = printable.find(highlighted);
        if(pos == std::string::npos) return;
        std::cerr << printable.substr(0, pos);
        auto last = printable.substr(0, pos).rfind("\n");
        if(last == std::string::npos) last = 0;
        auto distance = pos - last;
        auto substr = highlighted;
        bool errOnNext = false;
        while(substr.size() != 0)
        {
            auto pos2 = substr.find('\n');
            if(pos2 == std::string::npos) 
            {
                std::cerr << substr;
                substr = "";
                errOnNext = true;
            }
            else
            {
                std::cerr << substr.substr(0, pos2+1);
                substr = substr.substr(pos2+1);
                std::cerr << std::string(distance, ' ') << std::string(pos2, '^') << '\n';
                distance = 0;
            }
        }
        auto pos3 = printable.rfind(highlighted);
        auto rest = printable.substr(pos3 + highlighted.length());
        while(rest.size() != 0 && errOnNext == true)
        {
            auto pos2 = rest.find('\n');
            if(pos2 == std::string::npos)
            {
                std::cerr << rest << '\n';
                rest = "";
                std::cerr << std::string(distance, ' ') << std::string(highlighted.rfind("\n") == std::string::npos ? highlighted.length() : highlighted.length() - (highlighted.rfind("\n") + 1), '^') << '\n';
                errOnNext = false;
            }
            else
            {
                std::cerr << rest.substr(0, pos2) << '\n';
                std::cerr << std::string(distance, ' ') << std::string(highlighted.rfind("\n") == std::string::npos ? highlighted.length() : highlighted.length() - (highlighted.rfind("\n") + 1), '^');
                rest = rest.substr(pos2);
                errOnNext = false;
            }
        }
        std::cerr << rest;
    }
    
    std::vector<std::shared_ptr<Ty>> functionTypeSplit(std::shared_ptr<Ty> type)
    {
        assert(type->type == Ty::TY_FUNCTION);
        std::vector<std::shared_ptr<Ty>> result;
        auto recurse = type;
    
        while(recurse->type == Ty::TY_FUNCTION)
        {
            auto fn = std::static_pointer_cast<TyFunc>(recurse);
            result.push_back(fn->in);
            recurse = fn->out;
        }
        result.push_back(recurse);
        return result;
    }
    
    std::shared_ptr<Ty> functionTypeFromFunction(std::shared_ptr<FunctionDeclarationNode> f)
    {
        std::shared_ptr<Ty> result;
        result = f->returnType;
        for(auto it = f->params.rbegin(); it != f->params.rend(); it++)
        {
            auto out = result;
            auto in = it->type;
            auto fn = std::make_shared<TyFunc>(in, out);
            result = fn;
        }
        return result;
    }
    
    std::shared_ptr<node> findInScope(std::string toFind, std::shared_ptr<ScopeNode> scope)
    {
        Lexer l = initLexer(toFind.c_str());
        Token t = scanToken(&l);
        switch(t.type)
        {
            case TokenTypes::TYPE:
            {
                auto s = scope;
                while(s != nullptr)
                {
                    if(s->structs.find(tokenToString(t)) != s->structs.end())
                    {
                        return s->structs.at(tokenToString(t));
                    }
                    else if(s->unions.find(tokenToString(t)) != s->unions.end())
                    {
                        return s->unions.at(tokenToString(t));
                    }
                    else if(s->typeAliases.find(tokenToString(t)) != s->typeAliases.end())
                    {
                        return s->typeAliases.at(tokenToString(t));
                    }
                    else if(s->classes.find(tokenToString(t)) != s->classes.end())
                    {
                        return s->classes.at(tokenToString(t));
                    }
                    else if(s->tyCons.find(tokenToString(t)) != s->tyCons.end())
                    {
                        return s->tyCons.at(tokenToString(t));
                    }
                    else
                    {
                        s = s->parentScope;
                    }
                }
                break;
            }
            case TokenTypes::IDENTIFIER:
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
            default:
                return nullptr;
        }
        return nullptr;
    }
    
    bool isFunctionType(std::shared_ptr<Ty> type)
    {
        if(type->type == Ty::TY_FUNCTION) return true;
        else return false;
    }

    std::shared_ptr<Ty> returnTypeFromFunctionType(std::shared_ptr<Ty> type)
    {
        assert(type->type == Ty::TY_FUNCTION);
        auto recurse = type;
        while(recurse->type == Ty::TY_FUNCTION)
        {
            auto fn = std::static_pointer_cast<TyFunc>(recurse);
            recurse = fn->out;
        }
        return recurse;
    }

    std::shared_ptr<Ty> firstParameterFromFunctionType(std::shared_ptr<Ty> type)
    {
        assert(type->type == Ty::TY_FUNCTION);
        auto fn = std::static_pointer_cast<TyFunc>(type);
        return fn->in;
    }

    std::shared_ptr<Ty> typeInf(std::shared_ptr<node> n, std::shared_ptr<ScopeNode> currentScope)
    {
        assert(n != nullptr);
        assert(currentScope != nullptr);
        switch(n->nodeType)
        {
            case NODE_TYPEDEF:
            {
                auto td = std::static_pointer_cast<TypedefNode>(n);
                std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                currentScope->constraints.push_back(std::make_pair(generic(td->typeDefined, map1), generic(td->typeAliased, map2)));
                return td->typeDefined;
            }
            case NODE_MODULE:
            {
                auto md = std::static_pointer_cast<ModuleDeclarationNode>(n);
                if(md->block)
                {
                    auto blockType = typeInf(md->block, md->scope);
                    return blockType;
                }
                return nullptr;
            }
            case NODE_STRUCTDECL:
            case NODE_UNIONDECL:
            {
                return nullptr;
            }
            case NODE_FUNCTIONDECL:
            {
                auto fd = std::static_pointer_cast<FunctionDeclarationNode>(n);
                currentScope->nodeTVars.insert(std::make_pair(functionTypeFromFunction(fd), fd));
                if(fd->body) 
                {
                    auto bodyType = typeInf(fd->body, currentScope);
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                    if(fd->returnType == nullptr) fd->returnType = bodyType;
                    else currentScope->constraints.push_back(std::make_pair(generic(fd->returnType, map1), generic(bodyType, map2)));
                }
                return functionTypeFromFunction(fd);
            }
            case NODE_VARIABLEDECL:
            {
                auto vd = std::static_pointer_cast<VariableDeclarationNode>(n);
                if(isRefutable(vd->assigned)) error(0, std::string_view(vd->start, vd->end - vd->start), std::string_view(vd->assigned->start, vd->assigned->end - vd->assigned->start), "variable declaration cannot assign to a refutable pattern!");
                if(vd->identifiers.empty()) error(0, std::string_view(vd->start, vd->end - vd->start), std::string_view(vd->assigned->start, vd->assigned->end - vd->assigned->start), "variable declaration must have an identifier to assign to!");
                for(auto id : vd->identifiers)
                {
                    auto s = currentScope;
                    while(s != nullptr)
                    {
                        if(s->variables.find(id.first) != s->variables.end())
                        {
                            break;//error
                        }
                        else s = s->parentScope;
                    }
                    if(s == nullptr) currentScope->variables.insert(std::make_pair(id.first, vd));
                }
                auto assignedType = typeInf(vd->assigned, currentScope);
                {
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                    currentScope->constraints.push_back(std::make_pair(generic(vd->type, map1), generic(assignedType, map2)));
                }
                currentScope->nodeTVars.insert(std::make_pair(vd->type, vd));
                if(vd->value)
                {
                    auto valueType = typeInf(vd->value, currentScope);
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                    if(valueType != nullptr) currentScope->constraints.push_back(std::make_pair(generic(vd->type, map1), generic(valueType, map2)));
                }
                return vd->type;
            }
            case NODE_CLASSDECL:
            {
                return nullptr;
            }
            case NODE_CLASSIMPL:
            {
                return nullptr;
            }
            case NODE_FOR:
            {
                //TODO: for, if, while, switch should have their own scopes
                auto _for = std::static_pointer_cast<ForStatementNode>(n);
                if(_for->initExpr) typeInf(_for->initExpr, currentScope);
                if(_for->condExpr) typeInf(_for->condExpr, currentScope);
                if(_for->incrementExpr) typeInf(_for->incrementExpr, currentScope);
                if(_for->loopStmt) typeInf(_for->loopStmt, currentScope);
                return nullptr;
            }
            case NODE_IF:
            {
                auto _if = std::static_pointer_cast<IfStatementNode>(n);
                typeInf(_if->branchExpr, currentScope);
                std::shared_ptr<Ty> t1, t2;
                if(_if->thenStmt) t1 = typeInf(_if->thenStmt, currentScope);
                if(_if->elseStmt) t2 = typeInf(_if->elseStmt, currentScope);
                if(t1 == nullptr) return t2;
                else if (t2 == nullptr) return t1;
                else 
                {
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                    std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                    currentScope->constraints.push_back(std::make_pair(generic(t1, map1), generic(t2, map2)));
                    return t1;
                }
            }
            case NODE_WHILE:
            {
                auto _while = std::static_pointer_cast<WhileStatementNode>(n);
                typeInf(_while->loopExpr, currentScope);
                if(_while->loopStmt) typeInf(_while->loopStmt, currentScope);
                return nullptr;
            }
            case NODE_SWITCH:
            {
                auto sw = std::static_pointer_cast<SwitchStatementNode>(n);
                if(sw->switchExpr) typeInf(sw->switchExpr, currentScope);
                std::vector<std::shared_ptr<Ty>> caseRets;
                for(auto c : sw->cases)
                {
                    auto _case = std::static_pointer_cast<CaseNode>(c);
                    std::shared_ptr<Ty> ret = typeInf(_case, _case->scope);
                    if(ret != nullptr)
                    {
                        caseRets.push_back(ret);
                    }
                }
                if(caseRets.size() != 0)
                {
                    std::shared_ptr<Ty> t1 = caseRets.back();
                    caseRets.pop_back();
                    for(auto c : caseRets)
                    {
                        std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                        std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                        currentScope->constraints.push_back(std::make_pair(generic(t1, map1), generic(c, map2)));
                    }
                    return t1;
                }
                return nullptr;
            }
            case NODE_CASE:
            {
                auto c = std::static_pointer_cast<CaseNode>(n);
                if(c->caseExpr) typeInf(c->caseExpr, currentScope);
                if(c->caseStmt) return typeInf(c->caseStmt, currentScope);
                return nullptr;
            }
            case NODE_RETURN:
            {
                auto ret = std::static_pointer_cast<ReturnStatementNode>(n);
                if(ret->returnExpr) return typeInf(ret->returnExpr, currentScope);
                else {
                    auto t = std::make_shared<TyBasic>("Void");
                    return t;
                }
            }
            case NODE_BREAK:
            {
                return nullptr;
            }
            case NODE_CONTINUE:
            {
                return nullptr;
            }
            case NODE_BLOCK:
            {
                auto b = std::static_pointer_cast<BlockStatementNode>(n);
                std::vector<std::shared_ptr<Ty>> returnTypes;
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
                        std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                        std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                        b->scope->constraints.push_back(std::make_pair(generic(returnTypes[i-1], map1), generic(returnTypes[i], map2)));
                    }
                }
                if(returnTypes.size() > 0) return returnTypes[0];
                else return nullptr;
            }
            case NODE_ASSIGNMENT:
            {
                auto an = std::static_pointer_cast<AssignmentNode>(n);
                std::shared_ptr<Ty> t1 = typeInf(an->variable, currentScope);
                std::shared_ptr<Ty> t2 = typeInf(an->assignment, currentScope);
                std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                currentScope->constraints.push_back(std::make_pair(generic(t1, map1), generic(t2, map2)));
                return t1;
            }
            case NODE_BINARY:
            {
                //TODO: treat operators like functions and add constraints for operator types
                auto bn = std::static_pointer_cast<BinaryNode>(n);
                std::shared_ptr<Ty> t1 = typeInf(bn->expression1, currentScope);
                std::shared_ptr<Ty> t2 = typeInf(bn->expression2, currentScope);
                std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                currentScope->constraints.push_back(std::make_pair(generic(t1, map1), generic(t2, map2)));
                return t1;
            }
            case NODE_UNARY:
            {
                //TODO: treat operators like functions and add constraints for operator types
                auto un = std::static_pointer_cast<UnaryNode>(n);
                std::shared_ptr<Ty> t1 = typeInf(un->expression, currentScope);
                return t1;
            }
            case NODE_FUNCTIONCALL:
            {
                //TODO: add support for type constructors
                auto fc = std::static_pointer_cast<FunctionCallNode>(n);
                std::shared_ptr<Ty> returnType = newGenericType();
                std::shared_ptr<Ty> funcType = typeInf(fc->called, currentScope);
                std::shared_ptr<Ty> result = returnType;
                std::vector<std::shared_ptr<Ty>> argTypes;
                for(auto arg : fc->args)
                {
                    argTypes.push_back(typeInf(arg, currentScope));
                }
                auto closureType = returnType;
                for(auto it = argTypes.rbegin(); it != argTypes.rend(); it++)
                {
                    if((*it)->type != Ty::TY_BASIC || std::static_pointer_cast<TyBasic>(*it)->t.compare("%Void") != 0)
                    {
                        auto next = std::make_shared<TyFunc>(*it, closureType);
                        closureType = next;
                    }
                    //else
                    //{
                    //    std::shared_ptr<Ty> placeholder = newGenericType();
                    //    auto newResult = std::make_shared<TyFunc>(placeholder, result);
                    //    auto newClosure = std::make_shared<TyFunc>(placeholder, closureType);
                    //    closureType = newClosure;
                    //}
                }
                std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                currentScope->constraints.push_back(std::make_pair(generic(funcType, map1), generic(closureType, map2)));
    
                return result;
            }
            case NODE_FIELDCALL:
            {
                auto field = std::static_pointer_cast<FieldCallNode>(n);
                std::shared_ptr<Ty> t1 = typeInf(field->expr, currentScope);
                if(currentScope->fields.find(tokenToString(field->field)) != currentScope->fields.end())
                {
                    auto range = currentScope->fields.equal_range(tokenToString(field->field));
                    for(auto it = range.first; it != range.second; it++)
                    {
                        if(typesEqual(it->second.first, t1))
                        {
                            return it->second.second;
                        }
                    }
                    error(field->field.line, std::string_view(field->start, field->end - field->start), std::string_view(field->field.start, field->field.length), "Field name could not be found for type!");
                }
                
                return newGenericType();
            }
            case NODE_ARRAYCONSTRUCTOR:
            {
                auto arr = std::static_pointer_cast<ArrayConstructorNode>(n);
                std::shared_ptr<Ty> previous = nullptr;
                bool hasEllipse = false;
                for(auto v : arr->values)
                {
                    if(v->nodeType != NODE_ELLIPSE)
                    {
                        std::shared_ptr<Ty> curr = typeInf(v, currentScope);
                        if(previous != nullptr)
                        {
                            std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                            std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                            currentScope->constraints.emplace_back(generic(previous, map1), generic(curr, map2));
                        }
                        previous = curr;
                    }
                    else
                    {
                        if(hasEllipse)
                        {
                            error(0, std::string_view(arr->start, arr->end - arr->start), std::string_view(v->start, v->end - v->start), "array pattern can contain only one remainder pattern!");
                        }
                        else hasEllipse = true;
                        std::shared_ptr<Ty> curr = typeInf(v, currentScope);
                        std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                        std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                        currentScope->constraints.emplace_back(generic(std::make_shared<TyArray>(previous, std::optional<size_t>()), map1), generic(curr, map2));
                    }
                }
                auto result = std::make_shared<TyArray>(previous, std::optional<size_t>());
                return result;
            }
            case NODE_NAMESPACE:
            {
                auto ns = std::static_pointer_cast<NamespaceNode>(n);
                auto scope = getNamespaceScope(ns->name, currentScope);
                if(scope != nullptr) return typeInf(ns->expr, scope);
                else {
                    error(ns->name.line, std::string_view(ns->start, ns->end - ns->start), std::string_view(ns->name.start, ns->name.length), "Invalid namespace name!");
                    return nullptr;
                }
            }
            case NODE_ELLIPSE:
            {
                auto ellipse = std::static_pointer_cast<EllipsePatternNode>(n);
                return typeInf(ellipse->expr, currentScope);
            }
            case NODE_RANGE:
            {
                auto range = std::static_pointer_cast<RangePatternNode>(n);
                std::shared_ptr<Ty> t1 = typeInf(range->expression1, currentScope);
                std::shared_ptr<Ty> t2 = typeInf(range->expression2, currentScope);
                std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                currentScope->constraints.push_back(std::make_pair(generic(t1, map1), generic(t2, map2)));
                return t1;
            }
            case NODE_ARRAYINDEX:
            {
                auto index = std::static_pointer_cast<ArrayIndexNode>(n);
                auto arrayType = typeInf(index->array, currentScope);
                std::shared_ptr<Ty> indexedType = newGenericType();
                auto next = std::make_shared<TyArray>(indexedType, std::make_optional<size_t>());
                std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                currentScope->constraints.emplace_back(generic(next, map1), generic(arrayType, map2));
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
                            return vd->identifiers.at(tokenToString(var->variable));
                        }
                        case NODE_FUNCTIONDECL:
                        {
                            auto fd = std::static_pointer_cast<FunctionDeclarationNode>(node);
                            return functionTypeFromFunction(fd);
                        }
                        case NODE_TYPE:
                        {
                            auto ty = std::static_pointer_cast<TypeNode>(node);
                            return ty->type;
                        }
                        default:
                        {
                            //error
                            return nullptr;
                        }
                    }
                }
                error(var->variable.line, std::string_view(var->start, var->end - var->start), std::string_view(var->start, var->end - var->start), "could not find identifier!");
                return nullptr;
            }
            case NODE_LAMBDA:
            {
                auto l = std::static_pointer_cast<LambdaNode>(n);
                auto ret = typeInf(l->body, currentScope);
                std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                currentScope->constraints.push_back(std::make_pair(generic(ret, map1), generic(l->returnType, map2)));
                std::shared_ptr<Ty> result = nullptr;
                for(auto it = l->params.begin(); it != l->params.end(); it++)
                {
                    if(result == nullptr) result = it->type;
                    else result = std::make_shared<TyFunc>(result, it->type);
                }
                result = std::make_shared<TyFunc>(result, l->returnType);
                currentScope->nodeTVars.insert(std::make_pair(result, n));
                return result;
            }
            case NODE_LITERAL:
            {
                auto lit = std::static_pointer_cast<LiteralNode>(n);
                std::string t;
                std::shared_ptr<Ty> result = nullptr;
                switch(lit->value.type)
                {
                    case TokenTypes::_TRUE:
                    case TokenTypes::_FALSE: t = "Bool"; break;
    	            case TokenTypes::FLOAT: t = "Float"; break;
                    case TokenTypes::DOUBLE: t = "Double"; break;
                    case TokenTypes::CHAR: t = "Char"; break;
                    case TokenTypes::INT:
                    case TokenTypes::HEX_INT:
                    case TokenTypes::OCT_INT:
                    case TokenTypes::BIN_INT: t = "Int"; break;
                    case TokenTypes::STRING: t = "String"; break;
                    default:
                        return nullptr;
                }
                result = std::make_shared<TyBasic>(t);
                return result;
            }
            case NODE_TYPE:
            {
                auto t = std::static_pointer_cast<TypeNode>(n);
                auto node = findInScope(declarationName(t->type), currentScope);
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
                        case NODE_STRUCTDECL:
                        {
                            auto sd = std::static_pointer_cast<StructDeclarationNode>(node);
                            return sd->typeDefined;
                        }
                        case NODE_UNIONDECL:
                        {
                            auto ud = std::static_pointer_cast<UnionDeclarationNode>(node);
                            return ud->typeDefined;
                        }
                        case NODE_TYPE:
                        {
                            auto ty = std::static_pointer_cast<TypeNode>(node);
                            return ty->type;
                        }
                        default:
                        {
                            //error
                            return nullptr;
                        }
                    }
                }
                return nullptr;
            }
            case NODE_LISTINIT:
            {
                auto init = std::static_pointer_cast<ListInitNode>(n);
                if(init->type)
                {
                    auto namedType = typeInf(init->type, currentScope);                    
                    auto scope = namespaceScope(init->type, currentScope);
                    if(scope == nullptr) scope = currentScope;
                    std::shared_ptr<StructDeclarationNode> sd = nullptr;
                    while(scope != nullptr)
                    {
                        if(scope->structs.find(declarationName(namedType)) != scope->structs.end())
                        {
                            sd = scope->structs.at(declarationName(namedType));
                            break;
                        }
                        else scope = scope->parentScope;
                    }
                    if(sd == nullptr)
                    {
                        error(0, std::string_view(init->start, init->end - init->start), std::string_view(init->type->start, init->type->end - init->type->start), "Could not find struct type!");
                        return nullptr;
                    }
                    else
                    {
                        if(sd->fields.size() != init->values.size()) 
                        {
                            error(0, std::string_view(init->start, init->end - init->start), std::string_view(init->start, init->end - init->start), "too few/many values to initialize struct!");
                            return nullptr;
                        }
                        //TODO: account for type parameters in struct somehow
                        for(auto i = 0; i < sd->fields.size(); i++)
                        {
                            auto field = sd->fields[i];
                            auto value = init->values[i];
                            std::unordered_map<std::string, std::shared_ptr<Ty>> map1;
                            map1 = genericMap(sd->typeDefined, map1);
                            std::unordered_map<std::string, std::shared_ptr<Ty>> map2;
                            currentScope->constraints.push_back(std::make_pair(generic(field.type, map1), generic(typeInf(value, currentScope), map2)));
                            return generic(sd->typeDefined, map1);
                        }
                    }
                }
                else
                {
                    error(0, std::string_view(init->start, init->end - init->start), std::string_view(init->start, init->end - init->start), "struct initialization must have a type name!");
                    return nullptr;
                }
                break;
            }
            case NODE_TUPLE:
            {
                auto tuple = std::static_pointer_cast<TupleConstructorNode>(n);
                std::vector<std::shared_ptr<Ty>> types;
                for(auto v : tuple->values)
                {
                    types.push_back(typeInf(v, currentScope));
                }
                std::shared_ptr<TyTuple> t = std::make_shared<TyTuple>(types);
                currentScope->nodeTVars.insert(std::make_pair(t, n));
                return t;
            }
            case NODE_PLACEHOLDER:
            {
                auto result = std::make_shared<TyBasic>("%Void");
                return result;
            }
            default:
            {
                system("pause");
                return nullptr;
            }
        }
    }

}