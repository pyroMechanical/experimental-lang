#ifndef parser_header
#define parser_header

#include "core.h"
#include "lexer.h"

typedef enum {
    NODE_UNDEFINED,
    NODE_PROGRAM,
    NODE_LIBRARY,
    NODE_IMPORT,
    NODE_DECLARATION,
    NODE_TYPEDECL,
    NODE_FUNCTIONDECL,
    NODE_VARIABLEDECL,
    NODE_CLASSDECL,
    NODE_FOR,
    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,
    NODE_WHILE,
    NODE_BLOCK,
    NODE_EXPRESSIONSTMT,
    NODE_ASSIGNMENT,
    NODE_BINARY,
    NODE_UNARY,
    NODE_FUNCTIONCALL,
    NODE_FIELDCALL,
    NODE_ARRAYINDEX,
    NODE_IDENTIFIER,
    NODE_CONSTANT,
    NODE_SCOPE,
} NodeType;

typedef struct {
    Token type;
    Token identifier;
} parameter;

typedef struct {
    NodeType nodeType;
} node;

typedef struct {
    node node;
    node** declarations;
    size_t declarationSize;
    node* globalScope;
    bool hadError;
} programNode;

typedef struct {
    node node;
    Token typeDefined;
    parameter* fields;
    size_t fieldSize;
} TypeDeclarationNode;

typedef struct {
    node node;
    Token type;
    Token identifier;
    parameter* parameters;
    size_t parameterSize;
    node* body;
} FunctionDeclarationNode;

typedef struct {
    node node;
    Token* typeQualifiers;
    size_t typeQualifierSize;
    Token type;
    Token identifier;
    node* value;
} VariableDeclarationNode;






bool parse(const char* src);

#endif