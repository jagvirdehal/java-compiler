#include "ir-tiling.h"
#include "utillities/util.h"
#include "exceptions/exceptions.h"
#include "IR-tiling/register-allocation/register-allocator.h"
#include "IR/code-gen-constants.h"

#include "IR-tiling/assembly/assembly-instruction.h"
#include "IR-tiling/assembly/registers.h"

#include <regex>

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
                        AssemblyRefactor::Lea(Tile::ABSTRACT_REG, EffectiveAddress(operand1_reg, operand2_reg))
                    });
                    break;
                } 
                case BinOpIR::OpType::SUB: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Sub(operand1_reg, operand2_reg),
                        AssemblyRefactor::Mov(Tile::ABSTRACT_REG, operand1_reg)
                    });
                    break;
                }
                case BinOpIR::OpType::MUL: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Mov(AssemblyRefactor::REG32_ACCUM, operand1_reg),
                        AssemblyRefactor::IMul(operand2_reg),
                        AssemblyRefactor::Mov(Tile::ABSTRACT_REG, AssemblyRefactor::REG32_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::DIV: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand2_reg, "0"), // Check for division by zero
                        AssemblyRefactor::Je("__exception"), // Jump to exception handler if division by zero
                        AssemblyRefactor::Mov(AssemblyRefactor::REG32_ACCUM, operand1_reg), // Move low 32 bits of dividend into accumulator
                        AssemblyRefactor::Cdq(), // Sign extend REG32_ACCUM into REG32_DATA
                        AssemblyRefactor::IDiv(operand2_reg),
                        AssemblyRefactor::Mov(Tile::ABSTRACT_REG, AssemblyRefactor::REG32_ACCUM) // Quotient stored in ACCUM
                    });
                    break;
                }
                case BinOpIR::OpType::MOD: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand2_reg, "0"), // Check for division by zero
                        AssemblyRefactor::Je("__exception"), // Jump to exception handler if division by zero
                        AssemblyRefactor::Mov(AssemblyRefactor::REG32_ACCUM, operand1_reg), // Move low 32 bits of dividend into accumulator
                        AssemblyRefactor::Cdq(), // Sign extend REG32_ACCUM into REG32_DATA
                        AssemblyRefactor::IDiv(operand2_reg),
                        AssemblyRefactor::Mov(Tile::ABSTRACT_REG, AssemblyRefactor::REG32_DATA) // Remainder stored in DATA
                    });
                    break;
                }
                case BinOpIR::OpType::AND: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::And(operand1_reg, operand2_reg),
                        AssemblyRefactor::Mov(Tile::ABSTRACT_REG, operand1_reg)
                    });
                    break;
                }
                case BinOpIR::OpType::OR: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Or(operand1_reg, operand2_reg),
                        AssemblyRefactor::Mov(Tile::ABSTRACT_REG, operand1_reg)
                    });
                    break;
                }
                case BinOpIR::OpType::EQ: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand1_reg, operand2_reg),
                        AssemblyRefactor::SetZ(AssemblyRefactor::REG8L_ACCUM),
                        AssemblyRefactor::MovZX(Tile::ABSTRACT_REG, AssemblyRefactor::REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::NEQ: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand1_reg, operand2_reg),
                        AssemblyRefactor::SetNZ(AssemblyRefactor::REG8L_ACCUM),
                        AssemblyRefactor::MovZX(Tile::ABSTRACT_REG, AssemblyRefactor::REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::LT: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand1_reg, operand2_reg),
                        AssemblyRefactor::SetL(AssemblyRefactor::REG8L_ACCUM),
                        AssemblyRefactor::MovZX(Tile::ABSTRACT_REG, AssemblyRefactor::REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::GT: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand1_reg, operand2_reg),
                        AssemblyRefactor::SetG(AssemblyRefactor::REG8L_ACCUM),
                        AssemblyRefactor::MovZX(Tile::ABSTRACT_REG, AssemblyRefactor::REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::LEQ: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand1_reg, operand2_reg),
                        AssemblyRefactor::SetLE(AssemblyRefactor::REG8L_ACCUM),
                        AssemblyRefactor::MovZX(Tile::ABSTRACT_REG, AssemblyRefactor::REG8L_ACCUM)
                    });
                    break;
                }
                case BinOpIR::OpType::GEQ: {
                    generic_tile.add_instructions_after({
                        AssemblyRefactor::Cmp(operand1_reg, operand2_reg),
                        AssemblyRefactor::SetGE(AssemblyRefactor::REG8L_ACCUM),
                        AssemblyRefactor::MovZX(Tile::ABSTRACT_REG, AssemblyRefactor::REG8L_ACCUM)
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
                AssemblyRefactor::Mov(Tile::ABSTRACT_REG, node.getValue())
            });
        },

        [&](MemIR &node) {
            std::string address_reg = newAbstractRegister();

            generic_tile = Tile({
<<<<<<< HEAD
                tile(address_reg, node.getAddress()),
                Assembly::Mov(Tile::ABSTRACT_REG, Assembly::MakeAddress(address_reg))
=======
                tile(node.getAddress(), address_reg),
                AssemblyRefactor::Lea(Tile::ABSTRACT_REG, EffectiveAddress(address_reg))
>>>>>>> progress
            });
        },

        [&](NameIR &node) {
            generic_tile = Tile({
                AssemblyRefactor::Mov(Tile::ABSTRACT_REG, node.getName())
            });
        },

        [&](TempIR &node) {
            // Special registers : ABSTRACT_RET is REG32_ACCUM
            if (node.getName() == CGConstants::ABSTRACT_RET) {
                generic_tile = Tile({
                    AssemblyRefactor::Mov(Tile::ABSTRACT_REG, AssemblyRefactor::REG32_ACCUM)
                });
            }
            // Special registers : ABSTRACT_ARG_PREFIX# is stack offset from caller
            else if (node.getName().rfind(CGConstants::ABSTRACT_ARG_PREFIX, 0) == 0) {
                int arg_num 
                    = std::stoi(std::regex_replace(node.getName(), std::regex(CGConstants::ABSTRACT_ARG_PREFIX), ""));

                generic_tile = Tile({
                    AssemblyRefactor::Mov(
                        Tile::ABSTRACT_REG, 
                        EffectiveAddress(AssemblyRefactor::REG32_STACKBASEPTR, 4 * (arg_num + 2))
                    )
                });
            } 
            // Special temp : Static variable in .data section
            else if (node.isGlobal) {
                generic_tile = Tile({
                    AssemblyRefactor::Mov(
                        Tile::ABSTRACT_REG,
                        EffectiveAddress(node.getName())
                    )
                });
            }
            // Not special
            else {
                generic_tile = Tile({
                    AssemblyRefactor::Mov(Tile::ABSTRACT_REG, escapeTemporary(node.getName()))
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
<<<<<<< HEAD
                tile(cond_reg, node.getCondition()),
                Assembly::Test(cond_reg, cond_reg),
                Assembly::JumpIfNZ(node.trueLabel())
=======
                tile(node.getCondition(), cond_reg),
                AssemblyRefactor::Test(cond_reg, cond_reg),
                AssemblyRefactor::JumpIfNZ(node.trueLabel())
>>>>>>> progress
            });
        },

        [&](JumpIR &node) {
            if (auto target = std::get_if<NameIR>(&node.getTarget())) {
                generic_tile = Tile({
                    Assembly::Jump(target->getName())
                });
            } else {
                std::string target_reg = newAbstractRegister();

<<<<<<< HEAD
                generic_tile = Tile({
                    tile(target_reg, node.getTarget()),
                    Assembly::Jump(target_reg)
                });
            }
=======
            generic_tile = Tile({
                tile(node.getTarget(), target_reg),
                AssemblyRefactor::Jump(target_reg)
            });
>>>>>>> progress
        },

        [&](LabelIR &node) {
            generic_tile = Tile({AssemblyRefactor::Label(node.getName())});
        },

        [&](MoveIR &node) {
            // Behaviour depends on target type
            std::visit(util::overload {

                [&](TempIR& target) {
                    if (target.isGlobal) {
                        // Update global variable
                        std::string temp_value_reg = newAbstractRegister();

                        generic_tile = Tile({
<<<<<<< HEAD
                            tile(temp_value_reg, node.getSource()),
                            Assembly::Mov(Assembly::MakeAddress(target.getName()), temp_value_reg)
=======
                            tile(node.getSource(), temp_value_reg),
                            AssemblyRefactor::Mov(EffectiveAddress(target.getName()), temp_value_reg)
>>>>>>> progress
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
<<<<<<< HEAD
                        tile(source_reg, node.getSource()),
                        tile(address_reg, target.getAddress()),
                        Assembly::Mov(Assembly::MakeAddress(address_reg), source_reg)
=======
                        tile(node.getSource(), source_reg),
                        tile(target.getAddress(), address_reg),
                        AssemblyRefactor::Mov(EffectiveAddress(address_reg), source_reg)
>>>>>>> progress
                    });
                },

                [&](auto&) { THROW_CompilerError("Invalid MoveIR target"); }

            }, node.getTarget());
        },

        [&](ReturnIR &node) {
            if (node.getRet()) {
                // Return a value by placing it in REG32_ACCUM
                generic_tile.add_instructions_after({
<<<<<<< HEAD
                    tile(Assembly::REG32_ACCUM, *node.getRet())
=======
                    AssemblyRefactor::Comment("return a value by placing it in REG32_ACCUM"),
                    tile(*node.getRet(), AssemblyRefactor::REG32_ACCUM)
>>>>>>> progress
                });
            }
            // Function epilogue
            generic_tile.add_instructions_after({
                AssemblyRefactor::Comment("function epilogue"),
                AssemblyRefactor::Mov(AssemblyRefactor::REG32_STACKPTR, AssemblyRefactor::REG32_STACKBASEPTR),
                AssemblyRefactor::Pop(AssemblyRefactor::REG32_STACKBASEPTR),
                AssemblyRefactor::Ret()
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
                    tile(Assembly::REG32_ACCUM, *node.getArgs().front()),
                    Assembly::Call(called_function)
                });

                return;
            }

            // Not a special case

            // Push arguments onto stack, in reverse order (CDECL)
            for (auto &arg : node.getArgs()) {
                std::string argument_register = newAbstractRegister();
<<<<<<< HEAD
                generic_tile.add_instructions_before({
                    tile(argument_register, *arg),
                    Assembly::Push(argument_register)
=======
                generic_tile.add_instructions_after({
                    tile(*arg, argument_register),
                    AssemblyRefactor::Push(argument_register)
>>>>>>> progress
                });
            }
            generic_tile.add_instructions_before({AssemblyRefactor::Comment("Call: pushing arguments onto stack")});

            // Perform call instruction on function label
<<<<<<< HEAD
            generic_tile.add_instruction(Assembly::Call(called_function));
=======
            if (auto name = std::get_if<NameIR>(&node.getTarget())) {
                generic_tile.add_instruction(AssemblyRefactor::Call(name->getName()));
            } else {
                THROW_CompilerError("Function call target is not a label"); 
            }
>>>>>>> progress

            // Pop arguments from stack
            generic_tile.add_instruction(AssemblyRefactor::Comment("Call: popping arguments off stack"));
            generic_tile.add_instruction(AssemblyRefactor::Add(AssemblyRefactor::REG32_STACKPTR, 4 * node.getNumArgs()));
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
