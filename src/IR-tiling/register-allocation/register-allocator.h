#pragma once

#include <cstdint>

#include "IR-tiling/tiling/tile.h"

// Abstract class for a register allocation algorithm.
class RegisterAllocator {
  public:
    // Allocate concrete registers or "spill to stack" for all abstract registers in a function body, mutating it.
    //
    // Returns the number of stack spaces required for the function.
    virtual int32_t allocateRegisters(std::list<AssemblyInstruction>& function_body) = 0;

    virtual ~RegisterAllocator() = default;
};
