#pragma once

#include <string>
#include <variant>
#include <vector>
#include <unordered_set>

#include "utillities/overload.h"
#include "exceptions/exceptions.h"

// Available x86 addressing modes
struct EffectiveAddress {
    const static inline std::string EMPTY_REG = "";

    std::string base_register = EMPTY_REG;
    std::string index_register = EMPTY_REG;
    int scale = 1;
    int displacement = 0;

    EffectiveAddress(std::string base) : base_register{base} {}

    EffectiveAddress(std::string base, int dis) : 
        base_register{base}, displacement{dis} 
    {}

    EffectiveAddress(std::string base, std::string index) : 
        base_register{base}, index_register{index} 
    {}

    EffectiveAddress(std::string base, std::string index, int scale) : 
        base_register{base}, index_register{index}, scale{scale} 
    {}

    EffectiveAddress(std::string base, std::string index, int scale, int dis) : 
        base_register{base}, index_register{index}, scale{scale}, displacement{dis}
    {}

    std::string toString();
};

// Operand for x86 assembly instructions
//
// Register (real or abstract), effective address, or immediate
struct Operand : public std::variant<EffectiveAddress, std::string, int32_t> {
    using variant::variant;

    bool is_read = false;
    bool is_write = false;

    // Set the operands read/write status and return it
    Operand& read() {
        is_read = true;
        return *this;
    }

    Operand& write() {
        is_write = true;
        return *this;
    }

    Operand& readwrite() {
        is_read = true;
        is_write = true;
        return *this;
    }

    std::string toString() {
        return std::visit(util::overload {
            [&](EffectiveAddress &adr) { return adr.toString(); },
            [&](std::string &reg) { return reg; },
            [&](int32_t immediate) { return std::to_string(immediate); }
        }, *this);
    }
};

class AssemblyCommon {

    std::vector<Operand> operands; // Used operands

  protected:
    template<typename... OperandType>
    void useOperands(OperandType&... operands) {
        (this->operands.push_back(operands), ...);
    }

    // Get the index'th op (indexed starting at 1)
    Operand& getOp(size_t index) {
        if (index > operands.size()) {
            THROW_CompilerError("Attempted to get operand out of range");
        }
        return operands[index - 1];
    }

  public:
    void replaceRegister(std::string original_register, std::string new_register) {
        for (auto& operand : operands) {
            // Replace any register equal to the original register in each operand
            std::visit(util::overload {
                [&](EffectiveAddress& adr) {
                    if (adr.index_register == original_register) {
                        adr.index_register = new_register;
                    }
                    if (adr.base_register == original_register) {
                        adr.base_register = original_register;
                    }
                },
                [&](std::string& reg) {
                    if (reg == original_register) {
                        reg = new_register;
                    }
                },
                [&](int32_t) {}
            }, operand);
        }
    }

    std::unordered_set<std::string> getReadRegisters() {
        std::unordered_set<std::string> result;

        for (auto& operand : operands) {
            std::visit(util::overload {
                [&](EffectiveAddress& adr) {
                    // Registers used in an effective address are always read, even if the operand itself isn't
                    if (adr.index_register != EffectiveAddress::EMPTY_REG) {
                        result.insert(adr.index_register);
                    }
                    if (adr.base_register != EffectiveAddress::EMPTY_REG) {
                        result.insert(adr.base_register);
                    }
                },
                [&](std::string& reg) {
                    if (operand.is_read) {
                        result.insert(reg);
                    }
                },
                [&](int32_t) {}
            }, operand);
        }

        return std::move(result);
    }

    std::unordered_set<std::string> getWriteRegisters() {
        std::unordered_set<std::string> result;

        for (auto& operand : operands) {
            std::visit(util::overload {
                [&](EffectiveAddress& adr) {},
                [&](std::string& reg) {
                    if (operand.is_write) {
                        result.insert(reg);
                    }
                },
                [&](int32_t) {}
            }, operand);
        }

        return std::move(result);
    }

    std::unordered_set<std::string> getUsedRegisters() {
        std::unordered_set<std::string> result;

        for (auto& operand : operands) {
            std::visit(util::overload {
                [&](EffectiveAddress& adr) {
                    if (adr.index_register != EffectiveAddress::EMPTY_REG) {
                        result.insert(adr.index_register);
                    }
                    if (adr.base_register != EffectiveAddress::EMPTY_REG) {
                        result.insert(adr.base_register);
                    }
                },
                [&](std::string& reg) {
                    result.insert(reg);
                },
                [&](int32_t) {}
            }, operand);
        }

        return std::move(result);
    }

};
