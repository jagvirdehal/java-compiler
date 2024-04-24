#pragma once

#include "register-allocator.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <variant>

// Register allocator that greedily allocates registers based on the live interval of temporaries.
class LinearScanningRegisterAllocator : public RegisterAllocator {

    using StackOffset = size_t;
    using Register = std::string;

    struct Interval {
        size_t start;
        size_t end;
        Register original_register;

        std::variant<StackOffset, Register> assignment;

        bool assignmentIs(std::variant<StackOffset, Register> other_assignment) {
            return assignment == other_assignment;
        }

        Interval(size_t start, size_t end, Register original_register) 
            : start{start}, end{end}, original_register{original_register} {}
        Interval() = default;
    };

    std::unordered_set<Register> allocatable_registers = {
        Assembly::REG32_ACCUM,
        Assembly::REG32_BASE,
        Assembly::REG32_COUNTER,
        Assembly::REG32_DATA,
        Assembly::REG32_SOURCE,
        Assembly::REG32_DEST
    };
    size_t next_stack_offset = 4;

    std::unordered_set<Register> free_registers;
    std::unordered_set<StackOffset> free_stack_spaces;

    std::list<Interval> intervals;

    std::list<Interval*> active_intervals;

    void constructIntervals(std::list<AssemblyInstruction>& function_body);

    void assignInterval(Interval& interval, Register predetermined="");

    void freeInterval(Interval& interval);

    void finishInactiveIntervals(size_t current_instruction);

  public:
    int32_t allocateRegisters(std::list<AssemblyInstruction>& function_body) override;
};
