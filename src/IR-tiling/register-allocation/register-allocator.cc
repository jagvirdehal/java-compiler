#include <unordered_map>
#include <string>

#include "register-allocator.h"
#include "IR-tiling/assembly/assembly-instruction.h"
#include "IR-tiling/assembly/registers.h"

using namespace Assembly;

// Helper for asserting temporaries are not used without values being set
void RegisterAllocator::checkAllTemporariesInitialized(std::list<AssemblyInstruction>& function_body) {
    std::unordered_set<std::string> initialized;

    for (auto& instr : function_body) {
        for (auto& reg : instr.getReadRegisters()) {
            if (!isRealRegister(reg) && !initialized.count(reg)) {
                THROW_CompilerError("Temporary " + reg + " was never initialized!");
            }
        }

        for (auto& reg : instr.getWriteRegisters()) {
            if (!isRealRegister(reg) && !initialized.count(reg)) {
                // Temporary is set to a value as of this instruction
                initialized.insert(reg);
            }
        }
    }
}

AssemblyInstruction RegisterAllocator::loadAbstractRegister(std::string reg_to, std::string abstract_reg) {
    return Mov(reg_to, EffectiveAddress(REG32_STACKBASEPTR, -1 * reg_offsets[abstract_reg]));
}

AssemblyInstruction RegisterAllocator::storeAbstractRegister(std::string abstract_reg, std::string reg_from) {
    return Mov(EffectiveAddress(REG32_STACKBASEPTR, -1 * reg_offsets[abstract_reg]), reg_from);
}

void RegisterAllocator::replaceAbstracts(AssemblyInstruction& instruction, std::list<AssemblyInstruction>& target) {
    std::string original_instruction_text = instruction.toString();

    auto used_registers = instruction.getUsedRegisters();
    auto read_registers = instruction.getReadRegisters();
    auto write_registers = instruction.getWriteRegisters();

    target.push_back(LineBreak());

    // Each x86 instruction can use at most 3 registers; just asserting this is true
    if (used_registers.size() > 3) {
        std::string found_registers = "";
        for (auto& reg : used_registers) found_registers += " " + reg;
        THROW_CompilerError(
            "x86 instruction using more than 3 registers? Something is wrong.\n"
            "Instruction: " + original_instruction_text + "\n"
            "Found registers:" + found_registers
        );
    }

    // Replace each abstract register the instruction uses with a real register
    size_t next_real_reg = 0;
    std::unordered_map<std::string, std::string> abstract_to_real;

    for (auto& reg : used_registers) {
        if (!isRealRegister(reg)) {
            abstract_to_real[reg] = instruction_registers[next_real_reg++];
            instruction.replaceRegister(reg, abstract_to_real[reg]);
        }
    }

    // Add a load instruction for each abstract register the instruction reads
    for (auto& reg : read_registers) {
        if (!isRealRegister(reg)) {
            target.emplace_back(loadAbstractRegister(abstract_to_real[reg], reg));
            target.back().tagWithComment("Load from " + reg);
        }
    }

    // Add the original instruction, now modified to use real registers
    if (next_real_reg > 0) instruction.tagWithComment(original_instruction_text); // Tag if we did replacement
    target.push_back(instruction);

    // Add a store instruction for each abstract register the instruction writes
    for (auto& reg : write_registers) {
        if (!isRealRegister(reg)) {
            target.emplace_back(storeAbstractRegister(reg, abstract_to_real[reg]));
            target.back().tagWithComment("Store to " + reg);
        }
    }
}  
