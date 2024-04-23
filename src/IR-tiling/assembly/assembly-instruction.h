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
    AssemblyRefactor::Comment,
    AssemblyRefactor::Label
>;

struct AssemblyInstruction : public AssemblyInstructionInheritedVariant {
    using AssemblyInstructionInheritedVariant::AssemblyInstructionInheritedVariant;
    using AssemblyInstructionInheritedVariant::operator=;

    std::unordered_set<std::string> getUsedRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string> { return x.getUsedRegisters(); }
        }, *this);
    }

    std::unordered_set<std::string> getWriteRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string> { return x.getWriteRegisters(); }
        }, *this);
    }

    std::unordered_set<std::string> getReadRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string> { return x.getReadRegisters(); }
        }, *this);
    }

    void replaceRegister(std::string original_register, std::string new_register) {
        return std::visit(util::overload {
            [&](auto &x) { return x.replaceRegister(original_register, new_register); }
        }, *this);
    }

    std::string toString() {
        return std::visit(util::overload {
            [&](auto &x) { return x.toString(); }
        }, *this);
    }
};
