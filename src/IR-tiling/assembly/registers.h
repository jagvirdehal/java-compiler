#pragma once

namespace Assembly {
    // 8 bit general purpose registers
    static const inline std::string REG8L_ACCUM = "al";
    static const inline std::string REG8H_ACCUM = "ah";

    static const inline std::string REG8L_BASE = "bl";
    static const inline std::string REG8H_BASE = "bh";

    static const inline std::string REG8L_COUNTER = "cl";
    static const inline std::string REG8H_COUNTER = "ch";

    static const inline std::string REG8L_DATA = "dl";
    static const inline std::string REG8H_DATA = "dh";

    static const inline std::string REG8L_STACKPTR = "spl";
    static const inline std::string REG8L_STACKBASEPTR = "bpl";

    static const inline std::string REG8L_SOURCE = "sil";
    static const inline std::string REG8L_DEST = "dil";

    // 16 bit general purpose registers
    static const inline std::string REG16_ACCUM = "ax";
    static const inline std::string REG16_BASE = "bx";
    static const inline std::string REG16_COUNTER = "cx";
    static const inline std::string REG16_DATA = "dx";

    static const inline std::string REG16_STACKPTR = "sp";
    static const inline std::string REG16_STACKBASEPTR = "bp";

    static const inline std::string REG16_SOURCE = "si";
    static const inline std::string REG16_DEST = "di";

    // 32 bit general purpose registers
    static const inline std::string REG32_ACCUM = "eax";
    static const inline std::string REG32_BASE = "ebx";
    static const inline std::string REG32_COUNTER = "ecx";
    static const inline std::string REG32_DATA = "edx";

    static const inline std::string REG32_STACKPTR = "esp";
    static const inline std::string REG32_STACKBASEPTR = "ebp";

    static const inline std::string REG32_SOURCE = "esi";
    static const inline std::string REG32_DEST = "edi";

    // Instruction pointer register (not a gpr)
    static const inline std::string REG32_IP = "eip";

    // Return true if reg is a real gpr, and false if it is an abstract register
    static const inline std::unordered_set<std::string> assembly_ns_real_reg_set = {
        REG8L_ACCUM,
        REG8H_ACCUM,

        REG8L_BASE,
        REG8H_BASE,

        REG8L_COUNTER,
        REG8H_COUNTER,

        REG8L_DATA,
        REG8H_DATA,

        REG8L_STACKPTR,
        REG8L_STACKBASEPTR,

        REG8L_SOURCE,
        REG8L_DEST,

        REG16_ACCUM,
        REG16_BASE,
        REG16_COUNTER,
        REG16_DATA,

        REG16_STACKPTR,
        REG16_STACKBASEPTR,

        REG16_SOURCE,
        REG16_DEST,

        REG32_ACCUM,
        REG32_BASE,
        REG32_COUNTER,
        REG32_DATA,

        REG32_STACKPTR,
        REG32_STACKBASEPTR,

        REG32_SOURCE,
        REG32_DEST
    };
    bool inline isRealRegister(std::string reg) {
        return assembly_ns_real_reg_set.count(reg) ? true : false;
    }

    // Return all the registers that overlap with reg (i.e. al is the low part of eax, so both are the same register set)
    static const inline std::unordered_set<std::string> accum_set 
        = {REG8L_ACCUM, REG8H_ACCUM, REG16_ACCUM, REG32_ACCUM};

    static const inline std::unordered_set<std::string> base_set
        = {REG8L_BASE, REG8H_BASE, REG16_BASE, REG32_BASE};
    
    static const inline std::unordered_set<std::string> counter_set
        = {REG8L_COUNTER, REG8H_COUNTER, REG16_COUNTER, REG32_COUNTER};

    static const inline std::unordered_set<std::string> data_set
        = {REG8L_DATA, REG8H_DATA, REG16_DATA, REG32_DATA};
    
    static const inline std::unordered_set<std::string> source_set
        = {REG8L_SOURCE, REG16_SOURCE, REG32_SOURCE};

    static const inline std::unordered_set<std::string> dest_set
        = {REG8L_DEST, REG16_DEST, REG32_DEST};

    static const inline std::unordered_set<std::string> stack_ptr_set
        = {REG8L_STACKPTR, REG16_STACKPTR, REG32_STACKPTR};

    static const inline std::unordered_set<std::string> stack_base_ptr_set
        = {REG8L_STACKBASEPTR, REG16_STACKBASEPTR, REG32_STACKBASEPTR};

    std::unordered_set<std::string> inline sameRegisterSet(std::string reg) {
        if (accum_set.count(reg)) return accum_set;
        if (base_set.count(reg)) return base_set;
        if (counter_set.count(reg)) return counter_set;
        if (data_set.count(reg)) return data_set;
        if (source_set.count(reg)) return source_set;
        if (dest_set.count(reg)) return dest_set;
        if (stack_ptr_set.count(reg)) return stack_ptr_set;
        if (stack_base_ptr_set.count(reg)) return stack_base_ptr_set;
        return {reg};
    }

}; // namespace Assembly
