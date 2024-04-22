#include "call.h"
#include "IR/ir.h"
#include <memory>
#include <utility>

std::unique_ptr<ExpressionIR> CallIR::makeMalloc(std::unique_ptr<ExpressionIR> arg) {
    std::vector<std::unique_ptr<ExpressionIR>> args; // one vector
    args.push_back(std::move(arg));
    return makeExpr(NameIR::makeMalloc(), ConstIR::makeZero(), std::move(args));
}

std::unique_ptr<ExpressionIR> CallIR::makeException() {
    return makeExpr(NameIR::makeException(), ConstIR::makeZero(), {});
}

std::unique_ptr<ExpressionIR> CallIR::makeExpr(
    std::unique_ptr<ExpressionIR> target,
    std::unique_ptr<ExpressionIR> _this,
    std::vector<std::unique_ptr<ExpressionIR>> args
) {
    std::vector<std::unique_ptr<ExpressionIR>> passed_args = {std::move(_this)};
    for ( auto &arg : args ) {
        passed_args.push_back(std::move(arg));
    }

    return std::make_unique<ExpressionIR>(
        std::in_place_type<CallIR>,
        std::move(target),
        std::move(passed_args)
    );
}
