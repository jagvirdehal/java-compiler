#include "call.h"
#include "IR/ir.h"
#include <memory>
#include <utility>

std::unique_ptr<ExpressionIR> CallIR::makeMalloc(std::unique_ptr<ExpressionIR> arg) {
    std::vector<std::unique_ptr<ExpressionIR>> args(1); // one vector
    args.push_back(std::move(arg));
    return makeExpr(NameIR::makeMalloc(), std::move(args));
}

std::unique_ptr<ExpressionIR> CallIR::makeException() {
    std::vector<std::unique_ptr<ExpressionIR>> args(0); // empty vector
    return makeExpr(NameIR::makeException(), std::move(args));
}

std::unique_ptr<ExpressionIR> CallIR::makeExpr(std::unique_ptr<ExpressionIR> target, std::vector<std::unique_ptr<ExpressionIR> > args) {
    return std::make_unique<ExpressionIR>(
        std::in_place_type<CallIR>,
        std::move(target),
        std::move(args)
    );
}
