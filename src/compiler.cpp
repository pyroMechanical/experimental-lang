#include "compiler.h"
#include "semant.h"
namespace pilaf {
    bool compile(std::string src)
    {
        std::shared_ptr<ProgramNode> ast = analyze(src.c_str());
        if(ast == nullptr) return false;
        else return true;
    }
}