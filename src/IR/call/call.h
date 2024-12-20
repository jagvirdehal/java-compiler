#pragma once

#include <memory>
#include "IR/ir_variant.h"
#include <cassert>
#include <vector>
#include <string>

class CallIR {
protected:
    std::unique_ptr<ExpressionIR> target;
    std::vector<std::unique_ptr<ExpressionIR>> args;
public:
    CallIR(std::unique_ptr<ExpressionIR> target, std::vector<std::unique_ptr<ExpressionIR> > args)
        : target{std::move(target)}, args{std::move(args)} {}
    ExpressionIR &getTarget() { assert(target.get()); return *target.get(); };
    std::vector<std::unique_ptr<ExpressionIR>> &getArgs() { return args; };
    int getNumArgs() { return args.size(); };
    std::string label() { return "CALL"; }
    bool isConstant() { return false; };

    static std::unique_ptr<ExpressionIR> makeMalloc(std::unique_ptr<ExpressionIR> arg);
    static std::unique_ptr<ExpressionIR> makeException();
    static std::unique_ptr<ExpressionIR> makeExpr(
        std::unique_ptr<ExpressionIR> target,
        std::unique_ptr<ExpressionIR> _this,
        std::vector<std::unique_ptr<ExpressionIR> > args
    );
};
