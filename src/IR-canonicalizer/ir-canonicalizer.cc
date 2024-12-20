#include "ir-canonicalizer.h"
#include "utillities/util.h"
#include "exceptions/exceptions.h"
#include "concatenate.h"
#include "IR/code-gen-constants.h"

IRCanonicalizer::LoweredStatement::LoweredStatement(std::vector<StatementIR> statements)
    : statements{std::move(statements)} {}

IRCanonicalizer::LoweredExpression::LoweredExpression(
        std::vector<StatementIR> statements, std::unique_ptr<ExpressionIR> expression
    ) : statements{std::move(statements)}, expression{std::move(expression)} {}

IRCanonicalizer::LoweredExpression::LoweredExpression(
        std::vector<StatementIR> statements, ExpressionIR expression
    ) : statements{std::move(statements)}, expression{std::make_unique<ExpressionIR>(std::move(expression))} {}

void IRCanonicalizer::operator()(IR &ir) { convert(ir); }

IRCanonicalizer::LoweredExpression IRCanonicalizer::operator()(ExpressionIR &ir) { return convert(ir); }
IRCanonicalizer::LoweredStatement IRCanonicalizer::operator()(StatementIR &ir) { return convert(ir); }

void IRCanonicalizer::convert(IR &ir) {
    std::visit(util::overload {
        [&](CompUnitIR &node) {
            for (auto& func : node.getFunctionList()) {
                func->getBody() = SeqIR(convert(func->getBody()).statements);
            }

            for (auto& start_stmt : node.start_statements) {
                *start_stmt = SeqIR(convert(*start_stmt).statements);
            }

            std::vector<std::pair<std::string, std::unique_ptr<StatementIR>>> child_canonical_static_fields;
            for (auto& [static_field_name, initializer] : node.getFieldList()) {
                LoweredExpression lowered = convert(*initializer);

                std::vector<StatementIR> initializer_statements = concatenate(
                    lowered.statements,
                    MoveIR(std::make_unique<ExpressionIR>(TempIR(static_field_name, true)), std::move(lowered.expression))
                );

                child_canonical_static_fields.emplace_back(
                    static_field_name,
                    SeqIR::makeStmt(std::move(initializer_statements))
                );
            }
            node.canonicalizeStaticFields(child_canonical_static_fields);
        },
        [&](auto &node) { THROW_CompilerError("This should not happen"); }
    }, ir);
}

IRCanonicalizer::LoweredExpression IRCanonicalizer::convert(ExpressionIR &ir) {
    return std::visit(util::overload {
        [&](BinOpIR &node) {
            // Unoptimized form: assumes right's side effects can affect left
            LoweredExpression lowered1 = convert(node.getLeft());
            LoweredExpression lowered2 = convert(node.getRight());
            
            std::string temp_name = TempIR::generateName();

            auto statements = concatenate(
                lowered1.statements,
                MoveIR(std::make_unique<ExpressionIR>(TempIR(temp_name)), std::move(lowered1.expression)),
                lowered2.statements
            );

            auto expression = BinOpIR(node.op, std::make_unique<ExpressionIR>(TempIR(temp_name)), std::move(lowered2.expression));

            return LoweredExpression(std::move(statements), std::move(expression));
        },

        [&](CallIR &node) {
            LoweredExpression result;

            std::vector<std::unique_ptr<ExpressionIR>> arg_temporaries;

            for (auto& arg : node.getArgs()) {
                auto temporary = TempIR(TempIR::generateName(ARG_TEMPORARY_PREFIX));
                arg_temporaries.push_back(std::make_unique<ExpressionIR>(temporary));

                auto lowered = convert(*arg);

                result.statements = concatenate(
                    result.statements, 
                    lowered.statements, 
                    MoveIR(std::make_unique<ExpressionIR>(temporary), std::move(lowered.expression))
                );
            }

            if (std::get_if<NameIR>(&node.getTarget())) {
                // Call target is a label
                auto target = std::make_unique<ExpressionIR>(std::move(node.getTarget()));
                result.statements.emplace_back(CallIR(std::move(target), std::move(arg_temporaries)));
            } else {
                // Call target is an arbitrary expression
                auto lowered_target = convert(node.getTarget());
                result.statements = concatenate(
                    result.statements,
                    lowered_target.statements,
                    CallIR(std::move(lowered_target.expression), std::move(arg_temporaries))
                );
            }
            
            result.expression = std::make_unique<ExpressionIR>(TempIR(CGConstants::ABSTRACT_RET));

            return result;
        },

        [&](ConstIR &node) { 
            // Leave alone
            return LoweredExpression(std::move(node)); 
        },

        [&](ESeqIR &node) {
            // Concatenate the lowered statements followed by the lowered statements of the expression, then the lowered expression
            LoweredStatement lowered_statement = convert(node.getStmt());
            LoweredExpression lowered_expression = convert(node.getExpr());

            auto statements = concatenate(
                lowered_statement.statements,
                lowered_expression.statements
            );

            auto expression = std::move(lowered_expression.expression);

            return LoweredExpression(std::move(statements), std::move(expression));
        },

        [&](MemIR &node) {
            // Just concatenate the extracted statments in front and put the pure expr in the MemIR
            LoweredExpression lowered_address = convert(node.getAddress());

            return LoweredExpression(
                std::move(lowered_address.statements),
                MemIR::makeExpr(std::move(lowered_address.expression))
            ); 
        },

        [&](NameIR &node) { 
            // Leave alone
            return LoweredExpression(std::move(node)); 
        },

        [&](TempIR &node) { 
            // Leave alone
            return LoweredExpression(std::move(node)); 
        }
    }, ir);
}

IRCanonicalizer::LoweredStatement IRCanonicalizer::convert(StatementIR &ir) {
    return std::visit(util::overload {
        [&](CJumpIR &node) {
            LoweredExpression lowered_target = convert(node.getCondition());

            return LoweredStatement(concatenate(
                lowered_target.statements,
                CJumpIR(std::move(lowered_target.expression), node.trueLabel(), node.falseLabel())
            ));
        },

        [&](ExpIR &node) {
            // Throw away the pure expression, keep the extracted side effect statements
            LoweredExpression lowered_expression = convert(node.getExpr());
            return LoweredStatement(std::move(lowered_expression.statements));
        },

        [&](JumpIR &node) {
            LoweredExpression lowered_target = convert(node.getTarget());

            return LoweredStatement(concatenate(
                lowered_target.statements,
                JumpIR(std::move(lowered_target.expression))
            ));
        },

        [&](LabelIR &node) {
            // Leave alone 
            return LoweredStatement(std::move(node)); 
        },

        [&](MoveIR &node) {
            if (std::get_if<TempIR>(&node.getTarget())) {
                LoweredExpression lowered2 = convert(node.getSource());

                auto statements = concatenate(
                    lowered2.statements,
                    MoveIR(std::make_unique<ExpressionIR>(std::move(node.getTarget())), std::move(lowered2.expression))
                );

                return LoweredStatement(std::move(statements));
            } else if (MemIR* mem_target = std::get_if<MemIR>(&node.getTarget())) { // MOVE mem[i], k
                // Unoptimized form: assumes source's side effects can affect target
                LoweredExpression lowered1 = convert(mem_target->getAddress());

                // LoweredExpression lowered1 = convert(node.getTarget());
                LoweredExpression lowered2 = convert(node.getSource());
            
                std::string temp_name = TempIR::generateName();

                auto mem = MemIR(std::make_unique<ExpressionIR>(TempIR(temp_name)));

                auto statements = concatenate(
                    lowered1.statements,
                    MoveIR(std::make_unique<ExpressionIR>(TempIR(temp_name)), std::move(lowered1.expression)), // T = i
                    lowered2.statements,
                    MoveIR(std::make_unique<ExpressionIR>(std::move(mem)), std::move(lowered2.expression)) // mem[i] = k
                );

                return LoweredStatement(std::move(statements));
            } else {
                THROW_CompilerError("MoveIR target is not a MemIR or TempIR node");
            }
        },

        [&](ReturnIR &node) {
            // Empty return left alone
            if (!node.getRet()) {
                return LoweredStatement(ReturnIR(nullptr));
            }

            // Put statements in front of return expression after lowering
            LoweredExpression lowered_expression = convert(*node.getRet());
            return LoweredStatement(concatenate(
                lowered_expression.statements,
                ReturnIR(std::move(lowered_expression.expression))
            ));
        },

        [&](SeqIR &node) {
            // Lower all contained statements and concatenate them
            LoweredStatement result;
            for (auto& stmt : node.getStmts()) {
                result.statements = concatenate(result.statements, convert(*stmt).statements);
            }
            return result;
        },

        [&](CallIR &node) -> LoweredStatement {
            THROW_CompilerError("Call IR should not be considered a statement before conversion"); 
        },

        [&](CommentIR &node) -> LoweredStatement {
            // Leave alone
            return LoweredStatement(std::move(node));
        }
    }, ir);
}
