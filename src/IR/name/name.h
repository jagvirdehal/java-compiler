#pragma once

#include "IR/ir_variant.h"
#include <memory>
#include <string>

class NameIR {
    std::string name; 

public:
    bool isGlobal = false;
    NameIR(std::string name, bool isGlobal=false) : name{std::move(name)}, isGlobal{isGlobal} {}
    std::string &getName() { return name; }
    std::string label() { return "NAME(" + name + ")"; }
    bool isConstant() { return false; }

    static std::unique_ptr<ExpressionIR> makeMalloc();
    static std::unique_ptr<ExpressionIR> makeException();

    static std::unique_ptr<ExpressionIR> makeExpr(std::string name, bool isGlobal=false);
};
