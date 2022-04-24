#ifndef parser_header
#define parser_header

#include "core.h"
#include "lexer.h"
#include "hash_table.h"

typedef struct {
    Lexer* lexer;
    Token next;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
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
    NODE_LITERAL,
    NODE_SCOPE,
} NodeType;

typedef struct {
    NodeType nodeType;
} node;

typedef enum {
    PLAINTYPE,
    FUNCTIONTYPE,
    GENERICTYPE
} Kind;

typedef struct {
    size_t typeCapacity;
    Token* type;
} Ty;

typedef struct {
    Ty* type;
    Token identifier;
} Parameter;

typedef struct {
    node n;
    node* parentScope;
    HashTable variables;
    HashTable types;
    HashTable typeAliases;
    HashTable functions;
    HashTable classes;
    HashTable classImpls;
} ScopeNode;

typedef struct {
    node n;
    node** declarations;
    size_t declarationCapacity;
    ScopeNode* globalScope;
    bool hadError;
} ProgramNode;

typedef struct {
    node n;
    Ty* typeDefined;
    Ty* typeAliased;
} TypedefNode;

typedef struct {
    node n;
    Ty* typeDefined;
    size_t fieldCapacity;
    Parameter* fields;
    enum { UNDEFINED, IS_STRUCT, IS_UNION } struct_or_union;
} RecordDeclarationNode;

typedef struct {
    node n;
    Ty* returnType;
    Token identifier;
    size_t paramCapacity;
    size_t paramCount;
    Parameter* params;
    node* body;
} FunctionDeclarationNode;

typedef struct {
    node n;
    Ty* type;
    Token identifier;
    node* value;
} VariableDeclarationNode;

typedef struct {
    node n;
    Ty* className;
    size_t constraintCapacity;
    Ty** constraints;
    size_t functionCapacity;
    node** functions;
} ClassDeclarationNode;

typedef struct  {
    node n;
    node* branchExpr;
    node* thenStmt;
    node* elseStmt;
} IfStatementNode;

typedef struct {
    node n;
    node* loopExpr;
    node* loopStmt;
} WhileStatementNode;

typedef struct {
    node n;
    node* initExpr;
    node* condExpr;
    node* incrementExpr;
    node* loopStmt;
} ForStatementNode;

typedef struct {
    node n;
    node* caseExpr;
    node* caseStmt;
} CaseNode;

typedef struct {
    node n;
    node* switchExpr;
    size_t caseCount;
    size_t caseCapacity;
    CaseNode** cases;
} SwitchStatementNode;

typedef struct {
    node n;
    node* returnExpr;
} ReturnStatementNode;

typedef struct {
    node n;
    ScopeNode* scope;
    size_t declarationCount;
    size_t declarationCapacity;
    node** declarations;
} BlockStatementNode;

typedef struct {
    node n;
    node* array;
    node* index;
} ArrayIndexNode;

typedef struct {
    node n;
    node* called;
    size_t argCount;
    size_t argCapacity;
    node** args;
} FunctionCallNode;

typedef struct {
    node n;
    size_t count;
    size_t capacity;
    node** values;
} ArrayConstructorNode;

typedef struct { 
    node n;
    node* expr;
    Token op;
    Token field;
} FieldCallNode;

typedef struct {
    node n;
    node* variable;
    node* assignment;
} AssignmentNode;

typedef struct {
    node n;
    node* expression1;
    Token op;
    node* expression2;
} BinaryNode;

typedef struct {
    node n;
    Token op;
    node* expression;
} UnaryNode;

typedef struct {
    node n;
    Token variable;
} VariableNode;

typedef struct {
    node n;
    Token value;
} LiteralNode;

ScopeNode* newScope(ScopeNode* parent);

void freeScope(ScopeNode* node);

void printNodes(node* start, int depth);

char* typeToString(Ty* type);

bool typecmp(Ty* a, Ty* b);

ProgramNode* parse(const char* src);

#endif