#include "name.h"
#include <memory>
#include "IR/ir.h"

std::unique_ptr<ExpressionIR> NameIR::makeMalloc() {
    return std::make_unique<ExpressionIR>(std::in_place_type<NameIR>, "__malloc");
}

std::unique_ptr<ExpressionIR> NameIR::makeException() {
    return std::make_unique<ExpressionIR>(std::in_place_type<NameIR>, "__exception");
}

std::unique_ptr<ExpressionIR> NameIR::makeExpr(std::string name, bool isGlobal) {
    return std::make_unique<ExpressionIR>(
        std::in_place_type<NameIR>,
        name,
        isGlobal
    );
}
