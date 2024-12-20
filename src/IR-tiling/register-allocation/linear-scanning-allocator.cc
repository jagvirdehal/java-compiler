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
#include <cassert>

using namespace Assembly;

void LinearScanningRegisterAllocator::activateIntervals(size_t current) {
    for (auto& interval : intervals) {
        if (interval.start == current) {
            assignInterval(interval, interval.assignment);
        }
    }
};

void LinearScanningRegisterAllocator::constructIntervals(std::list<AssemblyInstruction>& function_body) {

    auto commitInterval = [&](Interval& interval) { 
        if (isRealRegister(interval.original_register)) {
            real_intervals[interval.original_register].push_back(interval);
        } else {
            intervals.push_back(interval); 
        }
    };

    std::unordered_map<Register, Interval> uncomitted_intervals;

    size_t current = -1;
    for (auto &instr : function_body) {
        ++current;

        for (auto &reg : instr.getUsedRegisters()) {
            if (stack_ptr_set.count(reg) || stack_base_ptr_set.count(reg)) continue;
            if (uncomitted_intervals.count(reg)) {
                uncomitted_intervals[reg].end = current;
            } else {
                // Open a new interval for the register
                uncomitted_intervals[reg] = Interval(current, current, reg); 
            }
        }
    }

    // Commit all uncomitted intrevals
    for (auto& [_, interval] : uncomitted_intervals) {
        commitInterval(interval);
    }

    // Sort intervals by starting
    intervals.sort([&](const Interval& first, const Interval& second) -> bool { return first.start < second.start; });
}

void LinearScanningRegisterAllocator::extendToLabels(std::list<AssemblyInstruction>& function_body) {

    active_intervals = {};
    std::unordered_map<std::string, size_t> label_indices;
    std::unordered_map<std::string, std::unordered_set<Interval*>> label_to_no_clobber_intervals;

    size_t current = -1;
    for (auto &instr : function_body) {
        ++current;

        finishInactiveIntervals(current);
        activateIntervals(current);

        if (std::get_if<Label>(&instr)) {
            label_indices[instr.toString()] = current;
            for (auto active_interval : active_intervals) {
                // This interval should be considered still active at any code that jumps to the label
                //
                // Any code that jumps to this label should not clobber any register active at the label definition
                label_to_no_clobber_intervals[instr.toString()].insert(active_interval);
            }
            continue;
        }

        for (auto &label : instr.getUsedLabels()) {
            // This is an instruction that MAY jump to label
            auto& no_clobber = label_to_no_clobber_intervals[label + ":"];
            for (auto interval : no_clobber) {
                if (interval->end < current) interval->end = current;
            }
        }
    }
}

void LinearScanningRegisterAllocator::printIntervals(bool print_reals_too) {
    std::cout << "Printing Abstract Intervals " << "\n";
    for (auto& interval : intervals) {
        std::cout << interval.toString() << "\n";
    }
    std::cout << "Done Printing Abstract Intervals " << "\n";

    if (print_reals_too) {
        std::cout << "Printing Real Intervals " << "\n";
        for (auto& [real_name, interval_list] : real_intervals) {
            for (auto& interval : interval_list) {
                std::cout << interval.toString() << "\n";
            }
        }
        std::cout << "Done Printing Real Intervals " << "\n";
    }

    std::cout << "\n";
}

void LinearScanningRegisterAllocator::freeInterval(Interval& interval) {
    // Free register or stack space, and remove from active intervals
    std::visit(util::overload {
        [&](StackOffset offset) {
            free_stack_spaces.insert(offset);
        },
        [&](Register reg) {
            if (allocatable_registers.count(reg)) {
                free_registers.insert(reg);
            }
        },
        [&](auto&){}
    }, interval.assignment);

    active_intervals.remove(&interval);
}

size_t LinearScanningRegisterAllocator::getFreeStackOffset() {
    // If there is a free stack space we used before, take that
    // This prevents us from having to allocate a larger stack frame than necessary
    for (auto offset : free_stack_spaces) {
        free_stack_spaces.erase(offset);
        return offset;
    }

    // Allocate a new stack space
    next_stack_offset += 4;
    return next_stack_offset - 4;
}

void LinearScanningRegisterAllocator::assignInterval(Interval& interval, Assignment predetermined) {
    // Interval becomes active
    active_intervals.push_back(&interval);

    // If we are supposed to assign to a specific register/offset, do that
    if (!std::get_if<std::monostate>(&predetermined)) {
        interval.assignment = predetermined;
        std::visit(util::overload {
            [&](Register& reg) { free_registers.erase(reg); },
            [&](StackOffset offset) { free_stack_spaces.erase(offset); },
            [&](auto&) {}
        }, predetermined);
        return;
    }

    // If there is a free register, take that
    for (auto &reg : free_registers) {
        if (intervalOverlapsWith(interval, reg)) continue; // Real register cannot be clobbered
        interval.assignment = reg;
        free_registers.erase(reg);
        return;
    }

    // Must spill to stack

    // Allocate a new stack space
    interval.assignment = getFreeStackOffset();
}

void LinearScanningRegisterAllocator::finishInactiveIntervals(size_t current_instruction) {
    std::list<Interval*> inactive;

    for (auto interval : active_intervals) {
        if (interval->end < current_instruction) {
            inactive.push_back(interval);
        }
    }

    for (auto interval : inactive) {
        freeInterval(*interval);
    }
}

bool LinearScanningRegisterAllocator::intervalOverlapsWith(Interval& interval, Register real_reg) {
    auto& interval_list = real_intervals[real_reg];
    for (auto& real_interval : interval_list) {
        if (interval.start <= real_interval.start && interval.start <= real_interval.end) return true;
        if (real_interval.start <= interval.start && real_interval.start <= interval.end) return true;
    }
    return false;
}

int32_t LinearScanningRegisterAllocator::allocateRegisters(std::list<AssemblyInstruction>& function_body) {
    checkAllTemporariesInitialized(function_body);

    // Construct the live intervals
    constructIntervals(function_body);
    extendToLabels(function_body);

    // Assign each interval a register or stack space
    active_intervals = {};
    free_registers = allocatable_registers;

    for (auto& interval : intervals) {
        finishInactiveIntervals(interval.start);
        assignInterval(interval);
    }

    // Do actual instruction replacement
    std::list<AssemblyInstruction> new_instructions;

    active_intervals = {};

    size_t current = -1;
    for (auto &instruction : function_body) {
        ++current;

        finishInactiveIntervals(current);
        activateIntervals(current);

        // If this is a call, save caller saved registers
        std::unordered_map<Register, StackOffset> caller_save_offsets;
        if (std::get_if<Call>(&instruction)) {
            for (auto& real_reg : allocatable_registers) {
                if (real_reg == REG32_ACCUM) continue;
                caller_save_offsets[real_reg] = getFreeStackOffset();
                new_instructions.emplace_back(
                    Mov(
                        EffectiveAddress(REG32_STACKBASEPTR, -1 * caller_save_offsets[real_reg]),
                        real_reg
                    )
                );
            }
        }

        // Replace abstract registers with allocated registers
        std::string original_string = instruction.toString();
        bool any_regs = false;
        for (auto active_interval : active_intervals) {
            if (Register* reg_ptr = std::get_if<Register>(&active_interval->assignment)) {
                instruction.replaceRegister(active_interval->original_register, *reg_ptr);
                any_regs = true;
            }
        }
        if (any_regs) instruction.tagWithComment("Reg allocated, original was " + original_string);

        // Replace abstract registers with instruction registers with loading/storing stack memory
        for (auto active_interval : active_intervals) {
            if (StackOffset* offset_ptr = std::get_if<StackOffset>(&active_interval->assignment)) {
                reg_offsets[active_interval->original_register] = *offset_ptr;
            }
        }
        replaceAbstracts(instruction, new_instructions);

        // If this is a call, restore currently used caller saved registers
        if (std::get_if<Call>(&instruction)) {
            for (auto& real_reg : allocatable_registers) {
                if (real_reg == REG32_ACCUM) continue;
                new_instructions.emplace_back(
                    Mov(
                        real_reg,
                        EffectiveAddress(REG32_STACKBASEPTR, -1 * caller_save_offsets[real_reg])
                    )
                );
                free_stack_spaces.insert(caller_save_offsets[real_reg]);
            }
        }
    }

    function_body = new_instructions;

    return (next_stack_offset - 4) / 4;
}
