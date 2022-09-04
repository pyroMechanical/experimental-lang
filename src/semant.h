#ifndef semant_header
#define semant_header
#include "typecheck.h"

namespace pilaf {
    std::shared_ptr<ProgramNode> analyze(const char* src);
}
#endif