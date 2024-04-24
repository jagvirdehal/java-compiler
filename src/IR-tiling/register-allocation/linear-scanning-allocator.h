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
    using Assignment = std::variant<std::monostate, StackOffset, Register>;

    struct Interval {
        size_t start;
        size_t end;

        Register original_register;
        Assignment assignment;

        bool assignmentIs(Assignment other_assignment) {
            return assignment == other_assignment;
        }

        Interval(size_t start, size_t end, Register original_register) 
            : start{start}, end{end}, original_register{original_register}, assignment{} {}
        Interval() = default;

        std::string toString() {
            std::string assignment_string = "";

            if (std::get_if<Register>(&assignment)) assignment_string = std::get<Register>(assignment) ;
            if (std::get_if<StackOffset>(&assignment)) assignment_string = std::to_string(std::get<StackOffset>(assignment));

            return "Interval(" + std::to_string(start) + ", " + std::to_string(end) + ", " 
            + original_register 
            + " -> " 
            + assignment_string;
        }
    };

    std::unordered_set<Register> allocatable_registers = {
        Assembly::REG32_ACCUM,
        Assembly::REG32_BASE,
        // Assembly::REG32_COUNTER,
        Assembly::REG32_DATA,
        // Assembly::REG32_SOURCE,
        // Assembly::REG32_DEST
    };
    size_t next_stack_offset = 4;

    std::unordered_set<Register> free_registers = {};
    std::unordered_set<StackOffset> free_stack_spaces = {};

    std::list<Interval> intervals = {};
    std::unordered_map<std::string, std::list<Interval>> real_intervals = {};

    std::list<Interval*> active_intervals = {};

    void constructIntervals(std::list<AssemblyInstruction>& function_body);

    size_t getFreeStackOffset();

    void assignInterval(Interval& interval, Assignment predetermined=std::monostate());

    void freeInterval(Interval& interval);

    void finishInactiveIntervals(size_t current_instruction);

    // Check if interval overlaps with any of a real regs interval
    bool intervalOverlapsWith(Interval& interval, Register real_reg);

    void printIntervals(bool print_reals_too=true);

  public:
    int32_t allocateRegisters(std::list<AssemblyInstruction>& function_body) override;
};
