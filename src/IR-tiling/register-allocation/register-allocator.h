#pragma once

#include <cstdint>
#include <list>
#include "IR-tiling/assembly/assembly-instruction.h"
#include "IR-tiling/assembly/registers.h"

// Abstract class for a register allocation algorithm.
class RegisterAllocator {
  protected:
    std::unordered_map<std::string, int> reg_offsets;

    // Registers stack-allocated temporaries are loaded into before being used in an instruction
    std::vector<std::string> instruction_registers 
      = {Assembly::REG32_COUNTER, Assembly::REG32_SOURCE, Assembly::REG32_DEST};

    // Helper for asserting temporaries are not used without values being set
    void checkAllTemporariesInitialized(std::list<AssemblyInstruction>& function_body);

    // Generate code to load the abstract register
    AssemblyInstruction loadAbstractRegister(std::string reg_to, std::string abstract_reg);

    // Generate code to store the abstract register
    AssemblyInstruction storeAbstractRegister(std::string abstract_reg, std::string reg_from);

    // Generate code to load all abstract registers into real registers, and replace the use of abstracts with reals
    void replaceAbstracts(AssemblyInstruction& instruction, std::list<AssemblyInstruction>& target);
  public:
    // Allocate concrete registers or "spill to stack" for all abstract registers in a function body, mutating it.
    //
    // Returns the number of stack spaces required for the function.
    virtual int32_t allocateRegisters(std::list<AssemblyInstruction>& function_body) = 0;

    virtual ~RegisterAllocator() = default;
};
