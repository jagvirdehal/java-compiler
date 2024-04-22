#pragma once

#include "assembly-refactor.h"
#include <variant>

// AssemblyInstruction variant, to treat instructions polymorphically without needing pointers

using AssemblyInstructionInheritedVariant = std::variant<
    AssemblyRefactor::Mov,
    AssemblyRefactor::Jump,
    AssemblyRefactor::Je,
    AssemblyRefactor::JumpIfNZ,
    AssemblyRefactor::Lea,
    AssemblyRefactor::Add,
    AssemblyRefactor::Sub,
    AssemblyRefactor::Xor,
    AssemblyRefactor::And,
    AssemblyRefactor::Or,
    AssemblyRefactor::MovZX,
    AssemblyRefactor::Cmp,
    AssemblyRefactor::Test,
    AssemblyRefactor::Push,
    AssemblyRefactor::Pop,
    AssemblyRefactor::Cdq,
    AssemblyRefactor::Ret,
    AssemblyRefactor::Call,
    AssemblyRefactor::SysCall,
    AssemblyRefactor::SetZ,
    AssemblyRefactor::SetNZ,
    AssemblyRefactor::SetL,
    AssemblyRefactor::SetG,
    AssemblyRefactor::SetLE,
    AssemblyRefactor::SetGE,
    AssemblyRefactor::IMul,
    AssemblyRefactor::IDiv,
    AssemblyRefactor::Comment
>;

struct AssemblyInstruction : public AssemblyInstructionInheritedVariant {
    using AssemblyInstructionInheritedVariant::AssemblyInstructionInheritedVariant;
    using AssemblyInstructionInheritedVariant::operator=;

    std::unordered_set<std::string*>& getUsedRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string*>& { return x.used_registers; }
        }, *this);
    }

    std::unordered_set<std::string*>& getWriteRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string*>& { return x.write_registers; }
        }, *this);
    }

    std::unordered_set<std::string*>& getReadRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string*>& { return x.read_registers; }
        }, *this);
    }

    std::string toString() {
        return std::visit(util::overload {
            [&](auto &x) { return x.toString(); }
        }, *this);
    }
};
