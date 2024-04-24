#include "brainless-allocator.h"
#include "IR-tiling/tiling/tile.h"
#include "utillities/overload.h"
#include "IR-tiling/assembly/assembly.h"
#include "IR-tiling/assembly/registers.h"
#include "exceptions/exceptions.h"

#include <iostream>
#include <regex>

using namespace Assembly;

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
    checkAllTemporariesInitialized(function_body);

    findOffsets(function_body);

    std::list<AssemblyInstruction> new_instructions;
    for (auto& instr : function_body) {
        replaceAbstracts(instr, new_instructions);
    }
    function_body = new_instructions;

    return reg_offsets.size();
}
