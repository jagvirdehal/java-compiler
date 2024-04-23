#include "ir-tiling.h"
#include "utillities/util.h"
#include "exceptions/exceptions.h"
#include "IR-tiling/register-allocation/register-allocator.h"
#include "IR/code-gen-constants.h"

#include "IR-tiling/assembly/assembly-instruction.h"
#include "IR-tiling/assembly/registers.h"

#include <regex>

using namespace AssemblyRefactor;

size_t IRToTilesConverter::abstract_reg_count = 0;

void IRToTilesConverter::decideIsCandidate(ExpressionIR& ir, Tile candidate) {
    Tile& optimal_tile = expression_memo[&ir];
    if (candidate.getCost() < optimal_tile.getCost()) {
        optimal_tile = candidate;
    }
};

void IRToTilesConverter::decideIsCandidate(StatementIR& ir, Tile candidate) {
    Tile& optimal_tile = statement_memo[&ir];
    if (candidate.getCost() < optimal_tile.getCost()) {
        optimal_tile = candidate;
    }
};

ExpressionTile IRToTilesConverter::tile(const std::string &abstract_reg, ExpressionIR &ir) {
    if (expression_memo.count(&ir)) {
        return expression_memo[&ir].pairWith(abstract_reg);
    }

    // Generic tile that must be applicable, if no specialized tiles are applicable
    Tile generic_tile;

    std::visit(util::overload {
        [&](BinOpIR &node) {
            
            // Generic tile that is always applicable : store operands in abstract registers, then do something
            std::string operand1_reg = newAbstractRegister();
            std::string operand2_reg = newAbstractRegister();

            generic_tile = Tile({
                tile(operand1_reg, node.getLeft()),
                tile(operand2_reg, node.getRight())
            });

            switch (node.op) {
                case BinOpIR::OpType::ADD: {
                    generic_tile.add_instructions_after({
                        Lea(Tile::ABSTRACT_REG, EffectiveAddress(operand1_reg, operand2_reg))
                    });
                    break;
                } 
                case BinOpIR::OpType::SUB: {
                    generic_tile.add_instructions_after({
                        Sub(operand1_reg, operand2_reg),
                        Mov(Tile::ABSTRACT_REG, operand1_reg)
                    });
                    break;
                }
                case BinOpIR::OpType::MUL: {
                    generic_tile.add_instructions_after({
                        Mov(REG32_ACCUM, operand1_reg),
                        IMul(operand2_reg),
                        Mov(Tile::ABSTRACT_REG, REG32_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::DIV: {
                    generic_tile.add_instructions_after({
                        Cmp(operand2_reg, 0), // Check for division by zero
                        Je(LabelUse("__exception")), // Jump to exception handler if division by zero
                        Mov(REG32_ACCUM, operand1_reg), // Move low 32 bits of dividend into accumulator
                        Cdq(), // Sign extend REG32_ACCUM into REG32_DATA
                        IDiv(operand2_reg),
                        Mov(Tile::ABSTRACT_REG, REG32_ACCUM) // Quotient stored in ACCUM
                    });
                    break;
                }
                case BinOpIR::OpType::MOD: {
                    generic_tile.add_instructions_after({
                        Cmp(operand2_reg, 0), // Check for division by zero
                        Je(LabelUse("__exception")), // Jump to exception handler if division by zero
                        Mov(REG32_ACCUM, operand1_reg), // Move low 32 bits of dividend into accumulator
                        Cdq(), // Sign extend REG32_ACCUM into REG32_DATA
                        IDiv(operand2_reg),
                        Mov(Tile::ABSTRACT_REG, REG32_DATA) // Remainder stored in DATA
                    });
                    break;
                }
                case BinOpIR::OpType::AND: {
                    generic_tile.add_instructions_after({
                        And(operand1_reg, operand2_reg),
                        Mov(Tile::ABSTRACT_REG, operand1_reg)
                    });
                    break;
                }
                case BinOpIR::OpType::OR: {
                    generic_tile.add_instructions_after({
                        Or(operand1_reg, operand2_reg),
                        Mov(Tile::ABSTRACT_REG, operand1_reg)
                    });
                    break;
                }
                case BinOpIR::OpType::EQ: {
                    generic_tile.add_instructions_after({
                        Cmp(operand1_reg, operand2_reg),
                        SetZ(REG8L_ACCUM),
                        MovZX(Tile::ABSTRACT_REG, REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::NEQ: {
                    generic_tile.add_instructions_after({
                        Cmp(operand1_reg, operand2_reg),
                        SetNZ(REG8L_ACCUM),
                        MovZX(Tile::ABSTRACT_REG, REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::LT: {
                    generic_tile.add_instructions_after({
                        Cmp(operand1_reg, operand2_reg),
                        SetL(REG8L_ACCUM),
                        MovZX(Tile::ABSTRACT_REG, REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::GT: {
                    generic_tile.add_instructions_after({
                        Cmp(operand1_reg, operand2_reg),
                        SetG(REG8L_ACCUM),
                        MovZX(Tile::ABSTRACT_REG, REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::LEQ: {
                    generic_tile.add_instructions_after({
                        Cmp(operand1_reg, operand2_reg),
                        SetLE(REG8L_ACCUM),
                        MovZX(Tile::ABSTRACT_REG, REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::GEQ: {
                    generic_tile.add_instructions_after({
                        Cmp(operand1_reg, operand2_reg),
                        SetGE(REG8L_ACCUM),
                        MovZX(Tile::ABSTRACT_REG, REG8L_ACCUM)
                    });
                    break;
                }
                default: {
                    THROW_CompilerError("Operation is not supported in Joosc");
                }
            }
        },

        [&](ConstIR &node) {
            // 32-bit immediate
            generic_tile = Tile({
                Mov(Tile::ABSTRACT_REG, node.getValue())
            });
        },

        [&](MemIR &node) {
            std::string address_reg = newAbstractRegister();

            generic_tile = Tile({
                tile(address_reg, node.getAddress()),
                Mov(Tile::ABSTRACT_REG, EffectiveAddress(address_reg))
            });
        },

        [&](NameIR &node) {
            generic_tile = Tile({
                Mov(Tile::ABSTRACT_REG, LabelUse(node.getName()))
            });
        },

        [&](TempIR &node) {
            // Special registers : ABSTRACT_RET is REG32_ACCUM
            if (node.getName() == CGConstants::ABSTRACT_RET) {
                generic_tile = Tile({
                    Mov(Tile::ABSTRACT_REG, REG32_ACCUM)
                });
            }
            // Special registers : ABSTRACT_ARG_PREFIX# is stack offset from caller
            else if (node.getName().rfind(CGConstants::ABSTRACT_ARG_PREFIX, 0) == 0) {
                int arg_num 
                    = std::stoi(std::regex_replace(node.getName(), std::regex(CGConstants::ABSTRACT_ARG_PREFIX), ""));

                generic_tile = Tile({
                    Mov(
                        Tile::ABSTRACT_REG, 
                        EffectiveAddress(REG32_STACKBASEPTR, 4 * (arg_num + 2))
                    )
                });
            } 
            // Special temp : Static variable in .data section
            else if (node.isGlobal) {
                generic_tile = Tile({
                    Mov(
                        Tile::ABSTRACT_REG,
                        EffectiveAddress(node.getName())
                    )
                });
            }
            // Not special
            else {
                generic_tile = Tile({
                    Mov(Tile::ABSTRACT_REG, escapeTemporary(node.getName()))
                });
            }
        },

        [&](ESeqIR &node) {
            THROW_CompilerError("ESeqIR should not exist after canonicalization"); 
        },

        [&](CallIR &node) {
            THROW_CompilerError("CallIR should not be considered an expression after canonicalization"); 
        }
    }, ir);

    if (generic_tile.getCost() == Tile().getCost()) {
        THROW_CompilerError("Generic tile was not assigned to " + std::visit([&](auto &x){ return x.label(); }, ir));
    }
    decideIsCandidate(ir, generic_tile);

    return expression_memo[&ir].pairWith(abstract_reg);
}

StatementTile IRToTilesConverter::tile(StatementIR &ir) {
    if (statement_memo.count(&ir)) {
        return &statement_memo[&ir];
    }

    // Generic tile that must be applicable, if no specialized tiles are applicable
    Tile generic_tile;
    
    std::visit(util::overload {
        [&](CJumpIR &node) {
            std::string cond_reg = newAbstractRegister();

            generic_tile = Tile({
                tile(cond_reg, node.getCondition()),
                Test(cond_reg, cond_reg),
                JumpIfNZ(LabelUse(node.trueLabel()))
            });
        },

        [&](JumpIR &node) {
            if (auto target = std::get_if<NameIR>(&node.getTarget())) {
                generic_tile = Tile({
                    Jump(LabelUse(target->getName()))
                });
            } else {
                std::string target_reg = newAbstractRegister();

                generic_tile = Tile({
                    tile(target_reg, node.getTarget()),
                    Jump(target_reg)
                });
            }
        },

        [&](LabelIR &node) {
            generic_tile = Tile({Label(node.getName())});
        },

        [&](MoveIR &node) {
            // Behaviour depends on target type
            std::visit(util::overload {

                [&](TempIR& target) {
                    if (target.isGlobal) {
                        // Update global variable
                        std::string temp_value_reg = newAbstractRegister();

                        generic_tile = Tile({
                            tile(temp_value_reg, node.getSource()),
                            Mov(EffectiveAddress(target.getName()), temp_value_reg)
                        });
                    } else {
                        generic_tile = Tile({
                            tile(escapeTemporary(target.getName()), node.getSource())
                        });
                    }
                },

                [&](MemIR& target) {
                    std::string address_reg = newAbstractRegister();
                    std::string source_reg = newAbstractRegister();

                    generic_tile = Tile({
                        tile(source_reg, node.getSource()),
                        tile(address_reg, target.getAddress()),
                        Mov(EffectiveAddress(address_reg), source_reg)
                    });
                },

                [&](auto&) { THROW_CompilerError("Invalid MoveIR target"); }

            }, node.getTarget());
        },

        [&](ReturnIR &node) {
            if (node.getRet()) {
                // Return a value by placing it in REG32_ACCUM
                generic_tile.add_instructions_after({
                    tile(REG32_ACCUM, *node.getRet())
                });
            }
            // Function epilogue
            generic_tile.add_instructions_after({
                Comment("function epilogue"),
                Mov(REG32_STACKPTR, REG32_STACKBASEPTR),
                Pop(REG32_STACKBASEPTR),
                Ret()
            });
        },

        [&](CallIR &node) {
            std::string called_function;
            if (auto name = std::get_if<NameIR>(&node.getTarget())) {
                called_function = name->getName();
            } else {
                THROW_CompilerError("Function call target is not a label"); 
            }

            // Special case : __malloc call
            if (called_function == "__malloc") {
                if (node.getArgs().size() != 1) {
                    THROW_CompilerError("malloc called with " + std::to_string(node.getArgs().size()) + " args instead of 1");
                }

                generic_tile.add_instructions_after({
                    tile(REG32_ACCUM, *node.getArgs().front()),
                    Call(called_function)
                });

                return;
            }

            // Not a special case

            // Push arguments onto stack, in reverse order (CDECL)
            for (auto &arg : node.getArgs()) {
                std::string argument_register = newAbstractRegister();
                generic_tile.add_instructions_before({
                    tile(argument_register, *arg),
                    Push(argument_register)
                });
            }
            generic_tile.add_instructions_before({Comment("Call: pushing arguments onto stack")});

            // Perform call instruction on function label
            generic_tile.add_instruction(Call(called_function));

            // Pop arguments from stack
            generic_tile.add_instruction(Comment("Call: popping arguments off stack"));
            generic_tile.add_instruction(Add(REG32_STACKPTR, 4 * node.getNumArgs()));
        },

        [&](SeqIR &node) {
            generic_tile = Tile(std::vector<Instruction>());
            for (auto& stmt : node.getStmts()) {
                generic_tile.add_instruction(tile(*stmt));
            }
        },

        [&](ExpIR &node) {
            THROW_CompilerError("ExpIR should not exist after canonicalization"); 
        },

        [&](CommentIR &node) {
            generic_tile = Tile(std::vector<Instruction>());
        }
    }, ir);

    if (generic_tile.getCost() == Tile().getCost()) {
        THROW_CompilerError("Generic tile was not assigned to " + std::visit([&](auto &x){ return x.label(); }, ir));
    }
    decideIsCandidate(ir, generic_tile);

    return &statement_memo[&ir];
}
