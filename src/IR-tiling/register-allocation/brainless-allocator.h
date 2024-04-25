#pragma once

#include "register-allocator.h"

#include <unordered_map>
#include <vector>
#include <string>

// Register allocator that spills all variables to the stack.
class BrainlessRegisterAllocator : public RegisterAllocator {
    int stack_offset = 4;

    // Pick a stack offset for each abstract register, and place in map
    void findOffsets(std::list<AssemblyInstruction>& function_body);

  public:
    int32_t allocateRegisters(std::list<AssemblyInstruction>& function_body) override;
};
