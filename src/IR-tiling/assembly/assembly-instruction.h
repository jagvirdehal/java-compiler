#pragma once

#include "assembly.h"
#include <variant>

// AssemblyInstruction variant, to treat instructions polymorphically without needing pointers

using AssemblyInstructionInheritedVariant = std::variant<
    Assembly::Mov,
    Assembly::Jump,
    Assembly::Je,
    Assembly::JumpIfNZ,
    Assembly::Lea,
    Assembly::Add,
    Assembly::Sub,
    Assembly::Xor,
    Assembly::And,
    Assembly::Or,
    Assembly::MovZX,
    Assembly::Cmp,
    Assembly::Test,
    Assembly::Push,
    Assembly::Pop,
    Assembly::Cdq,
    Assembly::Ret,
    Assembly::Call,
    Assembly::SysCall,
    Assembly::SetZ,
    Assembly::SetNZ,
    Assembly::SetL,
    Assembly::SetG,
    Assembly::SetLE,
    Assembly::SetGE,
    Assembly::IMul,
    Assembly::IDiv,
    Assembly::Comment,
    Assembly::Label,
    Assembly::LineBreak
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

    std::unordered_set<std::string> getUsedAbstractRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string> { return x.getUsedAbstractRegisters(); }
        }, *this);
    }

    std::unordered_set<std::string> getWriteAbstractRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string> { return x.getWriteAbstractRegisters(); }
        }, *this);
    }

    std::unordered_set<std::string> getReadAbstractRegisters() {
        return std::visit(util::overload {
            [&](auto &x) -> std::unordered_set<std::string> { return x.getReadAbstractRegisters(); }
        }, *this);
    }

    void replaceRegister(std::string original_register, std::string new_register) {
        return std::visit(util::overload {
            [&](auto &x) { return x.replaceRegister(original_register, new_register); }
        }, *this);
    }

    std::string toString() {
        return std::visit(util::overload {
            [&](auto &x) {
                if (x.tagged_comment != "") {
                    return x.toString() + " ; " + x.tagged_comment;
                }
                return x.toString(); 
            }
        }, *this);
    }

    void tagWithComment(const std::string& text) {
        return std::visit(util::overload {
            [&](auto &x) {
                x.tagWithComment(text);
            }
        }, *this);
    }

    bool hasComment() {
        return std::visit(util::overload {
            [&](auto &x) {
                return x.tagged_comment != "";
            }
        }, *this);
    }
};
