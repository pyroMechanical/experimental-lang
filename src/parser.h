#ifndef parser_header
#define parser_header

#include <memory>
#include <vector>
#include <unordered_map>
#include <deque>
#include "lexer.h"

struct Parser {
    Lexer* lexer;
    Token next;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
};

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
    PRECEDENCE_UNKNOWN,
    PRECEDENCE_PRIMARY
} Precedence;

struct node;
struct ScopeNode;

typedef struct
{
    std::shared_ptr<node>(*prefix)(Parser *parser, std::shared_ptr<ScopeNode> scope);
    std::shared_ptr<node>(*infix)(Parser *parser, std::shared_ptr<ScopeNode> scope, std::shared_ptr<node> n);
    uint8_t precedence;
    bool isPostfix;
} ParseRule;

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
    NODE_TUPLE,
    NODE_PLACEHOLDER,
    NODE_SCOPE,
};

struct node {
    NodeType nodeType;
    char* begin;
    char* end;
};

typedef std::string Ty;

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
    Ty kind;
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
    Ty kind;
    std::vector<Ty> constraints;
    std::vector<std::shared_ptr<node>> functions;
};

struct ClassImplementationNode : public node {
    Token _class;
    Ty implemented;
    std::vector<std::shared_ptr<node>> functions;
};

struct ScopeNode : public node {
    std::shared_ptr<ScopeNode> parentScope;
    std::vector<std::weak_ptr<ScopeNode>> childScopes;
    std::unordered_map<std::string, ParseRule> opRules;
    std::unordered_map<std::string, std::shared_ptr<VariableDeclarationNode>> variables;
    std::unordered_map<std::string, std::shared_ptr<RecordDeclarationNode>> types;
    std::unordered_multimap<std::string, std::pair<Ty, Ty>> fields;
    std::unordered_map<std::string, std::shared_ptr<TypedefNode>> typeAliases;
    std::unordered_map<std::string, std::shared_ptr<FunctionDeclarationNode>> functions;
    std::unordered_map<std::string, std::shared_ptr<ClassDeclarationNode>> classes;
    std::unordered_map<std::string, std::shared_ptr<node>> classImpls;
    std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<FunctionDeclarationNode>>> functionImpls;
    std::deque<std::pair<Ty, Ty>> constraints;
    std::deque<std::pair<Ty, std::vector<Ty>>> soft_constraints;
    std::deque<std::pair<Ty, Ty>> weak_constraints;
    std::unordered_map<Ty, std::shared_ptr<node>> nodeTVars;
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

struct TupleConstructorNode : public node {
    std::vector<std::shared_ptr<node>> values;
};

struct ListInitNode : public node {
    Ty type;
    std::vector<Token> fieldNames;
    std::vector<std::shared_ptr<node>> values;
};
struct PlaceholderNode : public node {}; //used for giving function parameters a definition

std::shared_ptr<ScopeNode> newScope(std::shared_ptr<ScopeNode> parent);

void freeScope(std::shared_ptr<ScopeNode> node);

void printNodes(std::shared_ptr<node> start, int depth);

bool typecmp(Ty a, Ty b);

Ty newGenericType();

std::shared_ptr<ProgramNode> parse(const char* src);

#endif