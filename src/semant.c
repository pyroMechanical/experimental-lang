#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
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

static bool typesEqual(Ty* type1, Ty* type2)
{
    if(type1->typeCapacity != type2->typeCapacity) return false;

    for(size_t i = 0; i < type1->typeCapacity; i++)
    {
        Token t1 = type1->type[i];
        Token t2 = type2->type[i];

        if(t1.length != t2.length || strncmp(t1.start, t2.start, t1.length) != 0)
        {
            return false;
        }
    }
    
    return true;
}

static Ty* resolveExpressionType(node* expr, ScopeNode* currentScope)
{
    switch(expr->nodeType)
    {
        case NODE_LITERAL:
        {
            Ty* type = malloc(sizeof(Ty));
            type->typeCapacity = 1;
            type->type = malloc(sizeof(Token));
            LiteralNode* n = (LiteralNode*)expr;
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
                    type->type[0] = t;
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
                    type->type[0] = t;
                    return type;
                }
                case FLOAT:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Float";
                    t.length = 5;
                    t.line = n->value.line;
                    type->type[0] = t;
                    return type;
                }
                case DOUBLE:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Double";
                    t.length = 6;
                    t.line = n->value.line;
                    type->type[0] = t;
                    return type;
                }
                case CHAR:
                {
                    Token t;
                    t.type = TYPE;
                    t.start = "Char";
                    t.length = 4;
                    t.line = n->value.line;
                    type->type[0] = t;
                    return type;
                }
                default:
                {
                    printf("unknown literal type: %d", n->value.type);
                    return NULL;
                }
            }
        }
        case NODE_IDENTIFIER:
        {
            VariableNode* v = (VariableNode*)expr;
            ScopeNode* s = currentScope;
            while(s != NULL)
            {
                char* t = tokenToString(v->variable);
                VariableDeclarationNode* n = searchTable(&s->variables, t);
                free(t);
                if(n != NULL)
                {
                    if(n->type != NULL)
                    {
                        return n->type;
                    }
                    else
                    {
                        error(&v->variable, "variable type not known!"); //possibly-redundant error
                        return NULL;
                    }
                }
                else if(s->parentScope != NULL)
                {
                    s = (ScopeNode*)s->parentScope;
                }
                else{
                    error(&v->variable, "variable not defined!");
                    return NULL;
                }
            }
            return NULL;
        }
        case NODE_ASSIGNMENT:
        {
            AssignmentNode* a = (AssignmentNode*)expr;
            if(a->variable != NULL)
            {
                Ty* varType = resolveExpressionType(a->variable, currentScope);
                if(varType != NULL)
                {
                    return varType;
                }
                else
                {
                    if(a->assignment != NULL)
                    {
                        Ty* aType = resolveExpressionType(a->assignment, currentScope);
                        return aType;
                    }
                }
            }
            else 
            {
                printf("and you may ask yourself \"how did I get here?\"");
                return NULL;
            }
            return NULL;
        }
        case NODE_UNARY:
        {
            //TODO: make this not hard-coded and dependent on standard library definitions
            UnaryNode* u = (UnaryNode*)expr;
            return resolveExpressionType(u->expression, currentScope);
            return NULL;
        }
        case NODE_BINARY:
        {
            //TODO: make this not hard-coded and dependent on standard library definitions
            BinaryNode* b = (BinaryNode*)expr;
            Ty* type1 = resolveExpressionType(b->expression1, currentScope);
            Ty* type2 = resolveExpressionType(b->expression2, currentScope);
            if(typesEqual(type1, type2))
            {
                return type1;
            }
            else
            {
                return NULL;
            }
        }
        case NODE_FIELDCALL:
        {
            FieldCallNode* fc = (FieldCallNode*)expr;
            Ty* type = resolveExpressionType(fc->expr, currentScope);
            if(type != NULL)
            {
                ScopeNode* s = currentScope;
                while(s != NULL)
                {   
                    char* t = tokenToString(type->type[0]);
                    RecordDeclarationNode* rd = searchTable(&s->types, t);
                    free(t);
                    if(rd != NULL)
                    {
                        for(size_t i = 0; i < rd->fieldCapacity; i++)
                        {
                            Token field = rd->fields[i].identifier;
                            if(tokencmp(field, fc->field))
                            {
                                if(type->typeCapacity == 1)
                                {
                                    return rd->fields[i].type;
                                }
                                else
                                {
                                    Ty* result = malloc(sizeof(Ty));
                                    result->typeCapacity = GROW_CAPACITY(0);
                                    result->type = GROW_ARRAY(Token, NULL, 0, result->typeCapacity);
                                    size_t typeCount = 0;
                                    for(size_t j = 0; j < rd->fields[i].type->typeCapacity; j++)
                                    {
                                        Token t = rd->fields[i].type->type[j];
                                        if(typeCount >= result->typeCapacity)
                                        {
                                            size_t newCapacity = GROW_CAPACITY(result->typeCapacity);
                                            result->type = GROW_ARRAY(Token, result->type, result->typeCapacity, newCapacity);
                                            result->typeCapacity = newCapacity;
                                        }
                                        if(t.type == IDENTIFIER)
                                        {
                                            for(size_t k = 0; k < rd->typeDefined->typeCapacity; k++)
                                            {
                                                if(tokencmp(t, rd->typeDefined->type[k]))
                                                {
                                                    result->type[typeCount] = type->type[k];
                                                    typeCount++;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            result->type[typeCount] = t;
                                            typeCount++;
                                        }
                                    }
                                    if(typeCount < result->typeCapacity)
                                    {
                                        result->type = GROW_ARRAY(Token, result->type, result->typeCapacity, typeCount);
                                        result->typeCapacity = typeCount;
                                    }
                                    char* t = typeToString(result);
                                    printf("%s", t);
                                    free(t);
                                    return result;
                                }
                            }
                        }
                    }
                    else
                    {
                        s = (ScopeNode*)s->parentScope;
                    }
                }
            }
            return NULL;
        }
        case NODE_FUNCTIONCALL:
        {
            FunctionCallNode* fc = (FunctionCallNode*)expr;
            switch(fc->called->nodeType)
            {
                case NODE_IDENTIFIER:
                {
                    VariableNode* fn = (VariableNode*)fc->called;
                    ScopeNode* s = currentScope;
                    while(s != NULL)
                    {
                        char* t = tokenToString(fn->variable);
                        FunctionDeclarationNode* fd = (FunctionDeclarationNode*)searchTable(&s->functions, t);
                        free(t);
                        if(fd != NULL)
                        {
                            return fd->returnType;
                        }
                        else
                        {
                            s = (ScopeNode*)s->parentScope;
                        }
                    }
                    error(&fn->variable, "Function definition not found!");
                    return NULL;
                }
                default:
                {
                    return NULL;
                }
            }
         }
        default:
        {
            printf("not an expression, or expression type not implemented!");
            return NULL;
        }
    }
}

static Ty* blockReturnType(BlockStatementNode* block)
{
    Ty* type = NULL;
    for(size_t i = 0; i < block->declarationCount; i++)
    {
        switch(block->declarations[i]->nodeType)
        {
            case NODE_RETURN:
            {
                ReturnStatementNode* ret = (ReturnStatementNode*)block->declarations[i];
                Ty* returnType = resolveExpressionType(ret->returnExpr, (ScopeNode*)block->scope);
                if(type == NULL)
                {
                    type = returnType;
                }
                if(returnType == NULL)
                {
                    //error() //unsure how to handle this ;~;
                }
                else if(!typecmp(type, returnType))
                {
                    error(returnType->type, "Type does not match previous return types!");
                }
            }
            default: continue;
        }
    }

    return type;
}

bool verifyTypes(node *n, ScopeNode *currentScope, Ty* expectedType)
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
            FunctionDeclarationNode* fd = (FunctionDeclarationNode*)n;
            if(fd->returnType == NULL)
            {
                if(fd->body == NULL)
                {
                    error(&fd->identifier, "cannot resolve type of undefined function!");
                    return true;
                }
                else
                {
                    fd->returnType = blockReturnType((BlockStatementNode*)fd->body);
                    if(fd->returnType == NULL)
                    {
                        error(&fd->identifier, "Unable to resolve return type!");
                    }
                }
            }
            else
            {
                return verifyTypes(fd->body, currentScope, fd->returnType);
            }
            return true;
        }
        case NODE_VARIABLEDECL:
        {
            VariableDeclarationNode* vd = (VariableDeclarationNode*)n;
            if(vd->type == NULL) 
            {
                if(vd->value == NULL)
                {
                    error(&vd->identifier, "cannot resolve type of undefined variable!");
                    return true;
                }
                else
                {
                    vd->type = resolveExpressionType(vd->value, currentScope);
                    if(vd->type == NULL) //if still null, error out
                    {
                        error(&vd->identifier, "cannot resolve type of declaration!");
                        return true;
                    }
                }
            }
            insertTable(&currentScope->variables, tokenToString(vd->identifier), vd);
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
            ReturnStatementNode* rn = (ReturnStatementNode*)n;
            if(typecmp(expectedType, resolveExpressionType(rn->returnExpr, currentScope)) == true)
            {
                return false;
            }
            else
            {
                Token t;
                t.type = RETURN;
                t.start = "return";
                t.length = 6;
                t.line = 0;
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
            BlockStatementNode* bn = (BlockStatementNode*)n;
            bool hadError = false;
            for(size_t i = 0; i < bn->declarationCount; i++)
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

static void printNodeFromTable(void *v)
{
    node *n = (node *)v;
    printNodes(n, 0);
}

ProgramNode *analyze(const char *src)
{
    ProgramNode *ast = parse(src);
    if(ast->hadError) return NULL;
    for (size_t i = 0; i < ast->declarationCapacity; i++)
    {
        ast->hadError |= verifyTypes(ast->declarations[i], ast->globalScope, NULL);
    }
    if(!ast->hadError)
    {
        applyTable(&ast->globalScope->variables, &printNodeFromTable);
        return ast;
    }
    return NULL;
}
