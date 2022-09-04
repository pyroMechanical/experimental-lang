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
namespace pilaf {

    bool stringInType(std::string s, std::shared_ptr<Ty> t)
    {
        switch(t->type)
        {
            case Ty::TY_FUNCTION:
            {
                auto f = std::static_pointer_cast<TyFunc>(t);
                return stringInType(s, f->in) || stringInType(s, f->out);
            }
            case Ty::TY_APPLICATION:
            {
                auto app = std::static_pointer_cast<TyAppl>(t);
                bool b = stringInType(s, app->applied);
                for(auto v : app->vars)
                {
                    b |= stringInType(s, v);
                }
                return b;
            }
            case Ty::TY_ARRAY:
            {
                auto arr = std::static_pointer_cast<TyArray>(t);
                return stringInType(s, arr->arrayOf);
            }
            case Ty::TY_POINTER:
            {
                auto ptr = std::static_pointer_cast<TyPointer>(t);
                return stringInType(s, ptr->pointsTo);
            }
            case Ty::TY_TUPLE:
            {
                auto tuple = std::static_pointer_cast<TyTuple>(t);
                bool b = false;
                for(auto t : tuple->types)
                {
                    b |= stringInType(s, t);
                }
                return b;
            }
            case Ty::TY_BASIC:
            {
                auto basic = std::static_pointer_cast<TyBasic>(t);
                return s.compare(basic->t) == 0;
            }
            case Ty::TY_VAR:
            {
                auto var = std::static_pointer_cast<TyVar>(t);
                return s.compare(var->var) == 0;
            }
            default: return false;
        }
    }

    std::shared_ptr<Ty> applyRewrites(std::shared_ptr<Ty> t, std::unordered_multimap<std::string, std::shared_ptr<Ty>> rewrites)
    {
        switch(t->type)
        {
            case Ty::TY_FUNCTION:
            {
                auto f = std::static_pointer_cast<TyFunc>(t);
                auto in = applyRewrites(f->in, rewrites);
                auto out = applyRewrites(f->out, rewrites);
                return std::make_shared<TyFunc>(in, out);
            }
            case Ty::TY_ARRAY:
            {
                auto arr = std::static_pointer_cast<TyArray>(t);
                auto arrayOf = applyRewrites(arr->arrayOf, rewrites);
                return std::make_shared<TyArray>(arrayOf, arr->size);
            }
            case Ty::TY_POINTER:
            {
                auto ptr = std::static_pointer_cast<TyPointer>(t);
                auto pointsTo = applyRewrites(ptr->pointsTo, rewrites);
                return std::make_shared<TyPointer>(pointsTo);
            }
            case Ty::TY_APPLICATION:
            {
                auto app = std::static_pointer_cast<TyAppl>(t);
                auto applied = applyRewrites(app->applied, rewrites);
                std::vector<std::shared_ptr<Ty>> vars;
                for(auto v : app->vars)
                {
                    vars.push_back(applyRewrites(v, rewrites));
                }
                return std::make_shared<TyAppl>(applied, vars);
            }
            case Ty::TY_BASIC:
            {
                //TODO: replace with lookup of type
                auto basic = std::static_pointer_cast<TyBasic>(t);
                if(rewrites.count(basic->t) > 0)
                {
                    auto t = rewrites.equal_range(basic->t);
                    return t.first->second;
                }
                else return std::make_shared<TyBasic>("*");
            }
            case Ty::TY_VAR:
            {
                auto var = std::static_pointer_cast<TyVar>(t);
                if(rewrites.count(var->var) > 0)
                {
                    auto t = rewrites.equal_range(var->var);
                    return t.first->second;
                }
                else return std::make_shared<TyBasic>("*");
            }
        }
    }

    std::pair<bool, std::shared_ptr<Ty>> solveKinds(std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> constraints, std::string name)
    {
        std::unordered_multimap<std::string, std::shared_ptr<Ty>> result;
        bool isValid = true;
        auto star = std::make_shared<TyBasic>("*");

        while(constraints.size() != 0)
        {
            auto constraint = constraints.front();
            constraints.pop_front();
            switch(constraint.first->type)
            {
                case Ty::TY_FUNCTION:
                {
                    auto f = std::static_pointer_cast<TyFunc>(constraint.first);
                    constraints.push_back(std::make_pair(f->in, star));
                    constraints.push_back(std::make_pair(f->out, star));
                    break;
                }
                case Ty::TY_ARRAY:
                {
                    auto arr = std::static_pointer_cast<TyArray>(constraint.first);
                    constraints.push_back(std::make_pair(arr->arrayOf, star));
                    break;
                }
                case Ty::TY_POINTER:
                {
                    auto ptr = std::static_pointer_cast<TyPointer>(constraint.first);
                    constraints.push_back(std::make_pair(ptr->pointsTo, star));
                    break;
                }
                case Ty::TY_TUPLE:
                {
                    auto tuple = std::static_pointer_cast<TyTuple>(constraint.first);
                    for(auto t : tuple->types)
                    {
                        constraints.push_back(std::make_pair(t, star));
                    }
                    break;
                }
                case Ty::TY_APPLICATION:
                {
                    auto app = std::static_pointer_cast<TyAppl>(constraint.first);
                    std::shared_ptr<Ty> t = star;
                    
                    for(auto it = app->vars.rbegin(); it != app->vars.rend(); it++)
                    {
                        t = std::make_shared<TyFunc>(*it, t);
                    }
                    constraints.push_back(std::make_pair(app->applied, t));
                    break;
                }
                case Ty::TY_BASIC:
                case Ty::TY_VAR:
                {
                    result.emplace(typeToString(constraint.first), constraint.second);
                    break;
                }
            }
        }
        
        for(auto constraint : result)
        {
            isValid &= !stringInType(constraint.first, constraint.second);
        }
        if(isValid)
        {
            for(auto& constraint : result)
            {
                constraint.second = applyRewrites(constraint.second, result);
            }
            
            for(auto constraint : result)
            {
               if(constraint.first.compare(name) == 0)
               {
                return std::make_pair(isValid, constraint.second);
               }
            }
        }
        return std::make_pair(false, nullptr);
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
                std::shared_ptr<StructDeclarationNode> r = nullptr;
                std::shared_ptr<UnionDeclarationNode> u = nullptr;
                auto s = currentScope;
                while((c == nullptr || r == nullptr || u == nullptr) && s != nullptr)
                {
                    if(s->classes.find(tokenToString(ci->_class)) != s->classes.end())
                    {
                        c = s->classes.at(tokenToString(ci->_class));
                    }
                    if(s->structs.find(typeToString(ci->implemented)) != s->structs.end())
                    {
                        r = s->structs.at(typeToString(ci->implemented));
                    }
                    if(s->unions.find(typeToString(ci->implemented)) != s->unions.end())
                    {
                        u = s->unions.at(typeToString(ci->implemented));
                    }
                    s = s->parentScope;
                }
                if((r == nullptr && u == nullptr) || c == nullptr)
                {
                    system("pause");
                    assert(false);
                }
                if(u == nullptr)
                {
                    if(!typesEqual(c->kind, r->kind))
                    {
                        //error
                        system("pause");
                        assert(false);
                    }
                }
                if(r == nullptr)
                {
                    if(!typesEqual(c->kind, u->kind))
                    {
                        //error
                        system("pause");
                        assert(false);
                    }
                }
                break;
            }
            case NODE_CLASSDECL:
            {
                break;
            }
            case NODE_STRUCTDECL:
            {
                break;
            }
            case NODE_UNIONDECL:
            {
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
            case NODE_STRUCTDECL:
            {
                auto sd = std::static_pointer_cast<StructDeclarationNode>(n);
                if(sd->kind == nullptr)
                {
                    std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> kinds;
                    auto star = std::make_shared<TyBasic>("*");
                    kinds.push_back(std::make_pair(sd->typeDefined, star));
                    for(auto f : sd->fields)
                    {
                        kinds.push_back(std::make_pair(f.type, star));
                    }

                    auto result = solveKinds(kinds, declarationName(sd->typeDefined));
                    if(result.first)
                    {
                        sd->kind = result.second;
                    }
                    else error(0, std::string_view(sd->start, sd->end - sd->start), /*todo: figure out what goes here*/std::string_view(sd->start, sd->end - sd->start), "inconsistent types in struct!");
                }
                break;
            }
            case NODE_UNIONDECL:
            {
                auto ud = std::static_pointer_cast<UnionDeclarationNode>(n);
                if(ud->kind == nullptr)
                {
                    std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> kinds;
                    auto star = std::make_shared<TyBasic>("*");
                    kinds.push_back(std::make_pair(ud->typeDefined, star));
                    for(auto f : ud->members)
                    {
                        if(f.type != nullptr) kinds.push_back(std::make_pair(f.type, star));
                    }

                    auto result = solveKinds(kinds, declarationName(ud->typeDefined));
                    if(result.first)
                    {
                        ud->kind = result.second;
                    }
                    else error(0, std::string_view(ud->start, ud->end - ud->start), /*todo: figure out what goes here*/std::string_view(ud->start, ud->end - ud->start), "inconsistent types in union!");
                    
                }
                break;
            }
            case NODE_VARIABLEDECL:
            {
                auto vd = std::static_pointer_cast<VariableDeclarationNode>(n);
                if(vd->value) RecordKinds(vd->value, currentScope);
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
                    std::shared_ptr<Ty> functionType = functionTypeFromFunction(fd);
                    std::vector<int> argCounts;
                    
                    std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> kinds;
                    auto star = std::make_shared<TyBasic>("*");
                    for(auto f : classdecl->functions)
                    {
                        kinds.push_back(std::make_pair(functionTypeFromFunction(std::static_pointer_cast<FunctionDeclarationNode>(f)), star));
                    }

                    auto result = solveKinds(kinds, tokenToString(classdecl->typeName));
                    if(result.first)
                    {
                        classdecl->kind = result.second;
                    }
                    //else error
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
                std::cout << "Constraint: " << typeToString(c.first) << " == " << typeToString(c.second) << "\n";
            }
            for(auto child : shared->childScopes)
            {
                printConstraints(child);
            }
        }
    }
    
    std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> flattenConstraints(std::shared_ptr<ScopeNode> scope)
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
    
    std::shared_ptr<Ty> replaceInType(std::shared_ptr<Ty> t, std::shared_ptr<Ty> replacing, std::shared_ptr<Ty> replaced)
    {
        switch(t->type)
        {
            case Ty::TY_APPLICATION:
            {
                auto app = std::static_pointer_cast<TyAppl>(t);
                auto applied = replaceInType(app->applied, replacing, replaced);
                assert(app->vars.size() != 0);
                std::vector<std::shared_ptr<Ty>> vars;
                for(auto i = 0; i < app->vars.size(); i++)
                {
                    vars.push_back(replaceInType(app->vars[i], replacing, replaced));
                }
                return std::make_shared<TyAppl>(applied, vars);
            }
            case Ty::TY_FUNCTION:
            {
                auto fn = std::static_pointer_cast<TyFunc>(t);
                auto in = replaceInType(fn->in, replacing, replaced);
                auto out = replaceInType(fn->out, replacing, replaced);
                return std::make_shared<TyFunc>(in, out);
            }
            case Ty::TY_ARRAY:
            {
                auto arr = std::static_pointer_cast<TyArray>(t);
                auto arrayOf = replaceInType(arr->arrayOf, replacing, replaced);
                return std::make_shared<TyArray>(arrayOf, arr->size);
            }
            case Ty::TY_POINTER:
            {
                auto ptr = std::static_pointer_cast<TyPointer>(t);
                auto pointsTo = replaceInType(ptr->pointsTo, replacing, replaced);
                return std::make_shared<TyPointer>(pointsTo);
            }
            case Ty::TY_TUPLE:
            {
                auto tuple = std::static_pointer_cast<TyTuple>(t);
                std::vector<std::shared_ptr<Ty>> types;
                for(auto t : tuple->types)
                {
                    types.push_back(replaceInType(t, replacing, replaced));
                }
                return std::make_shared<TyTuple>(types);
            }
            case Ty::TY_BASIC:
            case Ty::TY_VAR:
            {
                if(typesEqual(t, replaced))
                {
                    return replacing;
                }
                else return t;
            }
            default:
            {
                system("pause");
                return nullptr;
            }
        }
    }
    
    std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> resolveConstraints(std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> constraints)
    {
        std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> result;
        while(constraints.size() > 0)
        {
            auto constraint = constraints.front();
            constraints.pop_front();
            if(typesEqual(constraint.first, constraint.second)) continue;
            if(constraint.first->type == Ty::TY_BASIC && constraint.second->type == Ty::TY_BASIC)
            {
                printf("error: type %s is not equal to %s!", typeToString(constraint.first).c_str(), typeToString(constraint.second).c_str());
                return {};
            }
            else if(constraint.first->type == Ty::TY_APPLICATION && constraint.second->type == Ty::TY_APPLICATION)
            {
                auto c1 = std::static_pointer_cast<TyAppl>(constraint.first);
                auto c2 = std::static_pointer_cast<TyAppl>(constraint.second);
                assert(c1->vars.size() != 0);
                assert(c2->vars.size() != 0);
                auto t1 = c1->applied;
                auto t2 = c2->applied;
                constraints.push_front(std::make_pair(t1, t2));
                assert(c1->vars.size() == c2->vars.size());
                for(auto i = 0; i < c1->vars.size(); i++)
                {
                    t1 = c1->vars[i];
                    t2 = c2->vars[i];
                    constraints.push_front(std::make_pair(t1, t2));
                }
            }
            else if (isFunctionType(constraint.first) && isFunctionType(constraint.second))
            {
                auto t1 = firstParameterFromFunctionType(constraint.first);
                auto t2 = firstParameterFromFunctionType(constraint.second);
                constraints.push_front(std::make_pair(t1, t2));
                t1 = std::static_pointer_cast<TyFunc>(constraint.first)->out;
                t2 = std::static_pointer_cast<TyFunc>(constraint.second)->out;
                constraints.push_front(std::make_pair(t1, t2));
            }
            else if (constraint.first->type == Ty::TY_TUPLE && constraint.second->type == Ty::TY_TUPLE)
            {
                auto t1 = std::static_pointer_cast<TyTuple>(constraint.first);
                auto t2 = std::static_pointer_cast<TyTuple>(constraint.second);
                if(t1->types.size() != t2->types.size())
                {
                    //error
                    system("pause");
                }
                for(size_t i = 0; i < t1->types.size(); i++)
                {
                    constraints.push_front(std::make_pair(t1->types[i], t2->types[i]));
                }
            }
            else if (constraint.first->type == Ty::TY_ARRAY && constraint.second->type == Ty::TY_ARRAY)
            {
                auto t1 = std::static_pointer_cast<TyArray>(constraint.first);
                auto t2 = std::static_pointer_cast<TyArray>(constraint.second);
                if(t1->size.has_value() && t2->size.has_value() && t1->size.value() != t2->size.value())
                {
                    //error
                    system("pause");
                }
                constraints.push_front(std::make_pair(t1->arrayOf, t2->arrayOf));
            }
            else
            {
                auto t1 = constraint.first;
                auto t2 = constraint.second;
                std::shared_ptr<Ty> replacing, replaced;
                if(t2->type != Ty::TY_VAR)
                {
                    replacing = t2;
                    replaced = t1;
                }            
                else
                {
                    replacing = t1;
                    replaced = t2;
                }
                result.push_back(std::make_pair(replaced, replacing));
                std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> newConstraints;
                for(auto constraint : constraints)
                {
                    auto first = replaceInType(constraint.first, replacing, replaced);
                    auto second = replaceInType(constraint.second, replacing, replaced);
                    newConstraints.push_back(std::make_pair(first, second));
                }

                constraints = std::move(newConstraints);
            }
        }
        return result;
    }
    
    
    std::shared_ptr<ProgramNode> analyze(const char *src)
    {
        //std::string blah = std::string("blah\nblah\nblah\nblah error blah\nblah");
        //puts(blah.c_str());
        //printf("-----\n");
        //error(1, {blah.c_str(), blah.c_str() + 35}, {blah.c_str() + 20, blah.c_str() + 25}, "this is an error message!");
    
        std::shared_ptr<ProgramNode> ast = parse(src);
        if(ast == nullptr) return nullptr;
        if(ast->hadError) return nullptr;
        
        for (auto dec : ast->declarations)
        {
            typeInf(dec, ast->globalScope);
        }
        if(!ast->hadError && !hadError())
        {
            printConstraints(ast->globalScope);
            auto constraints = flattenConstraints(ast->globalScope);
            auto substitutions = resolveConstraints(constraints);
            if(substitutions.empty() && !constraints.empty()) ast->hadError = true;
            for(auto substitution : substitutions)
            {
                std::cout << "Substitution: " << typeToString(substitution.first) << " => " << typeToString(substitution.second) << "\n";
            }
        }
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

        if(!ast->hadError && !hadError())
        {
            return ast;
        }

        return nullptr;
    }
}