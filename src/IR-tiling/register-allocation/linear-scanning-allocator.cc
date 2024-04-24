#include "linear-scanning-allocator.h"

#include "brainless-allocator.h"
#include "IR-tiling/tiling/tile.h"
#include "utillities/overload.h"
#include "IR-tiling/assembly/assembly.h"
#include "IR-tiling/assembly/registers.h"
#include "exceptions/exceptions.h"
#include "utillities/overload.h"

#include <iostream>
#include <regex>

using namespace Assembly;

void LinearScanningRegisterAllocator::constructIntervals(std::list<AssemblyInstruction>& function_body) {

    auto commitInterval = [&](const Interval& interval) { intervals.emplace_back(interval); };

    std::unordered_map<Register, Interval> uncomitted_intervals;

    size_t current = -1;
    for (auto &instr : function_body) {
        ++current;

        for (auto &reg : instr.getWriteRegisters()) {
            if (uncomitted_intervals.count(reg)) {
                // Interval already open
                //
                // If this register wasn't read on this instruction, 
                // the last live interval containing this register is committed, 
                // with the end being when it was last read.
                // 
                // A new one is opened.
                if (!instr.getReadRegisters().count(reg)) {
                    commitInterval(uncomitted_intervals[reg]);
                    uncomitted_intervals[reg] = Interval(current, current, reg);
                }
            } else {
                // Open a new interval for the register
                uncomitted_intervals[reg] = Interval(current, current, reg); 
            }
        }

        for (auto &reg : instr.getReadRegisters()) {
            // Update the end of this interval, as the live range must contain all reads since the write
            uncomitted_intervals[reg].end = current;
        }
    }

    // Commit all uncomitted intrevals
    for (auto& [_, interval] : uncomitted_intervals) {
        commitInterval(interval);
    }

    // Sort intervals by starting
    intervals.sort([&](const Interval& first, const Interval& second) -> bool { return first.start < second.start; });
}

void LinearScanningRegisterAllocator::freeInterval(Interval& interval) {
    // Free register or stack space, and remove from active intervals
    std::visit(util::overload {
        [&](StackOffset offset) {
            free_stack_spaces.insert(offset);
        },
        [&](Register reg) {
            free_registers.insert(reg);
        }
    }, interval.assignment);

    active_intervals.remove(&interval);
}

void LinearScanningRegisterAllocator::assignInterval(Interval& interval, Register predetermined) {
    // Interval becomes active
    active_intervals.push_back(&interval);

    // If we are supposed to assign to a specific register, do that
    if (predetermined != "") {
        interval.assignment = predetermined;
        free_registers.erase(predetermined);
        return;
    }

    // If there is a free register, take that
    for (auto &reg : free_registers) {
        interval.assignment = reg;
        free_registers.erase(reg);
        return;
    }

    // Must spill to stack

    // If there is a free stack space we used before, take that
    // This prevents us from having to allocate a larger stack frame than necessary
    for (auto offset : free_stack_spaces) {
        interval.assignment = offset;
        free_stack_spaces.erase(offset);
        return;
    }

    // Allocate a new stack space
    interval.assignment = next_stack_offset;
    next_stack_offset += 4;
}

void LinearScanningRegisterAllocator::finishInactiveIntervals(size_t current_instruction) {
    for (auto interval : active_intervals) {
        if (interval->end > current_instruction) {
            freeInterval(*interval);
        }
    }
}

int32_t LinearScanningRegisterAllocator::allocateRegisters(std::list<AssemblyInstruction>& function_body) {
    checkAllTemporariesInitialized(function_body);

    // Construct the live intervals
    constructIntervals(function_body);

    // Assign each interval a register or stack space
    active_intervals = {};
    free_registers = allocatable_registers;

    for (auto& interval : intervals) {
        finishInactiveIntervals(interval.start);

        // Real registers need to be assigned to themselves, so if something else was,
        // kick it out and assign it to something else
        if (isRealRegister(interval.original_register)) {
            if (!free_registers.count(interval.original_register)) {
                for (auto active_interval : active_intervals) {
                    if (active_interval->assignmentIs(interval.original_register)) {
                        // Do reassignment of abstract, not allowing it to be assigned to the real thats needed
                        freeInterval(*active_interval);

                        free_registers.erase(interval.original_register);
                        assignInterval(*active_interval);
                        free_registers.insert(interval.original_register);

                        break;
                    }
                }
            }

            // Assign real register to itself
            assignInterval(interval, interval.original_register);
        } else {
            assignInterval(interval);
        }
    }

    // Do actual instruction replacement
    std::list<AssemblyInstruction> new_instructions;

    active_intervals = {};

    auto activateIntervals = [&](size_t current){
        for (auto& interval : intervals) {
            if (interval.start == current) {
                active_intervals.push_back(&interval);
            }
        }
    };

    size_t current = -1;
    for (auto &instruction : function_body) {
        ++current;

        finishInactiveIntervals(current);
        activateIntervals(current);

        // Replace abstract registers with allocated registers
        for (auto active_interval : active_intervals) {
            if (Register* reg_ptr = std::get_if<Register>(&active_interval->assignment)) {
                instruction.replaceRegister(active_interval->original_register, *reg_ptr);
            }
        }

        // Replace abstract registers with instruction registers with loading/storing stack memory
        for (auto active_interval : active_intervals) {
            if (StackOffset* offset_ptr = std::get_if<StackOffset>(&active_interval->assignment)) {
                reg_offsets[active_interval->original_register] = *offset_ptr;
            }
        }
        replaceAbstracts(instruction, new_instructions);
    }

    function_body = new_instructions;

    return (next_stack_offset - 4) / 4;
}
