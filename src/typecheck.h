#include "parser.h"
#include <cassert>
namespace pilaf
{
    void error(size_t line, std::string_view printable, std::string_view highlighted, const char* msg);

    std::shared_ptr<Ty> typeInf(std::shared_ptr<node> n, std::shared_ptr<ScopeNode> currentScope);

    bool isFunctionType(std::shared_ptr<Ty> type);

    std::shared_ptr<Ty> returnTypeFromFunctionType(std::shared_ptr<Ty> type);

    std::shared_ptr<Ty> functionTypeFromFunction(std::shared_ptr<FunctionDeclarationNode> f);
    
    std::shared_ptr<Ty> firstParameterFromFunctionType(std::shared_ptr<Ty> type);
    
    bool hadError();
}