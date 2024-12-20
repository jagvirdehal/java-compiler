#include "ir_visitor.h"

// CompUnitIR
void IRSkipVisitor::visit_children(CompUnitIR &node) {
    for ( auto kv_pair : node.getFunctions() ) {
        assert(kv_pair.second);
        this->operator()(*kv_pair.second);
    }

    if (node.areStaticFieldsCanonicalized()) {
        for (auto& [name, initializer] : node.getCanonFieldList()) {
            assert(initializer);
            this->operator()(*initializer);
        }
    } else {
        for (auto& [name, initializer] : node.getFieldList()) {
            assert(initializer);
            this->operator()(*initializer);
        }
    }

    for (auto& start_stmt : node.start_statements) {
        assert(start_stmt);
        this->operator()(*start_stmt);
    }
}

// FuncDeclIR
void IRSkipVisitor::visit_children(FuncDeclIR &node) {
    this->operator()(node.getBody());
}

// ExpressionIRs
void IRSkipVisitor::visit_children(std::unique_ptr<ExpressionIR> &node) {
    assert(node);
    this->operator()(*node);
}
void IRSkipVisitor::visit_children(ExpressionIR &node) {
    std::visit([&](auto &inner_node) {
        this->operator()(inner_node);
    }, node);
}
void IRSkipVisitor::visit_children(BinOpIR &node) {
    this->operator()(node.getLeft());
    this->operator()(node.getRight());
}
void IRSkipVisitor::visit_children(CallIR &node) {
    this->operator()(node.getTarget());
    for ( auto &arg : node.getArgs()) {
        this->operator()(arg);
    }
}
void IRSkipVisitor::visit_children(ConstIR &node) {
    // No children
}
void IRSkipVisitor::visit_children(ESeqIR &node) {
    this->operator()(node.getStmt());
    this->operator()(node.getExpr());
}
void IRSkipVisitor::visit_children(MemIR &node) {
    this->operator()(node.getAddress());
}
void IRSkipVisitor::visit_children(NameIR &node) {
    // No children
}
void IRSkipVisitor::visit_children(TempIR &node) {
    // No children
}

// StatementIRs
void IRSkipVisitor::visit_children(std::unique_ptr<StatementIR> &node) {
    assert(node);
    this->operator()(*node);
}
void IRSkipVisitor::visit_children(StatementIR &node) {
    std::visit([&](auto &innernode) {
        this->operator()(innernode);
    }, node);
}
void IRSkipVisitor::visit_children(CJumpIR &node) {
    this->operator()(node.getCondition());
}
void IRSkipVisitor::visit_children(ExpIR &node) {
    this->operator()(node.getExpr());
}
void IRSkipVisitor::visit_children(JumpIR &node) {
    this->operator()(node.getTarget());
}
void IRSkipVisitor::visit_children(LabelIR &node) {
    // No children
}
void IRSkipVisitor::visit_children(MoveIR &node) {
    this->operator()(node.getTarget());
    this->operator()(node.getSource());
}
void IRSkipVisitor::visit_children(ReturnIR &node) {
    if ( node.getRet() ) {
        this->operator()(*node.getRet());
    }
}
void IRSkipVisitor::visit_children(SeqIR &node) {
    for ( auto &stmt : node.getStmts() ) {
        this->operator()(stmt);
    }
}
void IRSkipVisitor::visit_children(CommentIR &node) {
    // No children
}
