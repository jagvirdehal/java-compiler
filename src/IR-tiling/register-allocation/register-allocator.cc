#include <unordered_map>
#include <string>

#include "register-allocator.h"
#include "IR-tiling/assembly/assembly-instruction.h"
#include "IR-tiling/assembly/registers.h"

using namespace Assembly;

// Helper for asserting temporaries are not used without values being set
void RegisterAllocator::checkAllTemporariesInitialized(std::list<AssemblyInstruction>& function_body) {
    std::unordered_set<std::string> initialized;

    for (auto& instr : function_body) {
        for (auto& reg : instr.getReadRegisters()) {
            if (!isRealRegister(reg) && !initialized.count(reg)) {
                THROW_CompilerError("Temporary " + reg + " was never initialized!");
            }
        }

        for (auto& reg : instr.getWriteRegisters()) {
            if (!isRealRegister(reg) && !initialized.count(reg)) {
                // Temporary is set to a value as of this instruction
                initialized.insert(reg);
            }
        }
    }
}