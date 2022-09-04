#include <cassert>
#include <cstring>
#include <cstdio>
#include <utility>
#include "parser.h"

namespace pilaf {
    std::shared_ptr<ScopeNode> newScope(std::shared_ptr<ScopeNode> parent)
    {
        return std::make_shared<ScopeNode>(parent);
    }

    std::shared_ptr<ScopeNode> getNamespaceScope(Token name, std::shared_ptr<ScopeNode> scope)
    {
        auto nameStr = tokenToString(name);
        while(scope != nullptr)
        {
            if(scope->namespaces.find(nameStr) != scope->namespaces.end())
            {
                return scope->namespaces.at(nameStr);
            }
            else
            {
                scope = scope->parentScope;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Ty> generic(std::shared_ptr<Ty> type, std::unordered_map<std::string, std::shared_ptr<Ty>>& replaced)
    {
        switch(type->type)
        {
            case Ty::TY_VAR:
            {
                auto t = typeToString(type);
                if(t.front() == '\'') return type;
                if(replaced.find(t) != replaced.end())
                {
                    return replaced.at(t);
                }
                else 
                {
                    auto ty = newGenericType();
                    replaced.emplace(t, ty);
                    return ty;
                }
            }
            case Ty::TY_BASIC:
            {
                return type;
            }
            case Ty::TY_ARRAY:
            {
                auto arr = std::static_pointer_cast<TyArray>(type);
                auto arrayOf = generic(arr->arrayOf, replaced);
                return std::make_shared<TyArray>(arrayOf, arr->size);
            }
            case Ty::TY_POINTER:
            {
                auto ptr = std::static_pointer_cast<TyPointer>(type);
                auto pointsTo = generic(ptr->pointsTo, replaced);
                return std::make_shared<TyPointer>(pointsTo);
            }
            case Ty::TY_FUNCTION:
            {
                auto fn = std::static_pointer_cast<TyFunc>(type);
                auto in = generic(fn->in, replaced);
                auto out = generic(fn->out, replaced);
                return std::make_shared<TyFunc>(in, out);
            }
            case Ty::TY_TUPLE:
            {
                auto tp = std::static_pointer_cast<TyTuple>(type);
                std::vector<std::shared_ptr<Ty>> types;
                for(auto ty : tp->types)
                {
                    types.push_back(generic(ty, replaced));
                }
                return std::make_shared<TyTuple>(types);
            }
            case Ty::TY_APPLICATION:
            {
                auto app = std::static_pointer_cast<TyAppl>(type);
                auto applied = generic(app->applied, replaced);
                std::vector<std::shared_ptr<Ty>> vars;
                for(auto var : app->vars)
                {
                    vars.push_back(generic(var, replaced));
                }
                return std::make_shared<TyAppl>(applied, vars);
            }
        }
    }

    std::unordered_map<std::string, std::shared_ptr<Ty>> genericMap(std::shared_ptr<Ty> type, std::unordered_map<std::string, std::shared_ptr<Ty>> replaced)
    {
        switch(type->type)
        {
            case Ty::TY_VAR:
            {
                auto t = typeToString(type);
                if(t.front() == '\'') return replaced;
                if(replaced.find(t) != replaced.end())
                {
                    return replaced;
                }
                else 
                {
                    auto ty = newGenericType();
                    replaced.emplace(t, ty);
                    return replaced;
                }
            }
            case Ty::TY_BASIC:
            {
                return replaced;
            }
            case Ty::TY_ARRAY:
            {
                auto arr = std::static_pointer_cast<TyArray>(type);
                
                return genericMap(arr->arrayOf, replaced);
            }
            case Ty::TY_POINTER:
            {
                auto ptr = std::static_pointer_cast<TyPointer>(type);
                return genericMap(ptr->pointsTo, replaced);
            }
            case Ty::TY_FUNCTION:
            {
                auto fn = std::static_pointer_cast<TyFunc>(type);
                auto in = genericMap(fn->in, replaced);
                auto out = genericMap(fn->out, in);
                return out;
            }
            case Ty::TY_TUPLE:
            {
                auto tp = std::static_pointer_cast<TyTuple>(type);
                auto result = replaced;
                for(auto ty : tp->types)
                {
                    result = genericMap(ty, result);
                }
                return result;
            }
            case Ty::TY_APPLICATION:
            {
                auto app = std::static_pointer_cast<TyAppl>(type);
                auto applied = genericMap(app->applied, replaced);
                for(auto var : app->vars)
                {
                    applied = genericMap(var, applied);
                }
                return applied;
            }
        }
    }

    std::string declarationName(std::shared_ptr<Ty> type)
    {
        switch(type->type)
        {
            case Ty::TY_BASIC:
                return typeToString(type);
            case Ty::TY_APPLICATION:
            {
                auto app = std::static_pointer_cast<TyAppl>(type);
                return typeToString(app->applied);
            }
            case Ty::TY_POINTER:
            {
                auto ptr = std::static_pointer_cast<TyPointer>(type);
                return declarationName(ptr->pointsTo);
            }
            case Ty::TY_ARRAY:
            {
                auto arr = std::static_pointer_cast<TyArray>(type);
                return declarationName(arr->arrayOf);
            }
            default:
                return "";
        }
    }

    //TODO: complete this function to enable parser testing
    bool compareAST(std::shared_ptr<node> a, std::shared_ptr<node> b)
    {
        if(a == nullptr || b == nullptr)
        {
            if(a == nullptr && b == nullptr)
            {
                return true;
            }
            else return false;
        }
        if(a->nodeType != b->nodeType) return false;
        switch(a->nodeType)
        {
            case NodeType::NODE_UNDEFINED: return false;
            case NodeType::NODE_PROGRAM:
            {
                auto pa = std::static_pointer_cast<ProgramNode>(a);
                auto pb = std::static_pointer_cast<ProgramNode>(b);
                if(pa->hadError != pa->hadError) return false;
                if(pa->declarations.size() != pb->declarations.size()) return false;
                bool result = true;
                for(int i = 0; i < pa->declarations.size(); i++)
                {
                    result &= compareAST(pa->declarations[i], pb->declarations[i]);
                }
                return result;
            }
            case NodeType::NODE_LIBRARY: return false; //unimplemented
            case NodeType::NODE_IMPORT: return false; //unimplemented
            case NodeType::NODE_TYPE: return false; //unimplemented
            case NodeType::NODE_DECLARATION: return false; //unimplemented
            case NodeType::NODE_TYPEDEF:
            {
                auto ta = std::static_pointer_cast<TypedefNode>(a);
                auto tb = std::static_pointer_cast<TypedefNode>(b);
            }
            case NodeType::NODE_UNIONDECL: return false; //unimplemented
            case NodeType::NODE_STRUCTDECL: return false; //unimplemented
            case NodeType::NODE_FUNCTIONDECL: 
            {
                auto fa = std::static_pointer_cast<FunctionDeclarationNode>(a);
                auto fb = std::static_pointer_cast<FunctionDeclarationNode>(b);
                bool result = true;
                result &= tokencmp(fa->identifier, fb->identifier);
                if(fa->params.size() != fb->params.size()) return false;
                for(auto i = 0; i < fa->params.size(); i++)
                {
                    auto pa = fa->params[i];
                    auto pb = fb->params[i];
                    result &= typesEqual(pa.type, pb.type);
                    result &= tokencmp(pa.identifier, pb.identifier);
                }
                result &= typesEqual(fa->returnType, fb->returnType);
                result &= compareAST(fa->body, fb->body);
                return result;
            } 
            case NodeType::NODE_VARIABLEDECL: return false; //unimplemented
            case NodeType::NODE_CLASSDECL: return false; //unimplemented
            case NodeType::NODE_CLASSIMPL: return false; //unimplemented
            case NodeType::NODE_FOR:
            {
                auto fa = std::static_pointer_cast<ForStatementNode>(a);
                auto fb = std::static_pointer_cast<ForStatementNode>(b);
                return compareAST(fa->initExpr, fb->initExpr) &&
                compareAST(fa->condExpr, fb->condExpr) &&
                compareAST(fa->incrementExpr, fb->incrementExpr) &&
                compareAST(fa->loopStmt, fb->loopStmt);
            }
            case NodeType::NODE_IF: 
            {
                auto ia = std::static_pointer_cast<IfStatementNode>(a);
                auto ib = std::static_pointer_cast<IfStatementNode>(b);
                return compareAST(ia->branchExpr, ib->branchExpr) &&
                 compareAST(ia->thenStmt, ib->thenStmt) &&
                 compareAST(ia->elseStmt, ib->elseStmt);
            } 
            case NodeType::NODE_WHILE: 
            {
                auto wa = std::static_pointer_cast<WhileStatementNode>(a);
                auto wb = std::static_pointer_cast<WhileStatementNode>(b);
                return compareAST(wa->loopExpr, wb->loopExpr) &&
                 compareAST(wa->loopStmt, wb->loopStmt);
            } 
            case NodeType::NODE_SWITCH: 
            {
                auto sa = std::static_pointer_cast<SwitchStatementNode>(a);
                auto sb = std::static_pointer_cast<SwitchStatementNode>(b);
                bool result = true;
                if(sa->cases.size() != sb->cases.size()) return false;
                for(auto i = 0; i < sa->cases.size(); i++)
                {
                    result &= compareAST(sa->cases[i], sb->cases[i]);
                }
                return result && compareAST(sa->switchExpr, sb->switchExpr);
            }
            case NodeType::NODE_CASE: 
            {
                auto ca = std::static_pointer_cast<CaseNode>(a);
                auto cb = std::static_pointer_cast<CaseNode>(b);
                return compareAST(ca->caseExpr, cb->caseExpr) &&
                 compareAST(ca->caseStmt, cb->caseStmt);
            } 
            case NodeType::NODE_RETURN: 
            {
                auto ra = std::static_pointer_cast<ReturnStatementNode>(a);
                auto rb = std::static_pointer_cast<ReturnStatementNode>(b);
                return compareAST(ra->returnExpr, rb->returnExpr);
            } 
            case NodeType::NODE_BREAK: return true;
            case NodeType::NODE_CONTINUE: return true;
            case NodeType::NODE_BLOCK: return false; //unimplemented
            case NodeType::NODE_ASSIGNMENT: 
            {
                auto aa = std::static_pointer_cast<AssignmentNode>(a);
                auto ab = std::static_pointer_cast<AssignmentNode>(b);
                return compareAST(aa->variable, ab->variable) && compareAST(aa->assignment, ab->assignment);
            }
            case NodeType::NODE_BINARY: {
                auto ba = std::static_pointer_cast<BinaryNode>(a);
                auto bb = std::static_pointer_cast<BinaryNode>(b);
                return tokencmp(ba->op, bb->op) && compareAST(ba->expression1, bb->expression1) && compareAST(ba->expression2, bb->expression2);
            } 
            case NodeType::NODE_UNARY: 
            {
                auto ua = std::static_pointer_cast<UnaryNode>(a);
                auto ub = std::static_pointer_cast<UnaryNode>(b);
                return tokencmp(ua->op, ub->op) && compareAST(ua->expression, ub->expression);
            } 
            case NodeType::NODE_FUNCTIONCALL: return false; //unimplemented
            case NodeType::NODE_FIELDCALL: return false; //unimplemented
            case NodeType::NODE_ARRAYCONSTRUCTOR: return false; //unimplemented
            case NodeType::NODE_ARRAYINDEX: return false; //unimplemented
            case NodeType::NODE_IDENTIFIER:
            {
                auto ia = std::static_pointer_cast<VariableNode>(a);
                auto ib = std::static_pointer_cast<VariableNode>(b);
                return tokencmp(ia->variable, ib->variable);
            }
            case NodeType::NODE_LAMBDA: return false; //unimplemented
            case NodeType::NODE_LITERAL:
            {
                auto la = std::static_pointer_cast<LiteralNode>(a);
                auto lb = std::static_pointer_cast<LiteralNode>(b);
                return tokencmp(la->value, lb->value);
            }
            case NodeType::NODE_LISTINIT: return false; //unimplemented
            case NodeType::NODE_TUPLE: return false; //unimplemented
            case NodeType::NODE_PLACEHOLDER: return false; //unimplemented
            case NodeType::NODE_SCOPE: return false; //unimplemented
            case NodeType::NODE_ERROR: return false;
            default: return false;
        }
    }
    
    bool typesEqual(std::shared_ptr<Ty> a, std::shared_ptr<Ty> b)
    {
        if(a == nullptr || b == nullptr) 
        {
            if(a == nullptr && b == nullptr) return true;
            else return false;
        }
        if(a->type != b->type) return false;
        switch(a->type)
        {
            case Ty::TY_FUNCTION:
            {
                auto fa = std::static_pointer_cast<TyFunc>(a);
                auto fb = std::static_pointer_cast<TyFunc>(b);
                return typesEqual(fa->in, fb->in) && typesEqual(fa->out, fb->out);
            }
            case Ty::TY_APPLICATION:
            {
                auto aa = std::static_pointer_cast<TyAppl>(a);
                auto ab = std::static_pointer_cast<TyAppl>(b);
                bool vars = true;
                if(aa->vars.size() != ab->vars.size())
                {
                    vars = false;
                }
                else
                {
                    for(auto i = 0; i < aa->vars.size(); i++)
                    {
                        vars &= typesEqual(aa->vars[i], ab->vars[i]);
                    }
                }
                return  vars && typesEqual(aa->applied, ab->applied);
            }
            case Ty::TY_VAR:
            {
                auto va = std::static_pointer_cast<TyVar>(a);
                auto vb = std::static_pointer_cast<TyVar>(b);
                return va->var.compare(vb->var) == 0;
            }
            case Ty::TY_TUPLE:
            {
                auto ta = std::static_pointer_cast<TyTuple>(a);
                auto tb = std::static_pointer_cast<TyTuple>(b);
                if(ta->types.size() != tb->types.size()) return false;
                bool result = true;
                for(size_t i = 0; i < ta->types.size(); i++)
                {
                    result &= typesEqual(ta->types[i], tb->types[i]);
                }
                return result;
            }
            case Ty::TY_ARRAY:
            {
                auto aa = std::static_pointer_cast<TyArray>(a);
                auto ab = std::static_pointer_cast<TyArray>(b);
                return typesEqual(aa->arrayOf, ab->arrayOf) && (aa->size == ab->size);
            }
            case Ty::TY_POINTER:
            {
                auto pa = std::static_pointer_cast<TyPointer>(a);
                auto pb = std::static_pointer_cast<TyPointer>(b);
                return typesEqual(pa->pointsTo, pa->pointsTo);
            }
            //TY_ARRAYCON,
            //TY_FUNCTIONCON,
            //TY_TUPLECON,
            case Ty::TY_BASIC:
            {
                auto ba = std::static_pointer_cast<TyBasic>(a);
                auto bb = std::static_pointer_cast<TyBasic>(a);
                return ba->t.compare(bb->t) == 0;
            }
        }
        return false;
    }
    
    std::string typeToString(std::shared_ptr<Ty> t)
    {
        //does not account for precedence of types, array should wrap functions, pointers; pointers should wrap functions
        if(t == nullptr) return "";
        switch(t->type)
        {
            case Ty::TY_FUNCTION:
            {
                auto ft = std::static_pointer_cast<TyFunc>(t);
                auto in = typeToString(ft->in);
                auto out = typeToString(ft->out);
                if(ft->in->type == Ty::TY_FUNCTION)
                {
                    in = std::string("(").append(in).append(")");
                }

                auto result = in;
                result.append(" -> ").append(out);
                return result;
            }
            case Ty::TY_APPLICATION:
            {
                auto at = std::static_pointer_cast<TyAppl>(t);
                auto applied = typeToString(at->applied);
                auto result = applied;
                for(auto v : at->vars)
                {
                    result.append(" ").append(typeToString(v));
                }
                return result;
            }
            case Ty::TY_VAR:
            {
                auto vt = std::static_pointer_cast<TyVar>(t);
                return vt->var;
            }
            case Ty::TY_BASIC:
            {
                auto bt = std::static_pointer_cast<TyBasic>(t);
                return bt->t;
            }
            case Ty::TY_TUPLE:
            {
                auto tt = std::static_pointer_cast<TyTuple>(t);
                auto result = std::string("( ");
                bool first = true;
                for(auto _t : tt->types)
                {
                    if(!first) result.append(", ");
                    first = false;
                    result.append(typeToString(_t));
                }
                result.append(")");
                return result;
            }
            case Ty::TY_ARRAY:
            {
                auto at = std::static_pointer_cast<TyArray>(t);
                auto result = typeToString(at->arrayOf);
                if(at->arrayOf->type == Ty::TY_FUNCTION || at->type == Ty::TY_POINTER)
                {
                    result = std::string("(").append(result).append(")");
                }
                result.append("[");
                if(at->size.has_value())
                {
                    result.append(std::to_string(at->size.value()));
                }
                result.append("]");
                return result;
            }
            case Ty::TY_POINTER:
            {
                auto pt = std::static_pointer_cast<TyPointer>(t);
                auto result = typeToString(pt->pointsTo);
                if(pt->pointsTo->type == Ty::TY_FUNCTION)
                {
                    result = std::string("(").append(result).append(")");
                }
                result.append("*");
                return result;
            }
        }
        return "invalid!";
    }
    
    std::shared_ptr<node>expression(Parser *parser, std::shared_ptr<ScopeNode> scope);
    
    std::shared_ptr<node>grouping(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>arrayIndex(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::shared_ptr<node>unary(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>binary(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::shared_ptr<node>assignment(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::shared_ptr<node>literal(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>placeholder(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>list_init(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::shared_ptr<node>lambda(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>identifier(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>_type(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>field_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::shared_ptr<node>array_constructor(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>array_index(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::shared_ptr<node>function_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    
    std::shared_ptr<node>pattern_grouping(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>pattern_array_constructor(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>pattern_function_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::shared_ptr<node> range(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>n);
    std::unordered_map<TokenTypes, ParseRule> rules = {
        {TokenTypes::PAREN, {grouping, function_call, PRECEDENCE_POSTFIX, false}},
        {TokenTypes::CLOSE_PAREN, {nullptr, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::BRACE, {nullptr, list_init, PRECEDENCE_POSTFIX, false}},
        {TokenTypes::CLOSE_BRACE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::BRACKET, {array_constructor, array_index, PRECEDENCE_POSTFIX, false}},
        {TokenTypes::CLOSE_BRACKET, {nullptr, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::DOT, {nullptr, field_call, PRECEDENCE_POSTFIX, false}},
        {TokenTypes::COLON, {nullptr, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::ARROW, {nullptr, field_call, PRECEDENCE_POSTFIX, false}},
        {TokenTypes::EQUAL, {nullptr, assignment, PRECEDENCE_ASSIGNMENT, false}},
        {TokenTypes::BANG, {unary, nullptr, PRECEDENCE_PREFIX, false}},
        {TokenTypes::BANG_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
        {TokenTypes::EQUAL_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
        {TokenTypes::LESS, {nullptr, binary, PRECEDENCE_COMPARISON, false}},
        {TokenTypes::LESS_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
        {TokenTypes::GREATER, {nullptr, binary, PRECEDENCE_COMPARISON, false}},
        {TokenTypes::GREATER_EQUAL, {nullptr, binary, PRECEDENCE_EQUALITY, false}},
        {TokenTypes::TYPE, {_type, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::UNSAFE, {nullptr, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::IDENTIFIER, {identifier, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::OPERATOR, {unary, binary, PRECEDENCE_UNKNOWN, false}}, //add precedence information from preprocessor
        {TokenTypes::UNDERSCORE, {placeholder, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::LAMBDA, {lambda, nullptr, PRECEDENCE_PREFIX, false}},
        {TokenTypes::_TRUE, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::_FALSE, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::FLOAT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::DOUBLE, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::CHAR, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::HEX_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::OCT_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::BIN_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::STRING, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::AND, {nullptr, binary, PRECEDENCE_LOGICAL_AND, false}},
        {TokenTypes::OR, {nullptr, binary, PRECEDENCE_LOGICAL_OR, false}},
        {TokenTypes::NOT, {unary, nullptr, PRECEDENCE_PREFIX, false}},
        {TokenTypes::BIT_OR, {nullptr, binary, PRECEDENCE_BITWISE_OR, false}},
        {TokenTypes::SHIFT_LEFT, {nullptr, binary, PRECEDENCE_SHIFT, false}},
        {TokenTypes::SHIFT_RIGHT, {nullptr, binary, PRECEDENCE_SHIFT, false}},};
    
    std::unordered_map<TokenTypes, ParseRule> patternRules = {
        {TokenTypes::PAREN, {pattern_grouping, pattern_function_call, PRECEDENCE_POSTFIX, false}},
        {TokenTypes::BRACKET, {pattern_array_constructor, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::IDENTIFIER, {identifier, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::_TRUE, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::_FALSE, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::FLOAT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::DOUBLE, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::CHAR, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::HEX_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::OCT_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::BIN_INT, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::STRING, {literal, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::TYPE, {_type, nullptr, PRECEDENCE_NONE, false}},
        {TokenTypes::INCLUSIVE_RANGE, {nullptr, range, PRECEDENCE_POSTFIX, true}},
        {TokenTypes::EXCLUSIVE_RANGE, {nullptr, range, PRECEDENCE_POSTFIX, true}},
        {TokenTypes::UNDERSCORE, {placeholder, nullptr, PRECEDENCE_NONE, false}}
    };

    static void errorAt(Parser *parser, Token *token, const char *msg)
    {
        if (parser->panicMode)
            return;
        parser->panicMode = true;
        fprintf(stderr, "[line %d] Error", token->line);
    
        if (token->type == TokenTypes::_EOF)
        {
            fprintf(stderr, " at end");
        }
        else if (token->type == TokenTypes::_ERROR)
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
            parser->next = scanToken(&(*parser).lexer);
            if (parser->next.type != TokenTypes::_ERROR)
                break;
            errorAtNext(parser, parser->next.start);
        }
    }
    
    static void consume(Parser *parser, TokenTypes type, const char *message)
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
    std::shared_ptr<Ty> newGenericType()
    {
        static std::vector<uint8_t> v = {0};
        bool incremented = false;
        std::string var;
        var.reserve(v.size() + 1);
        var.push_back('\'');
        for(auto i : v)
        {
            var.push_back('a' + i);
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
        auto result = std::make_shared<TyVar>(var);
        return result;
    }
    
    static bool is_function_token(Token t)
    {
        switch(t.type)
        {
            case TokenTypes::TYPE:
            case TokenTypes::IDENTIFIER:
            case TokenTypes::PAREN:
                return true;
            default:
                return false;
        }
    }
    
    static std::shared_ptr<Ty> resolve_type_nogeneric(Parser *parser);
    
    std::shared_ptr<Ty> basic_type(Parser* parser)
    {
            switch(parser->current.type)
            {
                case TokenTypes::TYPE:
                {
                    auto t = std::make_shared<TyBasic>(tokenToString(parser->current));
                    advance(parser);
                    return t;
                }
                case TokenTypes::IDENTIFIER:
                {
                    auto t = std::make_shared<TyVar>(tokenToString(parser->current));
                    advance(parser);
                    return t;
                }
                case TokenTypes::PAREN:
                {
                    consume(parser, TokenTypes::CLOSE_PAREN, "expected ')'!");
                    auto t = std::make_shared<TyBasic>(std::string("()"));
                    advance(parser);
                    return t;
                }
                case TokenTypes::BRACKET:
                {
                    consume(parser, TokenTypes::CLOSE_BRACKET, "expected ']'!");
                    auto t = std::make_shared<TyBasic>(std::string("[]"));
                    advance(parser);
                    return t;
                }
        }
        return nullptr;
    }
    
    std::shared_ptr<Ty> paren_type(Parser* parser)
    {
        if(parser->current.type == TokenTypes::PAREN)
        {
            advance(parser);
            auto t = resolve_type_nogeneric(parser);
            if(parser->current.type == TokenTypes::CLOSE_PAREN)
            {
                advance(parser);
                return t;
            }
            else
            {
                std::vector<std::shared_ptr<Ty>> types;
                types.push_back(t);
                while(parser->current.type != TokenTypes::CLOSE_PAREN)
                {
                    consume(parser, TokenTypes::COMMA, "expected ',' between tuple types!");
                    types.push_back(resolve_type_nogeneric(parser));
                }
                consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' at end of tuple!");
                auto result = std::make_shared<TyTuple>(types);
                return result;
            }
        }
        else
        {
            return basic_type(parser);
        }
    }
    
    std::shared_ptr<Ty> applied_type(Parser* parser)
    {
        auto prev = paren_type(parser);
        if(is_function_token(parser->current))
        {
            std::vector<std::shared_ptr<Ty>> vars;
            while((is_function_token(parser->current)))
            {
                vars.push_back(paren_type(parser));
            }
            auto app = std::make_shared<TyAppl>(prev, vars);
            return app;
        }
        else
        {
            return prev;
        }
    }
    
    bool isPointerOperator(Token t)
    {
        auto start = t.start;
        while(start != t.start + t.length)
        {
            switch(*start)
            {
                case '*':
                case '&':
                    start++;
                default: return false;
            }
        }
        return true;
    }

    std::shared_ptr<Ty> array_or_pointer_type(Parser* parser)
    {
        auto t = applied_type(parser);
        std::shared_ptr<Ty> prev = nullptr;
        while(parser->current.type == TokenTypes::BRACKET || parser->current.type == TokenTypes::STAR)
        {
            if(parser->current.type == TokenTypes::BRACKET)
            {
                advance(parser);
                auto arrPrev = prev;
                std::shared_ptr<Ty> arrayOf = nullptr;
                std::optional<size_t> size = {};

                if(arrPrev != nullptr)
                {
                    arrayOf = arrPrev;
                }
                else
                {
                    arrayOf = t;
                }
                if(parser->current.type == TokenTypes::INT)
                {
                    size = std::make_optional<size_t>(atoi(parser->current.start));
                    advance(parser);
                }
                if(parser->current.type == TokenTypes::CLOSE_BRACKET)
                {
                    advance(parser);
                }
                else
                {
                    error(parser, "expected ']' in array type!");
                }
                auto arr = std::make_shared<TyArray>(arrayOf, size);
                prev = arr;
            }
            else if(parser->current.type == TokenTypes::OPERATOR)
            {
                if(isPointerOperator(parser->current))
                {
                    advance(parser);
                    auto ptrPrev = prev;
                    std::shared_ptr<Ty> pointsTo = nullptr;
                    if(ptrPrev != nullptr)
                    {
                        pointsTo = ptrPrev;
                    }
                    else
                    {
                        pointsTo = t;
                    }
                    auto ptr = std::make_shared<TyPointer>(pointsTo);
                    prev = ptr;
                }
            }
        }
        if(prev != nullptr) return prev;
        else return t;
    }
    
    std::shared_ptr<Ty> function_type(Parser* parser)
    {
        auto in = array_or_pointer_type(parser);
        if(parser->current.type == TokenTypes::ARROW)
        {
            advance(parser);
            return std::make_shared<TyFunc>(in, resolve_type_nogeneric(parser));
        }
        else return in;
    }
    
    std::shared_ptr<Ty> resolve_type_nogeneric(Parser *parser)
    {
        return function_type(parser);
    }
    
    std::shared_ptr<Ty> resolve_type(Parser *parser)
    {
        const char* start = parser->current.start;
        std::shared_ptr<Ty> type = resolve_type_nogeneric(parser);
        const char* end = parser->current.start + parser->current.length;
        if(type == nullptr) return newGenericType();
        type->start = start;
        type->end = end;
        return type;
    }

    std::shared_ptr<node> parsePattern(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        advance(parser);
        if(patternRules.find(parser->previous.type) == patternRules.end()) return nullptr;
        ParseRule *p = &patternRules.at(parser->previous.type);
        if( p->prefix == nullptr)
        {
            error(parser, "could not find a pattern to match!");
            return nullptr;
        }
        std::shared_ptr<node> result = p->prefix(parser, scope);
        
        while(patternRules.find(parser->current.type) != patternRules.end() && patternRules.at(parser->current.type).precedence != PRECEDENCE_NONE)
        {
            advance(parser);
            p = &patternRules.at(parser->previous.type);
            if(p->infix != nullptr) result = p->infix(parser, scope, result);
        }
        return result;
    }

    std::shared_ptr<node> range(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node> expression1)
    {
        auto start = expression1->start;
        auto op = parser->previous;
        if(patternRules.find(parser->current.type) != patternRules.end())
        {
            auto expression2 = parsePattern(parser, scope);
            auto end = expression2->end;
            auto binaryNode = std::make_shared<RangePatternNode>(expression1, expression2, op.type == TokenTypes::INCLUSIVE_RANGE ? true : false, start, end);
            return binaryNode;
        }
        else
        {
            auto end = parser->previous.start + parser->previous.length;
            auto unaryNode = std::make_shared<EllipsePatternNode>(expression1, start, end);
            return unaryNode;
        }
    }

    std::shared_ptr<node>pattern_grouping(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        std::shared_ptr<node> expr = parsePattern(parser, scope);
        switch(parser->current.type)
        {
            case TokenTypes::CLOSE_PAREN:
                advance(parser);
                return expr;
            case TokenTypes::COMMA:
            {
                advance(parser);
                std::vector<std::shared_ptr<node>> values;
                values.push_back(expr);
                while(parser->current.type != TokenTypes::CLOSE_PAREN)
                {
                    values.push_back(parsePattern(parser, scope));
                    if(parser->current.type != TokenTypes::CLOSE_PAREN)
                        consume(parser, TokenTypes::COMMA, "expected ',' between tuple values!");
                }
                consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after tuple constructor!");
                auto end = parser->previous.start + parser->previous.length;
                auto tuple = std::make_shared<TupleConstructorNode>(values, start, end);
                return tuple;
            }
        }
        error(parser, "Invalid expression!");
        return nullptr;
    }
    std::shared_ptr<node>pattern_array_constructor(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        bool hasEllipse = false;
        std::vector<std::shared_ptr<node>> values;
        while (parser->current.type != TokenTypes::CLOSE_BRACKET)
        {
            values.push_back(parsePattern(parser, scope));
            if (parser->current.type != TokenTypes::CLOSE_BRACKET)
            {
                consume(parser, TokenTypes::COMMA, "expected ',' after expression!");
            }
        }
        consume(parser, TokenTypes::CLOSE_BRACKET, "expected ']' after expression!");
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<ArrayConstructorNode>(values, start, end);
        return std::static_pointer_cast<node>(n);
    }
    std::shared_ptr<node>pattern_function_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node> expression1)
    {
        auto start = expression1->start;
        auto called = expression1;
        std::vector<std::shared_ptr<node>> args;
        while (parser->current.type != TokenTypes::CLOSE_PAREN)
        {
            args.push_back(parsePattern(parser, scope));
            if (parser->current.type != TokenTypes::CLOSE_PAREN)
            {
                consume(parser, TokenTypes::COMMA, "expected ',' after pattern expression!");
            }
        }
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after pattern expression!");
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<FunctionCallNode>(called, args, start, end);
        return std::static_pointer_cast<node>(n);
    }

    std::unordered_map<std::string, std::shared_ptr<Ty>> getIdentifiers(std::shared_ptr<node> n)
    {
        switch(n->nodeType)
        {
            case NODE_IDENTIFIER:
            {
                auto id = std::static_pointer_cast<VariableNode>(n);
                return {{tokenToString(id->variable), newGenericType()}};
            }
            case NODE_TYPE:
            {
                return {};
            }
            case NODE_ARRAYCONSTRUCTOR:
            {
                auto ac = std::static_pointer_cast<ArrayConstructorNode>(n);
                std::unordered_map<std::string, std::shared_ptr<Ty>> t;
                for(auto v : ac->values)
                {
                    auto last = getIdentifiers(v);
                    t.merge(last);
                }
                return t;
            }
            case NODE_TUPLE:
            {
                auto tuple = std::static_pointer_cast<TupleConstructorNode>(n);
                std::unordered_map<std::string, std::shared_ptr<Ty>> t;
                for(auto v : tuple->values)
                {
                    auto last = getIdentifiers(v);
                    t.merge(last);
                }
                return t;
            }
            case NODE_PLACEHOLDER:
            {
                return {};
            }
            case NODE_ELLIPSE:
            {
                auto ellipse = std::static_pointer_cast<EllipsePatternNode>(n);
                return getIdentifiers(ellipse->expr);
            }
            case NODE_RANGE:
            {
                auto range = std::static_pointer_cast<RangePatternNode>(n);
                auto t = getIdentifiers(range->expression1);
                t.merge(getIdentifiers(range->expression2));
                return t;
            }
            case NODE_FUNCTIONCALL:
            {
                auto fc = std::static_pointer_cast<FunctionCallNode>(n);
                std::unordered_map<std::string, std::shared_ptr<Ty>> t;
                for(auto arg : fc->args)
                {
                    auto last = getIdentifiers(arg);
                    t.merge(last);
                }
                return t;
            }
            default: return {};
        }
    }

    bool isRefutable(std::shared_ptr<node> n)
    {
        switch(n->nodeType)
        {
            case NODE_IDENTIFIER: return false;
            case NODE_TYPE: return true;
            case NODE_FUNCTIONCALL: return true;
            case NODE_ARRAYCONSTRUCTOR: return true;
            case NODE_PLACEHOLDER: return false;
            case NODE_TUPLE:
            {
                auto tuple = std::static_pointer_cast<TupleConstructorNode>(n);
                auto result = false;
                for(auto v : tuple->values)
                {
                    result |= isRefutable(v);
                }
                return result;
            }
            default: return true;
        }
    }
    
    static std::shared_ptr<node> module_decl(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        consume(parser, TokenTypes::TYPE, "expected module name!");
        auto name = parser->previous;
        std::shared_ptr<BlockStatementNode> block = std::static_pointer_cast<BlockStatementNode>(block_stmt(parser, scope));
        auto result = std::make_shared<ModuleDeclarationNode>(name, block, block->scope, start, block->end);
        scope->namespaces.emplace(tokenToString(name), block->scope);
        return result;
    }

    static std::shared_ptr<node> var_decl(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        std::shared_ptr<Ty> type = nullptr;
        auto assigned = parsePattern(parser, scope);
        std::unordered_map<std::string, std::shared_ptr<Ty>> ids = getIdentifiers(assigned);
        
        std::shared_ptr<node> value = nullptr;
        if(parser->current.type == TokenTypes::COLON)
        {
            advance(parser);
            type = resolve_type(parser);
        }
        else type = newGenericType();

        if (parser->current.type == TokenTypes::EQUAL)
        {
            advance(parser);
            value = expression(parser, scope);
        }
        consume(parser, TokenTypes::SEMICOLON, "expected ';' after variable declaration!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<VariableDeclarationNode>(type, assigned, ids, value, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    static void op_decl(Parser* parser, std::shared_ptr<ScopeNode> scope)
    {
        ParseRule p { nullptr, nullptr, PRECEDENCE_NONE, false};
        bool isPrefix = false;
        bool isInfix = false;
        bool isPostfix = false;
        while(parser->current.type != TokenTypes::IDENTIFIER)
        {
            switch(parser->current.type)
            {
                case TokenTypes::PREFIX: isPrefix = true; break;
                case TokenTypes::INFIX: isInfix = true; break;
                case TokenTypes::POSTFIX: isPostfix = true; break;
                default:
                {
                    errorAtCurrent(parser, "invalid operator fixity!");
                    return;
                }
            }
            advance(parser);
            if(parser->current.type != TokenTypes::IDENTIFIER) consume(parser, TokenTypes::COMMA, "expected ',' between fixity types!");
        }
    
        if(isPrefix) p.prefix = unary;
        if(isInfix) p.infix = binary;
        if(isPostfix) p.isPostfix = true;
    
        auto op = parser->current;
        advance(parser);
        consume(parser, TokenTypes::INT, "expected precedence for operator declaration!");
        p.precedence = std::stoi(tokenToString(parser->previous))+2;
        if(p.precedence < 2 || p.precedence > 11) error(parser, "precedence must be between 0 and 9!");
        
        scope->opRules.emplace(tokenToString(op), p);
        consume(parser, TokenTypes::SEMICOLON, "expected ';' after operator declaration!");
    }
    
    static std::shared_ptr<node> func_decl(Parser *parser, std::shared_ptr<ScopeNode>scope, std::shared_ptr<Ty> isImplOf)
    {
        auto start = parser->previous.start;
        std::shared_ptr<Ty> returnType = nullptr;
        auto identifier = parser->current;
        auto fn = scope->functions.find(tokenToString(identifier));
        if (fn != scope->functions.end())
        {
            if (fn->second->body != nullptr) 
            {
                errorAtCurrent(parser, "redefinition of existing function!");
                return nullptr;
            }
        }
    
        advance(parser);
        consume(parser, TokenTypes::PAREN, "expected '(' before parameter list!");
        std::vector<Parameter> params;
        while (parser->current.type != TokenTypes::CLOSE_PAREN)
        {
            Parameter p;
            p.identifier = parser->current;
            advance(parser);
            if(parser->current.type == TokenTypes::COLON)
            {
                advance(parser);
                p.type = resolve_type_nogeneric(parser);
            }
            else
            {
                p.type = newGenericType();
            }
            params.push_back(p);
            if (parser->current.type == TokenTypes::COMMA)
                advance(parser);
        }
        if(params.size() == 0)
        {
            Parameter _void;
            auto t = std::make_shared<TyBasic>("Void");
            _void.type = t;
            _void.identifier = {TokenTypes::IDENTIFIER, "", 0, identifier.line};
            params.push_back(_void);
        }
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after parameters!");
        if(parser->current.type == TokenTypes::COLON)
        {
            advance(parser);
            returnType = resolve_type_nogeneric(parser);
        }
        std::shared_ptr<node> body = nullptr;
        if (parser->current.type == TokenTypes::BRACE)
        {
            body = block_stmt(parser, scope);
            std::shared_ptr<ScopeNode> s = std::static_pointer_cast<BlockStatementNode>(body)->scope;
            for(size_t i = 0; i < params.size(); i++)
            {
                
                auto assigned = std::make_shared<VariableNode>(params[i].identifier);
                auto type = params[i].type;
                std::unordered_map<std::string, std::shared_ptr<Ty>> ids = {{tokenToString(params[i].identifier), params[i].type}};
                auto value = std::make_shared<PlaceholderNode>();
                auto vd = std::make_shared<VariableDeclarationNode>(type, assigned, ids, value);
                s->variables.insert(std::make_pair(tokenToString(params[i].identifier), vd));
            }
        }
        else
        {
            consume(parser, TokenTypes::SEMICOLON, "expected '{' or ';' after function parameters!");
        }

        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<FunctionDeclarationNode>(returnType, identifier, params, body, start, end);
    
        if(isImplOf == nullptr) 
        {
            scope->functions.insert(std::make_pair(tokenToString(identifier), result));
        }
        else 
        {
            if(scope->functionImpls.find(tokenToString(identifier)) == scope->functionImpls.end())
            {
                scope->functionImpls.insert(std::make_pair(tokenToString(identifier), std::unordered_map<std::string, std::shared_ptr<FunctionDeclarationNode>>()));
            }
    
            auto impls = scope->functionImpls.at(tokenToString(identifier));
    
            if(impls.find(typeToString(isImplOf)) == impls.end())
            {
                impls.insert(std::make_pair(typeToString(isImplOf), result));
            }
            //error?
        }
        return std::static_pointer_cast<node>(result);
    }
    
    static std::shared_ptr<node> _struct(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        auto nodeType = NODE_STRUCTDECL;
        auto typeDefined = resolve_type_nogeneric(parser);
        auto s = newScope(scope);
        scope->namespaces.emplace(declarationName(typeDefined), s);
        consume(parser, TokenTypes::BRACE, "expected '{' after type name!");
        std::vector<Parameter> fields;
        while (parser->current.type != TokenTypes::CLOSE_BRACE)
        {
            auto pidentifier = parser->current;
            advance(parser);
            std::shared_ptr<Ty> ptype = nullptr;
            if(parser->current.type == TokenTypes::COLON)
            {
                advance(parser);
                ptype = resolve_type_nogeneric(parser);
                if(ptype == nullptr)
                {
                    errorAtCurrent(parser, "expected type for struct field!");
                }
            }
            Parameter p {ptype, pidentifier};
            fields.push_back(p);
            consume(parser, TokenTypes::SEMICOLON, "expected ';' after field!");
        }
        consume(parser, TokenTypes::CLOSE_BRACE, "expected '}' after struct or union declaration!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<StructDeclarationNode>(typeDefined, fields, s, start, end);
        {    
            scope->structs.insert(std::make_pair(declarationName(typeDefined), result));
        }
        return std::static_pointer_cast<node>(result);
    }
    
static std::shared_ptr<node> _union(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        auto nodeType = NODE_UNIONDECL;
        auto typeDefined = resolve_type_nogeneric(parser);
        consume(parser, TokenTypes::BRACE, "expected '{' after type name!");
        auto s = newScope(scope);
        scope->namespaces.emplace(declarationName(typeDefined), s);
        std::vector<Parameter> fields;
        while (parser->current.type != TokenTypes::CLOSE_BRACE)
        {
            auto pidentifier = parser->current;
            advance(parser);
            std::shared_ptr<Ty> ptype = nullptr;
            if(parser->current.type == TokenTypes::PAREN)
            {
                ptype = resolve_type_nogeneric(parser);
            }
            if(ptype == nullptr){
                s->tyCons.emplace(tokenToString(pidentifier), std::make_shared<TypeNode>(typeDefined));
            }
            else
            {
                auto f = std::make_shared<TyFunc>(ptype, typeDefined);
                s->tyCons.emplace(tokenToString(pidentifier), std::make_shared<TypeNode>(f));
            }
            Parameter p {ptype, pidentifier};
            fields.push_back(p);
            if(parser->current.type != TokenTypes::CLOSE_BRACE) consume(parser, TokenTypes::COMMA, "expected ',' after union member!");
        }
        consume(parser, TokenTypes::CLOSE_BRACE, "expected '}' after union declaration!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<UnionDeclarationNode>(typeDefined, fields, s, start, end);
        scope->unions.insert(std::make_pair(declarationName(typeDefined), result));
        return std::static_pointer_cast<node>(result);
    }

    static std::shared_ptr<node> typedef_decl(Parser *parser, std::shared_ptr<ScopeNode>scope)
    {
        auto start = parser->previous.start;
        auto nodeType = NODE_TYPEDEF;
        advance(parser);
        auto typeDefined = resolve_type(parser);
        consume(parser, TokenTypes::EQUAL, "expected '=' after defined type!");
        auto typeAliased = resolve_type(parser);
        consume(parser, TokenTypes::SEMICOLON, "expected ';' after typedef");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<TypedefNode>(typeDefined, typeAliased, start, end);
        if(typeDefined == nullptr) assert(false);
        scope->typeAliases.insert(std::make_pair(declarationName(typeDefined), result));
        return std::static_pointer_cast<node>(result);
    }
    
    static std::shared_ptr<node> class_decl(Parser *parser, std::shared_ptr<ScopeNode>scope)
    {
        auto start = parser->previous.start;
        advance(parser);
        auto className = parser->current;
        advance(parser);
        auto typeName = parser->current;
        advance(parser);
        std::vector<std::shared_ptr<Ty>> constraints;
        if (parser->current.type == TokenTypes::COLON)
        {
            advance(parser);
            while (parser->current.type != TokenTypes::BRACE)
            {
                auto constraint = resolve_type(parser);
                constraints.push_back(constraint);
                if (parser->current.type != TokenTypes::BRACE)
                {
                    consume(parser, TokenTypes::COMMA, "expected ',' between class constraints!");
                }
            }
        }
    
        consume(parser, TokenTypes::BRACE, "expected '{' after class declaration!");
        std::vector<std::shared_ptr<node>> functions;
        while (parser->current.type != TokenTypes::CLOSE_BRACE)
        {
            consume(parser, TokenTypes::FN, "expected function declaration!");
            std::shared_ptr<node> func = func_decl(parser, scope, nullptr);
            functions.push_back(func);
        }
        auto s = newScope(scope);
        for(auto f : functions)
        {
            auto fd = std::static_pointer_cast<FunctionDeclarationNode>(f);
            s->functions.emplace(tokenToString(fd->identifier), fd);
        }
        scope->namespaces.emplace(tokenToString(className), s);
        consume(parser, TokenTypes::CLOSE_BRACE, "expected '}' after function declarations!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<ClassDeclarationNode>(className, typeName, constraints, functions, start, end);
        scope->classes.insert(std::make_pair(tokenToString(result->className), result));
        return std::static_pointer_cast<node>(result);
    }
    
    static std::shared_ptr<node> impl_decl(Parser* parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        advance(parser);
        auto nodeType = NODE_CLASSIMPL;
        auto _class = parser->current;
        advance(parser);
        auto implemented = resolve_type(parser);
        consume(parser, TokenTypes::BRACE, "expected '{' before function definitions!");
        std::vector<std::shared_ptr<node>> functions;
        while(parser->current.type != TokenTypes::CLOSE_BRACE)
        {
            consume(parser, TokenTypes::FN, "expected function declaration in class implementation!");
            auto fd = func_decl(parser, scope, implemented);
            functions.push_back(fd);
        }
        consume(parser, TokenTypes::CLOSE_BRACE, "expected '}' after function definitions!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<ClassImplementationNode>(_class, implemented, functions, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    std::shared_ptr<node> declaration(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        switch (parser->current.type)
        {
        case TokenTypes::LET:
        {
            advance(parser);
            return var_decl(parser, scope);
        }
        case TokenTypes::FN:
        {
            advance(parser);
            return func_decl(parser, scope, nullptr);
        }
        case TokenTypes::STRUCT:
            advance(parser);
            return _struct(parser, scope);

        case TokenTypes::UNSAFE:
        {
            switch(parser->next.type)
            {
                case TokenTypes::FN:
                {
                    //functions, classes, types, and statements
                }
            }
        }
        case TokenTypes::UNION:
            advance(parser);
            return _union(parser, scope);
        case TokenTypes::TYPEDEF:
            advance(parser);
            return typedef_decl(parser, scope);
        case TokenTypes::MODULE:
            advance(parser);
            return module_decl(parser, scope);
        case TokenTypes::CLASS:
            return class_decl(parser, scope);
        case TokenTypes::IMPLEMENT:
            return impl_decl(parser, scope);
        case TokenTypes::PREFIX:
        case TokenTypes::INFIX:
        case TokenTypes::POSTFIX:
            op_decl(parser, scope);
            return nullptr;
        default:
            return statement(parser, scope);
        }
    }
    
    static std::shared_ptr<node> switch_stmt(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        advance(parser);
        consume(parser, TokenTypes::PAREN, "expected '(' after 'switch!'");
        auto switchExpr = expression(parser, scope);
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after expression!");
        consume(parser, TokenTypes::BRACE, "expected '{' at the start of switch block!");
        std::vector<std::shared_ptr<node>> cases;
        while (parser->current.type == TokenTypes::CASE)
        {
            auto start = parser->current.start;
            auto caseScope = newScope(scope);
            consume(parser, TokenTypes::CASE, "expect 'case' in switch block!");
            auto caseExpr = parsePattern(parser, caseScope);
            auto ids = getIdentifiers(caseExpr);
            if(!ids.empty())
            {
                auto t = newGenericType();
                auto vd = std::make_shared<VariableDeclarationNode>(t, caseExpr, ids, switchExpr);
                for(auto id : ids)
                {
                    caseScope->variables.emplace(id.first, vd);
                }
            }
            consume(parser, TokenTypes::COLON, "expected ':' after expression!");
            auto caseStmt = statement(parser, caseScope);
            auto end = parser->previous.start + parser->previous.length;
            auto current = std::make_shared<CaseNode>(caseExpr, caseStmt, caseScope, start, end);
            cases.push_back(current);
        } 

        consume(parser, TokenTypes::CLOSE_BRACE, "expect '}' after switch block!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<SwitchStatementNode>(switchExpr, cases, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    static std::shared_ptr<node> for_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
    {
        auto start = parser->previous.start;
        advance(parser);
        std::shared_ptr<node> initExpr = nullptr;
        std::shared_ptr<node> condExpr = nullptr;
        std::shared_ptr<node> incrementExpr = nullptr;
        std::shared_ptr<node> loopStmt = nullptr;
        consume(parser, TokenTypes::PAREN, "expected '(' after 'for!'");
        if (parser->current.type != TokenTypes::SEMICOLON)
        {
            switch(parser->current.type)
            {
                case TokenTypes::LET:
                    initExpr = var_decl(parser, scope);
                    break;
                default:
                    initExpr = expression(parser, scope);
                    break;
            }
        }
        consume(parser, TokenTypes::SEMICOLON, "expected ';' after declaration/expression!");
        if (parser->current.type != TokenTypes::SEMICOLON)
        {
            condExpr = expression(parser, scope);
        }
        consume(parser, TokenTypes::SEMICOLON, "expected ';' after condition expression!");
        if (parser->current.type != TokenTypes::CLOSE_PAREN)
        {
            incrementExpr = expression(parser, scope);
        }
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after iteration expression!");
        loopStmt = statement(parser, scope);
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<ForStatementNode>(initExpr, condExpr, incrementExpr, loopStmt, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    static std::shared_ptr<node> if_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
    {
        auto start = parser->previous.start;
        advance(parser);
        consume(parser, TokenTypes::PAREN, "expected '(' after 'if!'");
        auto branchExpr = expression(parser, scope);
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after expression!");
        auto thenStmt = statement(parser, scope);
        std::shared_ptr<node> elseStmt;
        if (parser->current.type == TokenTypes::ELSE)
        {
            advance(parser);
            elseStmt = statement(parser, scope);
        }
        else
        {
            elseStmt = nullptr;
        }
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<IfStatementNode>(branchExpr, thenStmt, elseStmt, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    static std::shared_ptr<node> while_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
    {
        auto start = parser->previous.start;
        advance(parser);
        consume(parser, TokenTypes::PAREN, "expected '(' after 'while!'");
        auto loopExpr = expression(parser, scope);
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after expression!");
        auto loopStmt = statement(parser, scope);
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<WhileStatementNode>(loopExpr, loopStmt, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    std::shared_ptr<node> block_stmt(Parser *parser, std::shared_ptr<ScopeNode>scope)
    {
        advance(parser);
        auto start = parser->previous.start;
        std::shared_ptr<ScopeNode> next = newScope(scope);
        scope->childScopes.push_back(next);
        scope = next;
        std::vector<std::shared_ptr<node>> declarations;
        while (parser->current.type != TokenTypes::CLOSE_BRACE)
        {
            auto dec = declaration(parser, scope);
            if(dec != nullptr) declarations.push_back(dec);
        }
        consume(parser, TokenTypes::CLOSE_BRACE, "expect '}' at end of block!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<BlockStatementNode>(declarations, scope, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    static std::shared_ptr<node> return_stmt(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        advance(parser);
        auto returnExpr = expression(parser, scope);
        consume(parser, TokenTypes::SEMICOLON, "expected ';' after return expression!");
        auto end = parser->previous.start + parser->previous.length;
        auto result = std::make_shared<ReturnStatementNode>(returnExpr, start, end);
        return std::static_pointer_cast<node>(result);
    }
    
    std::shared_ptr<node>statement(Parser *parser, std::shared_ptr<ScopeNode>scope)
    {
        switch (parser->current.type)
        {
            case TokenTypes::SWITCH:
                return switch_stmt(parser, scope);
            case TokenTypes::FOR:
                return for_stmt(parser, scope);
            case TokenTypes::IF:
                return if_stmt(parser, scope);
            case TokenTypes::WHILE:
                return while_stmt(parser, scope);
            case TokenTypes::BRACE:
                return block_stmt(parser, newScope(scope));
            case TokenTypes::RETURN:
                return return_stmt(parser, scope);
            case TokenTypes::BREAK:
            {
                advance(parser);
                std::shared_ptr<node> result = std::make_shared<node>(NODE_BREAK, parser->previous.start, parser->previous.start + parser->previous.length);
                result->nodeType = NODE_BREAK;
                consume(parser, TokenTypes::SEMICOLON, "expected ';' after 'break!'");
                return result;
            }
            case TokenTypes::CONTINUE:
            {
                advance(parser);
                std::shared_ptr<node> result = std::make_shared<node>(NODE_CONTINUE, parser->previous.start, parser->previous.start + parser->previous.length);
                consume(parser, TokenTypes::SEMICOLON, "expected ';' after 'continue!'");
                return result;
            }
            default:
            {
                if(parser->current.type == TokenTypes::SEMICOLON){ advance(parser); return nullptr; }
                std::shared_ptr<node>result = expression(parser, scope);
                consume(parser, TokenTypes::SEMICOLON, "expected ';' after expression!");
                return result;
            }
        }
    }
    
static ParseRule *getRule(TokenTypes type)
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
    
    static std::shared_ptr<node> parsePrecedence(Parser *parser, uint8_t prec, std::shared_ptr<ScopeNode> scope)
    {
        advance(parser);
        if(rules.find(parser->previous.type) != rules.end())
        {
            ParseRule *p = getRule(parser->previous.type);
            if ( p->prefix == nullptr)
            {
                error(parser, "expected an expression!");
                return nullptr;
            }

            std::shared_ptr<node> result = p->prefix(parser, scope);

            while (rules.find(parser->current.type) != rules.end() && prec <= (parser->current.type == TokenTypes::OPERATOR ? getOperatorPrecedence(parser->current, scope) : getRule(parser->current.type)->precedence))
            {
                advance(parser);
                p = getRule(parser->previous.type);
                if(parser->previous.type == TokenTypes::OPERATOR)
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
        error(parser, "expected an expression!");
        return nullptr;
    }
    
    std::shared_ptr<node>expression(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        return parsePrecedence(parser, PRECEDENCE_ASSIGNMENT, scope);
    }
    
    std::shared_ptr<node>literal(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        auto value = parser->previous;
        auto end = parser->previous.start + parser->previous.length;
        return std::make_shared<LiteralNode>(value, start, end);
    }
    
    std::shared_ptr<node>placeholder(Parser* parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        auto end = parser->previous.start + parser->previous.length;
        return std::make_shared<PlaceholderNode>(start, end);
    }

    std::shared_ptr<node> _type(Parser* parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        if(parser->current.type == TokenTypes::DOT)
        {
            auto name = parser->previous;
            advance(parser);
            auto expr = parsePrecedence(parser, PRECEDENCE_PRIMARY, scope);
            auto end = parser->previous.start + parser->previous.length;
            return std::make_shared<NamespaceNode>(name, expr, start, end);            
        }
        else
        {
            auto t = std::make_shared<TyBasic>(tokenToString(parser->previous));
            auto end = parser->previous.start + parser->previous.length;
            return std::make_shared<TypeNode>(t, start, end);
        }
    }
    
    std::shared_ptr<node> list_init(Parser* parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node> expression1)
    {
        auto start = expression1->start;
        std::vector<Token> fieldNames;
        std::vector<std::shared_ptr<node>> values;
        while(parser->current.type != TokenTypes::CLOSE_BRACE)
        {
            auto temp = expression(parser, scope);
            if(parser->current.type == TokenTypes::COLON)
            {
                if(temp->nodeType != NODE_IDENTIFIER) error(parser, "expected an identifier before ':' in list initializer!");
                else
                {
                    auto literal = std::static_pointer_cast<VariableNode>(temp)->variable;
                    fieldNames.push_back(literal);
                }
                advance(parser);
                values.push_back(expression(parser, scope));
            }
            else
            {
                values.push_back(temp);
            }
            if(parser->current.type == TokenTypes::COMMA)
            {
                advance(parser);
            }
        }
        consume(parser, TokenTypes::CLOSE_BRACE, "expected '}' after list initialization!");
        auto end = parser->previous.start + parser->previous.length;
        return std::make_shared<ListInitNode>(expression1, fieldNames, values, start, end);
    }
    
    std::shared_ptr<node>identifier(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        auto variable = parser->previous;
        auto end = parser->previous.start + parser->previous.length;
        return std::make_shared<VariableNode>(variable, start, end);
    }
    
    std::shared_ptr<node>lambda(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        auto returnType = newGenericType();
        consume(parser, TokenTypes::PAREN, "expected '(' before lambda arguments!");
        std::vector<Parameter> params;
        while(parser->current.type != TokenTypes::CLOSE_PAREN)
        {
            Parameter p;
            p.type = resolve_type(parser);
            p.identifier = parser->current;
            advance(parser);
            params.push_back(p);
            if (parser->current.type == TokenTypes::COMMA)
                advance(parser);
        }
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after lambda arguments!");
        auto body = block_stmt(parser, scope);
        std::shared_ptr<ScopeNode> s = std::static_pointer_cast<BlockStatementNode>(body)->scope;
            for(size_t i = 0; i < params.size(); i++)
            {
                auto identifier = std::make_shared<VariableNode>(params[i].identifier);
                std::unordered_map<std::string, std::shared_ptr<Ty>> ids = {{tokenToString(params[i].identifier), params[i].type}};
                auto value = std::make_shared<PlaceholderNode>();
                auto vd = std::make_shared<VariableDeclarationNode>(params[i].type, identifier, ids, value);
                s->variables.insert(std::make_pair(tokenToString(params[i].identifier), vd)); 
            }
        auto end = parser->previous.start + parser->previous.length;
        return std::make_shared<LambdaNode>(returnType, params, body, start, end);
    }
    
    std::shared_ptr<node>grouping(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        std::shared_ptr<node> expr = expression(parser, scope);
        switch(parser->current.type)
        {
            case TokenTypes::CLOSE_PAREN:
                advance(parser);
                return expr;
            case TokenTypes::COMMA:
            {
                advance(parser);
                std::vector<std::shared_ptr<node>> values;
                values.push_back(expr);
                while(parser->current.type != TokenTypes::CLOSE_PAREN)
                {
                    values.push_back(expression(parser, scope));
                    if(parser->current.type != TokenTypes::CLOSE_PAREN)
                        consume(parser, TokenTypes::COMMA, "expected ',' between tuple values!");
                }
                consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after tuple constructor!");
                auto end = parser->previous.start + parser->previous.length;
                auto tuple = std::make_shared<TupleConstructorNode>(values, start, end);
                return tuple;
            }
        }
        error(parser, "Invalid expression!");
        return nullptr;
    }
    
    std::shared_ptr<node>unary(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        auto op = parser->previous;
        auto expr = expression(parser, scope);
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<UnaryNode>(op, expr, start, end);
        return std::static_pointer_cast<node>(n);
    }
    
    std::shared_ptr<node>binary(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
    {
        ParseRule* rule = nullptr;
        if(rules.find(parser->previous.type) != rules.end())
        {
            rule = &rules[parser->previous.type];
        }
        if(rule != nullptr && rule->precedence == PRECEDENCE_UNKNOWN)
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
        if(rule != nullptr && rule->isPostfix)
        {

            ParseRule *next = nullptr;
            if(rules.find(parser->current.type) == rules.end() || rules.at(parser->current.type).prefix == nullptr)
            {
                auto start = expression1->start;
                auto expr = expression1;
                auto op = parser->previous;
                auto end = parser->previous.start + parser->previous.length;
                auto n = std::make_shared<UnaryNode>(op, expr, start, end);
                return std::static_pointer_cast<node>(n);
            }
        }
        auto start = expression1->start;
        auto op = parser->previous;
        auto expression2 = parsePrecedence(parser, Precedence(rule->precedence + 1), scope);
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<BinaryNode>(expression1, op, expression2, start, end);
        return std::static_pointer_cast<node>(n);
    }
    
    std::shared_ptr<node>assignment(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
    {
        auto start = expression1->start;
        auto variable = expression1;
        auto assignment = expression(parser, scope);
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<AssignmentNode>(variable, assignment, start, end);
        return std::static_pointer_cast<node>(n);
    }
    
    std::shared_ptr<node>field_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
    {
        auto start = expression1->start;
        auto expr = expression1;
        auto field = parser->current;
        consume(parser, TokenTypes::IDENTIFIER, "Expected an identifier!");
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<FieldCallNode>(expr, field, start, end);
        return std::static_pointer_cast<node>(n);
    }
    
    std::shared_ptr<node>array_constructor(Parser *parser, std::shared_ptr<ScopeNode> scope)
    {
        auto start = parser->previous.start;
        std::vector<std::shared_ptr<node>> values;
        while (parser->current.type != TokenTypes::CLOSE_BRACKET)
        {
            values.push_back(expression(parser, scope));
            if (parser->current.type != TokenTypes::CLOSE_BRACKET)
            {
                consume(parser, TokenTypes::COMMA, "expected ',' after expression!");
            }
        }
        consume(parser, TokenTypes::CLOSE_BRACKET, "expected ']' after expression!");
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<ArrayConstructorNode>(values, start, end);
        return std::static_pointer_cast<node>(n);
    }
    
    std::shared_ptr<node>function_call(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
    {
        auto start = expression1->start;
        auto called = expression1;
        std::vector<std::shared_ptr<node>> args;
        while (parser->current.type != TokenTypes::CLOSE_PAREN)
        {
            args.push_back(expression(parser, scope));
            if (parser->current.type != TokenTypes::CLOSE_PAREN)
            {
                consume(parser, TokenTypes::COMMA, "expected ',' after expression!");
            }
        }
        consume(parser, TokenTypes::CLOSE_PAREN, "expected ')' after expression!");
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<FunctionCallNode>(called, args, start, end);
        return std::static_pointer_cast<node>(n);
    }
    
    std::shared_ptr<node>array_index(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node>expression1)
    {
        auto start = expression1->start;
        auto array = expression1;
        auto index = expression(parser, scope);
        consume(parser, TokenTypes::CLOSE_BRACKET, "expected ']' after expression!");
        auto end = parser->previous.start + parser->previous.length;
        auto n = std::make_shared<ArrayIndexNode>(array, index, start, end);
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
                printf("Type: Variable Declaration; Type: %s\n", typeToString(n->type).c_str());
                printf("Assigned: \n");
                printNodes(n->assigned, depth + 1);
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
            case NODE_STRUCTDECL:
            {
                auto n = std::static_pointer_cast<StructDeclarationNode>(start);
                printf("Type: STRUCT; Name: %s\n", typeToString(n->typeDefined).c_str());
                bool first = false;
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
            case NODE_UNIONDECL:
            {
                auto n = std::static_pointer_cast<UnionDeclarationNode>(start);
                printf("Type: UNION; Name: %s\n", typeToString(n->typeDefined).c_str());
                bool first = false;
                printf("Fields: ");
                for (size_t i = 0; i < n->members.size(); i++)
                {
                    if (first)
                        first = false;
                    else
                        printf(", ");
                    printf("%s %.*s", n->members[i].type == nullptr ? "Void" : typeToString(n->members[i].type).c_str(), n->members[i].identifier.length, n->members[i].identifier.start);
        
                }
                printf("\n");
                break;
            }
            case NODE_CLASSDECL:
            {
                auto n = std::static_pointer_cast<ClassDeclarationNode>(start);
                printf("Type: Class; Name: %s\n", tokenToString(n->className).c_str());
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
                        
                        printf("%s", typeToString(n->constraints[i]).c_str());
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
                printf("Implemented Type: %s\n", typeToString(n->implemented).c_str());
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
            case NODE_TYPE:
            {
                auto n = std::static_pointer_cast<TypeNode>(start);
                printf("Type: Type Name; Value: %s.\n", typeToString(n->type).c_str());
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
                printNodes(n->type, depth + 1);
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
    
    std::shared_ptr<ProgramNode> parse(const char* src)
    {
        Lexer lexer = initLexer(src);
        Parser parser;
        parser.lexer = lexer;
        parser.hadError = false;
        parser.panicMode = false;
        advance(&parser);
        advance(&parser);
    
        auto ast = std::make_shared<ProgramNode>();
        ast->start = src;
        ast->nodeType = NODE_PROGRAM;
        ast->globalScope = newScope(nullptr);
        while (parser.current.type != TokenTypes::_EOF)
        {
            auto dec = declaration(&parser, ast->globalScope);
            if(dec != nullptr)
            {
                ast->declarations.push_back(dec);
            }
        }
        ast->end = parser.previous.start + parser.previous.length;
        ast->hadError = parser.hadError;
        if (!ast->hadError)
        {
            //printNodes(std::static_pointer_cast<node>(ast), 0);
            //printf("%.*s\n", ast->end - ast->start, ast->start);
            return ast;
        }
        return nullptr;
    }
}