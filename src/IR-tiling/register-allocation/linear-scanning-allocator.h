#pragma once

#include "register-allocator.h"

#include <unordered_map>
#include <vector>
#include <string>

// Register allocator that greedily allocates registers based on the live interval of temporaries.
class LinearScanningRegisterAllocator : public RegisterAllocator {
  public:
    int32_t allocateRegisters(std::list<AssemblyInstruction>& function_body) override = 0;
};
