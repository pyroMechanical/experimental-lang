#ifndef parser_header
#define parser_header

#include <memory>
#include <vector>
#include <unordered_map>
#include <deque>
#include <optional>
#include "lexer.h"

namespace pilaf {
    struct Parser {
        Lexer lexer;
        Token next;
        Token current;
        Token previous;
        bool hadError;
        bool panicMode;
    };
    
    enum Precedence
    {
        PRECEDENCE_NONE = 0,
        PRECEDENCE_APPLY = 1,
        PRECEDENCE_ASSIGNMENT = 2,
        PRECEDENCE_BITWISE_OR = 3,
        PRECEDENCE_BITWISE_AND = 4,
        PRECEDENCE_LOGICAL_OR = 5,
        PRECEDENCE_LOGICAL_AND = 6,
        PRECEDENCE_EQUALITY = 7,
        PRECEDENCE_COMPARISON = 8,
        PRECEDENCE_SHIFT = 9,
        PRECEDENCE_ADD = 10,
        PRECEDENCE_MULTIPLY = 11,
        PRECEDENCE_PREFIX = 12,
        PRECEDENCE_POSTFIX = 13,
        PRECEDENCE_UNKNOWN = 14,
        PRECEDENCE_PRIMARY = 15
    };
    
    struct node;
    struct ScopeNode;
    
    struct ParseRule
    {
        std::shared_ptr<node>(*prefix)(Parser *parser, std::shared_ptr<ScopeNode> scope);
        std::shared_ptr<node>(*infix)(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node> n);
        uint8_t precedence;
        bool isPostfix;
    };
    
    enum NodeType {
        NODE_UNDEFINED,
        NODE_PROGRAM,
        NODE_LIBRARY,
        NODE_IMPORT,
        NODE_MODULE,
        NODE_TYPE,
        NODE_NAMESPACE,
        NODE_DECLARATION,
        NODE_TYPEDEF,
        NODE_STRUCTDECL,
        NODE_UNIONDECL,
        NODE_FUNCTIONDECL,
        NODE_VARIABLEDECL,
        NODE_CLASSDECL,
        NODE_CLASSIMPL,
        NODE_FOR,
        NODE_IF,
        NODE_WHILE,  
        NODE_SWITCH,
        NODE_CASE,
        NODE_RETURN,
        NODE_BREAK,
        NODE_CONTINUE,
        NODE_BLOCK,
        NODE_ASSIGNMENT,
        NODE_BINARY,
        NODE_UNARY,
        NODE_FUNCTIONCALL,
        NODE_FIELDCALL,
        NODE_ARRAYCONSTRUCTOR,
        NODE_ARRAYINDEX,
        NODE_IDENTIFIER,
        NODE_LAMBDA,
        NODE_LITERAL,
        NODE_LISTINIT,
        NODE_TUPLE,
        NODE_PLACEHOLDER,
        NODE_SCOPE,
        NODE_RANGE,
        NODE_ELLIPSE,
        NODE_ERROR
    };
    
    struct node {
        NodeType nodeType;
        const char* start;
        const char* end;
    
        virtual bool hasError() {return false;}
        node(NodeType n, const char* s = nullptr, const char* e = nullptr)
            :nodeType(n), start(s), end(e) {}
    };
    
    struct Ty {
        enum TyNodeType
        {
            TY_CONSTRAINTS,
            TY_FUNCTION,
            TY_APPLICATION,
            TY_VAR,
            TY_TUPLE,
            TY_ARRAY,
            TY_POINTER,
            TY_REFERENCE,
            TY_ARRAYCON,
            TY_FUNCTIONCON,
            TY_TUPLECON,
            TY_BASIC
        };
        const TyNodeType type;
        const char* start;
        const char* end;
        Ty(TyNodeType t, const char* start, const char* end)
        :type(t), start(start), end(end) {};
    };
    
    struct TyFunc : public Ty {
        std::shared_ptr<Ty> in;
        std::shared_ptr<Ty> out;
        TyFunc(std::shared_ptr<Ty> in, std::shared_ptr<Ty> out, const char* s = nullptr, const char* e = nullptr)
        : in(in), out(out), Ty(Ty::TY_FUNCTION, s, e) {}
    };
    
    struct TyAppl : public Ty {
        std::shared_ptr<Ty> applied;
        std::vector<std::shared_ptr<Ty>> vars;
        TyAppl(std::shared_ptr<Ty> applied, std::vector<std::shared_ptr<Ty>> vars, const char* s = nullptr, const char* e = nullptr)
        : applied(applied), vars(vars), Ty(Ty::TY_APPLICATION, s, e) {}
    };
    
    struct TyVar : public Ty {
        std::string var;
        TyVar(std::string var, const char* s = nullptr, const char* e = nullptr)
        : var(var), Ty(Ty::TY_VAR, s, e) {}
    };
    
    struct TyBasic : public Ty {
        std::string t;
        TyBasic(std::string t, const char* s = nullptr, const char* e = nullptr)
        : t(t), Ty(Ty::TY_BASIC, s, e) {}
    };
    
    struct TyTuple : public Ty {
        std::vector<std::shared_ptr<Ty>> types;
        TyTuple(std::vector<std::shared_ptr<Ty>> types, const char* s = nullptr, const char* e = nullptr)
        : types(types), Ty(Ty::TY_TUPLE, s, e) {}
    };
    
    struct TyArray : public Ty {
        std::shared_ptr<Ty> arrayOf;
        std::optional<size_t> size;
        TyArray(std::shared_ptr<Ty> arrayOf, std::optional<size_t> size, const char* s = nullptr, const char* e = nullptr)
        : arrayOf(arrayOf), size(size), Ty(Ty::TY_ARRAY, s, e) {}
    };

    struct TyPointer : public Ty
    {
        std::shared_ptr<Ty> pointsTo;
        TyPointer(std::shared_ptr<Ty> pointsTo, const char* s = nullptr, const char* e = nullptr)
        : pointsTo(pointsTo), Ty(Ty::TY_POINTER, s, e) {}
    };

    struct TyRef : public Ty
    {
        std::shared_ptr<Ty> refTo;
        TyRef(std::shared_ptr<Ty> refTo, const char* s = nullptr, const char* e = nullptr)
        : refTo(refTo), Ty(Ty::TY_REFERENCE, s, e) {}
    };
    
    struct Parameter {
        std::shared_ptr<Ty> type;
        Token identifier;
    };
    
    struct TypedefNode : public node {
    std::shared_ptr<Ty> typeDefined;
    std::shared_ptr<Ty> typeAliased;

    virtual bool hasError()
    {
        return false;
    }
    TypedefNode(std::shared_ptr<Ty> td, std::shared_ptr<Ty> ta, const char* s = nullptr, const char* e = nullptr)
        :typeDefined(td), typeAliased(ta), node(NodeType::NODE_TYPEDEF, s, e) {}
    };

    struct ModuleDeclarationNode : public node
    {
        Token name;
        std::shared_ptr<node> block;
        std::shared_ptr<ScopeNode> scope;
        virtual bool hasError()
        {
            return block->hasError();
        }
        ModuleDeclarationNode(Token name, std::shared_ptr<node> block, std::shared_ptr<ScopeNode> scope, const char* s = nullptr, const char* e = nullptr)
        : name(name), block(block), scope(scope), node(NODE_MODULE, s, e) {}
    };
    
    struct StructDeclarationNode : public node {
    std::shared_ptr<Ty>  typeDefined;
    std::shared_ptr<Ty> kind;
    std::vector<Parameter> fields;
    std::shared_ptr<ScopeNode> scope;
    virtual bool hasError()
    {
        return false;
    }
    StructDeclarationNode(std::shared_ptr<Ty> t, std::vector<Parameter>f, std::shared_ptr<ScopeNode> scope, const char* s = nullptr, const char* e = nullptr)
        :typeDefined(t), kind(nullptr), fields(f), scope(scope), node(NodeType::NODE_STRUCTDECL, s, e) {}
    };

    struct UnionDeclarationNode : public node {
    std::shared_ptr<Ty>  typeDefined;
    std::shared_ptr<Ty> kind;
    std::vector<Parameter> members;
    std::shared_ptr<ScopeNode> scope;
    virtual bool hasError()
    {
        return false;
    }
    UnionDeclarationNode(std::shared_ptr<Ty> t, std::vector<Parameter>m, std::shared_ptr<ScopeNode> scope, const char* s = nullptr, const char* e = nullptr)
        :typeDefined(t), kind(nullptr), members(m), scope(scope), node(NodeType::NODE_UNIONDECL, s, e) {}
    };
    
    struct FunctionDeclarationNode : public node {
        std::shared_ptr<Ty> returnType;
        Token identifier;
        std::vector<Parameter> params;
        std::shared_ptr<node> body;
        virtual bool hasError()
        {
            return body? body->hasError() : false;
        }

        FunctionDeclarationNode(std::shared_ptr<Ty> rt, Token id, std::vector<Parameter> p, std::shared_ptr<node> b, const char* s = nullptr, const char* e = nullptr)
            :returnType(rt), identifier(id), params(p), body(b), node(NodeType::NODE_FUNCTIONDECL, s, e) {}  
    };
    
    struct VariableDeclarationNode : public node {
        std::shared_ptr<Ty> type;
        std::shared_ptr<node> assigned;
        std::unordered_map<std::string, std::shared_ptr<Ty>> identifiers;
        std::shared_ptr<node> value;
        virtual bool hasError()
        {
            return value? value->hasError() : false;
        }
        VariableDeclarationNode(std::shared_ptr<Ty>t, std::shared_ptr<node> assigned, std::unordered_map<std::string, std::shared_ptr<Ty>> ids, std::shared_ptr<node>v, const char* s = nullptr, const char* e = nullptr)
            :type(t), assigned(assigned), identifiers(ids), value(v), node(NodeType::NODE_VARIABLEDECL, s, e) {}
    };
    
    struct RangePatternNode : public node 
    {
        std::shared_ptr<node> expression1;
        std::shared_ptr<node> expression2;
        bool isInclusive;
        virtual bool hasError()
        {
            return expression1->hasError() || expression2->hasError();
        }
        RangePatternNode(std::shared_ptr<node> e1, std::shared_ptr<node> e2, bool inc, const char* s = nullptr, const char* e = nullptr)
        : expression1(e1), expression2(e2), isInclusive(inc) ,node(NodeType::NODE_RANGE, s, e){}
    };

    struct EllipsePatternNode : public node 
    {
        std::shared_ptr<node> expr;
        EllipsePatternNode (std::shared_ptr<node> expr, const char* s = nullptr, const char* e = nullptr)
        :expr(expr), node(NodeType::NODE_ELLIPSE, s, e) {}
    };

    struct ClassDeclarationNode : public node 
    {
        Token className;
        Token typeName;
        std::shared_ptr<Ty> kind;
        std::vector<std::shared_ptr<Ty>> constraints;
        std::vector<std::shared_ptr<node>> functions;
        virtual bool hasError()
        {
            bool result = false;
            for(auto f : functions)
            {
                result |= f->hasError();
            }
            return result;
        }
        ClassDeclarationNode(Token cn, Token tn, std::vector<std::shared_ptr<Ty>> c, std::vector<std::shared_ptr<node>> f, const char* s = nullptr, const char* e = nullptr)
            :className(cn), typeName(tn), constraints(c), functions(f), node(NodeType::NODE_CLASSDECL, s, e) {}
    };
    
    struct ClassImplementationNode : public node {
        Token _class;
        std::shared_ptr<Ty> implemented;
        std::vector<std::shared_ptr<node>> functions;
        virtual bool hasError()
        {
            bool result = false;
            for(auto f : functions)
            {
                result |= f->hasError();
            }
            return result;
        }
        ClassImplementationNode(Token c, std::shared_ptr<Ty> i, std::vector<std::shared_ptr<node>> f, const char* s = nullptr, const char* e = nullptr)
            :_class(c), implemented(i), functions(f), node(NodeType::NODE_CLASSIMPL, s, e){}
    };

    struct NamespaceNode : public node {
        Token name;
        std::shared_ptr<node> expr;
        virtual bool hasError()
        {
            return expr->hasError();
        }
        NamespaceNode(Token name, std::shared_ptr<node> expr, const char* s = nullptr, const char* e = nullptr)
            :name(name), expr(expr), node(NodeType::NODE_NAMESPACE, s, e) {};
    };

    struct TypeNode : public node {
        std::shared_ptr<Ty> type;
        virtual bool hasError()
        {
            return false;
        }
        TypeNode(std::shared_ptr<Ty>t, const char* s = nullptr, const char* e = nullptr)
            :type(t), node(NodeType::NODE_TYPE, s, e) {};
    };
    
    struct ScopeNode : public node {
        std::shared_ptr<ScopeNode> parentScope;
        std::vector<std::weak_ptr<ScopeNode>> childScopes;
        std::unordered_map<std::string, ParseRule> opRules;
        std::unordered_map<std::string, std::shared_ptr<VariableDeclarationNode>> variables;
        std::unordered_map<std::string, std::shared_ptr<StructDeclarationNode>> structs;
        std::unordered_map<std::string, std::shared_ptr<UnionDeclarationNode>> unions;
        std::unordered_multimap<std::string, std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> fields;
        std::unordered_map<std::string, std::shared_ptr<TypeNode>> tyCons;
        std::unordered_map<std::string, std::shared_ptr<TypedefNode>> typeAliases;
        std::unordered_map<std::string, std::shared_ptr<FunctionDeclarationNode>> functions;
        std::unordered_map<std::string, std::shared_ptr<ClassDeclarationNode>> classes;
        std::unordered_map<std::string, std::shared_ptr<node>> classImpls;
        std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<FunctionDeclarationNode>>> functionImpls;
        std::deque<std::pair<std::shared_ptr<Ty>, std::shared_ptr<Ty>>> constraints;
        std::unordered_map<std::shared_ptr<Ty>, std::shared_ptr<node>> nodeTVars;
        std::unordered_map<std::string, std::shared_ptr<ScopeNode>> namespaces;
        virtual bool hasError()
        {
            return false;
        }
        ScopeNode(std::shared_ptr<ScopeNode> parent = nullptr) 
            :parentScope(parent), node(NodeType::NODE_SCOPE) {}
    };
    
    struct ProgramNode : public node {
        std::vector<std::shared_ptr<node>> declarations;
        std::shared_ptr<ScopeNode> globalScope;
        bool hadError;
        virtual bool hasError()
        {
            return hadError;
        }
        ProgramNode() :hadError(false), globalScope(nullptr), node(NodeType::NODE_PROGRAM) {}
    };
    
    struct IfStatementNode : public node {
        std::shared_ptr<node> branchExpr;
        std::shared_ptr<node> thenStmt;
        std::shared_ptr<node> elseStmt;
        virtual bool hasError()
        {
            return branchExpr? branchExpr->hasError() : false ||
            thenStmt->hasError() ||
            elseStmt? elseStmt->hasError() : false;
        }
        IfStatementNode(std::shared_ptr<node> b, std::shared_ptr<node> t, std::shared_ptr<node> els, const char* s = nullptr, const char* e = nullptr)
            :branchExpr(b), thenStmt(t), elseStmt(els), node(NodeType::NODE_IF, s, e) {}
    };
    
    struct WhileStatementNode : public node {
        std::shared_ptr<node> loopExpr;
        std::shared_ptr<node> loopStmt;
        virtual bool hasError()
        {
            return loopExpr? loopExpr->hasError() : false ||
            loopStmt? loopStmt->hasError() : false;
        }
        WhileStatementNode(std::shared_ptr<node> le, std::shared_ptr<node> ls, const char* s = nullptr, const char* e = nullptr)
            :loopExpr(le), loopStmt(ls), node(NodeType::NODE_WHILE, s, e) {}
    };
    
    struct ForStatementNode : public node {
        std::shared_ptr<node> initExpr;
        std::shared_ptr<node> condExpr;
        std::shared_ptr<node> incrementExpr;
        std::shared_ptr<node> loopStmt;
        virtual bool hasError()
        {
            return initExpr? initExpr->hasError() : false ||
            condExpr? condExpr->hasError() : false ||
            incrementExpr? incrementExpr->hasError() : false ||
            loopStmt? loopStmt->hasError() : false;
        }
        ForStatementNode(std::shared_ptr<node> init, std::shared_ptr<node> cond, std::shared_ptr<node> incr, std::shared_ptr<node>loop, const char* s = nullptr, const char* e = nullptr)
            :initExpr(init), condExpr(cond), incrementExpr(incr), loopStmt(loop), node(NodeType::NODE_FOR, s, e) {}
    };
    
    struct CaseNode : public node {
        std::shared_ptr<node> caseExpr;
        std::shared_ptr<node> caseStmt;
        std::shared_ptr<ScopeNode> scope;
        virtual bool hasError()
        {
            return caseExpr? caseExpr->hasError() : false ||
            caseStmt? caseStmt->hasError() : false;
        }
        CaseNode(std::shared_ptr<node>ce, std::shared_ptr<node>cs, std::shared_ptr<ScopeNode> scope, const char* s = nullptr, const char* e = nullptr)
            :caseExpr(ce), caseStmt(cs), scope(scope), node(NodeType::NODE_CASE, s, e) {}
    };
    
    struct SwitchStatementNode : public node {
        std::shared_ptr<node> switchExpr;
        std::vector<std::shared_ptr<node>> cases;
        virtual bool hasError()
        {
            bool result = switchExpr? switchExpr->hasError() : false;
            for(auto _case : cases)
            {
                result |= _case->hasError();
            }
            return result;
        }
        SwitchStatementNode(std::shared_ptr<node>se, std::vector<std::shared_ptr<node>>cases, const char* s = nullptr, const char* e = nullptr)
            :switchExpr(se), cases(cases), node(NodeType::NODE_SWITCH, s, e) {}
    };
    
    struct ReturnStatementNode : public node { 
        std::shared_ptr<node> returnExpr;
        virtual bool hasError()
        {
            return returnExpr? returnExpr->hasError() : false;
        }
        ReturnStatementNode(std::shared_ptr<node> re, const char* s = nullptr, const char* e = nullptr)
            :returnExpr(re), node(NodeType::NODE_RETURN, s, e) {}
    };
    
    struct BlockStatementNode : public node {  
        std::shared_ptr<ScopeNode> scope;
        std::vector<std::shared_ptr<node>> declarations;
        virtual bool hasError()
        {
            bool result = false;
            for(auto dec : declarations)
            {
                result |= dec->hasError();
            }
            return result;
        }
        BlockStatementNode(std::vector<std::shared_ptr<node>> declarations, std::shared_ptr<ScopeNode> scope, const char* s = nullptr, const char* e = nullptr)
            :declarations(declarations), scope(scope), node(NodeType::NODE_BLOCK, s, e) {};
    };
    
    struct ArrayIndexNode : public node {  
        std::shared_ptr<node> array;
        std::shared_ptr<node> index;
        virtual bool hasError()
        {
            return array? array->hasError() : false || index? index->hasError() : false;
        }
        ArrayIndexNode(std::shared_ptr<node>a, std::shared_ptr<node>i, const char* s = nullptr, const char* e = nullptr)
            :array(a), index(i), node(NodeType::NODE_ARRAYINDEX, s, e) {}
    };
    
    struct FunctionCallNode : public node {
        std::shared_ptr<node> called;
        std::vector<std::shared_ptr<node>> args;
        virtual bool hasError()
        {
            bool result = false;
            result |= called? called->hasError() : false;
            for(auto arg : args)
            {
                result |= arg->hasError();
            }
            return result;
        }
        FunctionCallNode(std::shared_ptr<node>c, std::vector<std::shared_ptr<node>>a, const char* s = nullptr, const char* e = nullptr)
            :called(c), args(a), node(NodeType::NODE_FUNCTIONCALL, s, e) {}
    };
    
    struct ArrayConstructorNode : public node {
        std::vector<std::shared_ptr<node>> values;
        virtual bool hasError()
        {
            bool result = false;
            for(auto value : values)
            {
                result |= value->hasError();
            }
            return result;
        }
        ArrayConstructorNode(std::vector<std::shared_ptr<node>>v, const char* s = nullptr, const char* e = nullptr)
            :values(v), node(NodeType::NODE_ARRAYCONSTRUCTOR, s, e) {}
    };
    
    struct FieldCallNode : public node {
        std::shared_ptr<node> expr;
        Token field;
        virtual bool hasError()
        {
            return expr? expr->hasError() : false;
        }
        FieldCallNode(std::shared_ptr<node>ex, Token f, const char* s = nullptr, const char* e = nullptr)
            :expr(ex), field(f), node(NodeType::NODE_FIELDCALL, s, e) {}
    };
    
    struct LambdaNode : public node {
        std::shared_ptr<Ty> returnType;
        std::vector<Parameter> params;
        std::shared_ptr<node> body;
        virtual bool hasError()
        {
            return body? body->hasError() : false;
        }
        LambdaNode(std::shared_ptr<Ty> rt, std::vector<Parameter> p, std::shared_ptr<node> b, const char* s = nullptr, const char* e = nullptr)
            :returnType(rt), params(p), body(b), node(NodeType::NODE_LAMBDA, s, e) {}
    };
    
    struct AssignmentNode : public node {
        std::shared_ptr<node> variable;
        std::shared_ptr<node> assignment;
        virtual bool hasError()
        {
            return variable? variable->hasError() : false || assignment? assignment->hasError() : false;
        }
        AssignmentNode(std::shared_ptr<node>v, std::shared_ptr<node>a, const char* s = nullptr, const char* e = nullptr)
            :variable(v), assignment(a), node(NodeType::NODE_ASSIGNMENT, s, e) {}
    };
    
    struct BinaryNode : public node {
        std::shared_ptr<node> expression1;
        Token op;
        std::shared_ptr<node> expression2;
        virtual bool hasError()
        {
            return expression1? expression1->hasError() : false || expression2? expression2->hasError() : false;
        }
        BinaryNode(std::shared_ptr<node>e1, Token o, std::shared_ptr<node>e2, const char* s = nullptr, const char* e = nullptr)
            :expression1(e1), op(o), expression2(e2), node(NodeType::NODE_BINARY, s, e) {}
    };
    
    struct UnaryNode : public node {
        Token op;
        std::shared_ptr<node> expression;
        virtual bool hasError()
        {
            return expression? expression->hasError() : false;
        }
        UnaryNode(Token o, std::shared_ptr<node>ex, const char* s = nullptr, const char* e = nullptr)
            :op(o), expression(ex), node(NodeType::NODE_UNARY, s, e) {}
    };
    
    struct VariableNode : public node {
        Token variable;
        virtual bool hasError()
        {
            return false;
        }
        VariableNode(Token v, const char* s = nullptr, const char* e = nullptr)
            :variable(v), node(NodeType::NODE_IDENTIFIER, s, e) {}
    };
    
    struct LiteralNode : public node {
        Token value;
        virtual bool hasError()
        {
            return false;
        }
        LiteralNode(Token v, const char* s = nullptr, const char* e = nullptr)
            :value(v), node(NodeType::NODE_LITERAL, s, e) {}
    };
    
    struct TupleConstructorNode : public node {
        std::vector<std::shared_ptr<node>> values;
        virtual bool hasError()
        {
            bool result = false;
            for(auto value : values)
            {
                result |= value->hasError();
            }
            return result;
        }
        TupleConstructorNode(std::vector<std::shared_ptr<node>>v, const char* s = nullptr, const char* e = nullptr)
            :values(v), node(NodeType::NODE_TUPLE, s, e){}
    };
    
    struct ListInitNode : public node {
        std::shared_ptr<node> type;
        std::vector<Token> fieldNames;
        std::vector<std::shared_ptr<node>> values;
        virtual bool hasError()
        {
            bool result = false;
            for(auto value : values)
            {
                result |= value->hasError();
            }
            return result;
        }
        ListInitNode(std::shared_ptr<node>t, std::vector<Token>fn, std::vector<std::shared_ptr<node>>v, const char* s = nullptr, const char* e = nullptr)
            :type(t), fieldNames(fn), values(v), node(NodeType::NODE_LISTINIT, s, e) {}
    };
    
    struct ErrorNode : public node {
        virtual bool hasError()
        {
            return true;
        }
        ErrorNode(const char* s = nullptr, const char* e = nullptr)
            :node(NodeType::NODE_ERROR, s, e) {}
    };
    
    struct PlaceholderNode : public node {
        virtual bool hasError()
        {
            return false;
        }
        PlaceholderNode(const char* s = nullptr, const char* e = nullptr)
            :node(NodeType::NODE_PLACEHOLDER, s, e) {}
    }; 
    
    std::shared_ptr<ScopeNode> newScope(std::shared_ptr<ScopeNode> parent);
    
    void freeScope(std::shared_ptr<ScopeNode> node);
    
    void printNodes(std::shared_ptr<node> start, int depth);
    
    bool typesEqual(std::shared_ptr<Ty> a, std::shared_ptr<Ty> b);
    
    bool hasError(std::shared_ptr<node>);
    
    std::string typeToString(std::shared_ptr<Ty> t);
    
    std::shared_ptr<Ty> newGenericType();
    
    std::shared_ptr<ProgramNode> parse(const char* src);
    
    bool compareAST(std::shared_ptr<node> a, std::shared_ptr<node> b);

    std::string declarationName(std::shared_ptr<Ty> type);

    std::shared_ptr<Ty> generic(std::shared_ptr<Ty> type, std::unordered_map<std::string, std::shared_ptr<Ty>>& replaced);

    std::unordered_map<std::string, std::shared_ptr<Ty>> genericMap(std::shared_ptr<Ty> type, std::unordered_map<std::string, std::shared_ptr<Ty>> replaced);

    bool isRefutable(std::shared_ptr<node> n);
    
    std::shared_ptr<ScopeNode> getNamespaceScope(Token name, std::shared_ptr<ScopeNode> scope);
}
#endif