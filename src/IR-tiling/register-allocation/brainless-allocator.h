#pragma once

#include "register-allocator.h"

#include <unordered_map>
#include <vector>

// Register allocator that spills all variables to the stack.
class BrainlessRegisterAllocator : public RegisterAllocator {
    std::unordered_map<std::string, int> reg_offsets;
    int stack_offset = 4;

    // Registers designated to load stack values into
    static std::vector<std::string> instruction_registers;

    // Generate code to load the abstract register
    AssemblyInstruction loadAbstractRegister(std::string reg_to, std::string abstract_reg);

    // Generate code to store the abstract register
    AssemblyInstruction storeAbstractRegister(std::string abstract_reg, std::string reg_from);

    // Pick a stack offset for each abstract register, and place in map
    void findOffsets(std::list<AssemblyInstruction>& function_body);

    // Generate code to load all abstract registers into real registers, and replace the use of abstracts with reals
    void replaceAbstracts(AssemblyInstruction& instruction, std::list<AssemblyInstruction>& target);

  public:
    int32_t allocateRegisters(std::list<AssemblyInstruction>& function_body) override;
};
