#include <gtest/gtest.h>

#include "IR-tiling/assembly/assembly.h"
#include "IR-tiling/assembly/assembly-instruction.h"

#include "IR-tiling/register-allocation/brainless-allocator.h"
#include "IR-tiling/register-allocation/linear-scanning-allocator.h"

// Test that the assembly representation works

TEST(Mov, StringifiesCorrectly) {
    using namespace Assembly;

    Operand a = "a";
    AssemblyInstruction move_basic = Mov("a", "b");

    EXPECT_EQ(move_basic.toString(), "mov a, b") << "Basic move did not stringify correctly";

    AssemblyInstruction move_effective_address1 = Mov("a", EffectiveAddress("eax", 2));

    EXPECT_EQ(move_effective_address1.toString(), "mov a, [eax + 2]") << "Advanced move 1 did not stringify correctly";
}

TEST(Lea, registerusagecorrect) {
    using namespace Assembly;

    AssemblyInstruction lea = Lea("eax", EffectiveAddress("%REGABS1%"));

    EXPECT_EQ(lea.toString(), "lea eax, [%REGABS1%]") << "Basic lea did not stringify correctly";

    auto used = lea.getUsedRegisters();

    EXPECT_TRUE(used.count("eax") == 1) << "used does not contain eax";
    EXPECT_TRUE(used.count("%REGABS1%") == 1) << "used does not contain %REGABS1%";

    auto read = lea.getReadRegisters();

    std::string readstr;

    for (auto reg : read) readstr += " " + reg;

    EXPECT_TRUE(read.count("eax") == 0) << "read contains eax; specifically it contains" << readstr;
    EXPECT_TRUE(read.count("%REGABS1%") == 1) << "read does not contain %REGABS1%";

    auto write = lea.getWriteRegisters();

    EXPECT_TRUE(write.count("eax") == 1) << "write does not contain eax";
    EXPECT_TRUE(write.count("%REGABS1%") == 0) << "write contains %REGABS1%";

    EXPECT_TRUE(used.size() == 2) << "used amount incorrect";

    lea.replaceRegister("eax", "a");
    EXPECT_EQ(lea.toString(), "lea a, [%REGABS1%]") << "replaced lea did not stringify correctly";

    lea.replaceRegister("%REGABS1%", "b");
    EXPECT_EQ(lea.toString(), "lea a, [b]") << "replaced lea did not stringify correctly";
}

TEST(BrainlessRegisterAllocator, allocatescorrectly) {
    using namespace Assembly;

    BrainlessRegisterAllocator allocator;

    std::list<AssemblyInstruction> instrs = {
        Mov("%REGABS1%", 123),
        Lea("eax", EffectiveAddress("%REGABS1%"))
    };

    allocator.allocateRegisters(instrs);

    for (auto &instr : instrs) {
        std::cout << instr.toString() << "\n";
    }
}


TEST(LinearScanningRegisterAllocator, allocatescorrectly) {
    using namespace Assembly;

    LinearScanningRegisterAllocator allocator;

    std::list<AssemblyInstruction> instrs = {
        SetL(REG8L_ACCUM),
        Mov("a", 2),
        Mov(REG32_ACCUM, "a"),
        Mov(REG32_COUNTER, REG32_ACCUM)
    };

    allocator.allocateRegisters(instrs);

    for (auto &instr : instrs) {
        std::cout << instr.toString() << "\n";
    }
}