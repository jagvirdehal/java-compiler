#include "brainless-allocator.h"
#include "IR-tiling/tiling/tile.h"
#include "utillities/overload.h"
#include "IR-tiling/assembly/assembly-refactor.h"
#include "IR-tiling/assembly/registers.h"
#include "exceptions/exceptions.h"

#include <iostream>
#include <regex>

using namespace AssemblyRefactor;

std::vector<std::string> BrainlessRegisterAllocator::instruction_registers 
    = {REG32_COUNTER, REG32_SOURCE, REG32_DEST};

void BrainlessRegisterAllocator::findOffsets(std::list<AssemblyInstruction>& function_body) {
    for (auto& instr : function_body) {
        for (auto& reg : instr.getUsedRegisters()) {
            // Assign an offset to every abstract register used that hasn't been assigned one yet
            if (!isRealRegister(reg) && !reg_offsets.count(reg)) {
                reg_offsets[reg] = stack_offset;
                stack_offset += 4;
            }
        }
    }
}

int32_t BrainlessRegisterAllocator::allocateRegisters(std::list<AssemblyInstruction>& function_body) {
    findOffsets(function_body);

    std::list<AssemblyInstruction> new_instructions;

    for (auto& instr : function_body) {
        replaceAbstracts(instr, new_instructions);
    }

    function_body = new_instructions;

    return reg_offsets.size();
}

AssemblyInstruction BrainlessRegisterAllocator::loadAbstractRegister(std::string reg_to, std::string abstract_reg) {
    return Mov(reg_to, EffectiveAddress(REG32_STACKBASEPTR, -1 * reg_offsets[abstract_reg]));
}

AssemblyInstruction BrainlessRegisterAllocator::storeAbstractRegister(std::string abstract_reg, std::string reg_from) {
    return Mov(EffectiveAddress(REG32_STACKBASEPTR, -1 * reg_offsets[abstract_reg]), reg_from);
}

void BrainlessRegisterAllocator::replaceAbstracts(AssemblyInstruction& instruction, std::list<AssemblyInstruction>& target) {
    std::string original_instruction_text = instruction.toString();

    auto used_registers = instruction.getUsedRegisters();
    auto read_registers = instruction.getReadRegisters();
    auto write_registers = instruction.getWriteRegisters();

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

    for (auto& reg : instruction.getUsedRegisters()) {
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
    instruction.tagWithComment(original_instruction_text);
    target.push_back(instruction);

    // Add a store instruction for each abstract register the instruction writes
    for (auto& reg : write_registers) {
        if (!isRealRegister(reg)) {
            target.emplace_back(storeAbstractRegister(reg, abstract_to_real[reg]));
            target.back().tagWithComment("Store to " + reg);
        }
    }
}  
