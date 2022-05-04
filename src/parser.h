#ifndef parser_header
#define parser_header

#include <memory>
#include <vector>
#include <unordered_map>
#include "lexer.h"

struct Parser {
    Lexer* lexer;
    Token next;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
};

enum NodeType {
    NODE_UNDEFINED,
    NODE_PROGRAM,
    NODE_LIBRARY,
    NODE_IMPORT,
    NODE_TYPE,
    NODE_DECLARATION,
    NODE_TYPEDEF,
    NODE_RECORDDECL,
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
    NODE_PLACEHOLDER,
    NODE_SCOPE,
};

struct node {
    NodeType nodeType;
};

enum Kind {
    PLAINTYPE,
    FUNCTIONTYPE,
    GENERICTYPE
};

typedef std::vector<Token> Ty;

struct Parameter {
  
    Ty type;
    Token identifier;
};

struct TypedefNode : public node {
    Ty typeDefined;
    Ty typeAliased;
};

struct RecordDeclarationNode : public node {
    Ty  typeDefined;
    std::vector<Parameter> fields;
    enum { UNDEFINED, IS_STRUCT, IS_UNION } struct_or_union;
};

struct FunctionDeclarationNode : public node {
    Ty returnType;
    Token identifier;
    std::vector<Parameter> params;
    std::shared_ptr<node> body;
};

struct VariableDeclarationNode : public node {
    Ty type;
    Token identifier;
    std::shared_ptr<node> value;
};

struct ClassDeclarationNode : public node {
    Ty className;
    std::vector<Ty> constraints;
    std::vector<std::shared_ptr<node>> functions;
};

struct ClassImplementationNode : public node {
    Token _class;
    Ty implemented;
    std::vector<std::shared_ptr<node>> functions;
};

struct ScopeNode;

struct ScopeNode : public node {
    std::shared_ptr<ScopeNode> parentScope;
    std::unordered_map<std::string, std::shared_ptr<VariableDeclarationNode>> variables;
    std::unordered_map<std::string, std::shared_ptr<RecordDeclarationNode>> types;
    std::unordered_map<std::string, std::shared_ptr<TypedefNode>> typeAliases;
    std::unordered_map<std::string, std::shared_ptr<FunctionDeclarationNode>> functions;
    std::unordered_map<std::string, std::shared_ptr<ClassDeclarationNode>> classes;
    std::unordered_map<std::string, std::shared_ptr<node>> classImpls; //TODO: add class implementation node here
};

struct ProgramNode : public node {
    std::vector<std::shared_ptr<node>> declarations;
    std::shared_ptr<ScopeNode> globalScope;
    bool hadError;
};



struct IfStatementNode : public node {
    std::shared_ptr<node> branchExpr;
    std::shared_ptr<node> thenStmt;
    std::shared_ptr<node> elseStmt;
};

struct WhileStatementNode : public node {
    std::shared_ptr<node> loopExpr;
    std::shared_ptr<node> loopStmt;
};

struct ForStatementNode : public node {
    std::shared_ptr<node> initExpr;
    std::shared_ptr<node> condExpr;
    std::shared_ptr<node> incrementExpr;
    std::shared_ptr<node> loopStmt;
};

struct CaseNode : public node {
    std::shared_ptr<node> caseExpr;
    std::shared_ptr<node> caseStmt;
};

struct SwitchStatementNode : public node {
    std::shared_ptr<node> switchExpr;
    std::vector<std::shared_ptr<CaseNode>> cases;
};

struct ReturnStatementNode : public node { 
    std::shared_ptr<node> returnExpr;
};

struct BlockStatementNode : public node {  
    std::shared_ptr<ScopeNode> scope;
    std::vector<std::shared_ptr<node>> declarations;
};

struct ArrayIndexNode : public node {  
    std::shared_ptr<node> array;
    std::shared_ptr<node> index;
};

struct FunctionCallNode : public node {
    std::shared_ptr<node> called;
    std::vector<std::shared_ptr<node>> args;
};

struct ArrayConstructorNode : public node {
    std::vector<std::shared_ptr<node>> values;
};

struct FieldCallNode : public node { 
    std::shared_ptr<node> expr;
    Token op;
    Token field;
};

struct LambdaNode : public node {
    Ty returnType;
    std::vector<Parameter> params;
    std::shared_ptr<node> body;
};

struct AssignmentNode : public node {
    std::shared_ptr<node> variable;
    std::shared_ptr<node> assignment;
};

struct BinaryNode : public node {
    std::shared_ptr<node> expression1;
    Token op;
    std::shared_ptr<node> expression2;
};

struct UnaryNode : public node {
    Token op;
    std::shared_ptr<node> expression;
};

struct VariableNode : public node {
    Token variable;
};

struct LiteralNode : public node {
    Token value;
};

struct ListInitNode : public node {
    Ty type;
    std::vector<std::shared_ptr<node>> values;
};
struct PlaceholderNode : public node {}; //used for giving function parameters a definition

std::shared_ptr<ScopeNode> newScope(std::shared_ptr<ScopeNode> parent);

void freeScope(std::shared_ptr<ScopeNode> node);

void printNodes(std::shared_ptr<node> start, int depth);

std::string typeToString(Ty type);

bool typecmp(Ty a, Ty b);

std::shared_ptr<ProgramNode> parse(const char* src);

#endif