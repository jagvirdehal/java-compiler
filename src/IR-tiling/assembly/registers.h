#pragma once

namespace AssemblyRefactor {
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
    bool inline isRealRegister(std::string reg) {
        std::unordered_set<std::string> set;

        set.emplace(REG8L_ACCUM);
        set.emplace(REG8H_ACCUM);

        set.emplace(REG8L_BASE);
        set.emplace(REG8H_BASE);

        set.emplace(REG8L_COUNTER);
        set.emplace(REG8H_COUNTER);

        set.emplace(REG8L_DATA);
        set.emplace(REG8H_DATA);

        set.emplace(REG8L_STACKPTR);
        set.emplace(REG8L_STACKBASEPTR);

        set.emplace(REG8L_SOURCE);
        set.emplace(REG8L_DEST);

        set.emplace(REG16_ACCUM);
        set.emplace(REG16_BASE);
        set.emplace(REG16_COUNTER);
        set.emplace(REG16_DATA);

        set.emplace(REG16_STACKPTR);
        set.emplace(REG16_STACKBASEPTR);

        set.emplace(REG16_SOURCE);
        set.emplace(REG16_DEST);

        set.emplace(REG32_ACCUM);
        set.emplace(REG32_BASE);
        set.emplace(REG32_COUNTER);
        set.emplace(REG32_DATA);

        set.emplace(REG32_STACKPTR);
        set.emplace(REG32_STACKBASEPTR);

        set.emplace(REG32_SOURCE);
        set.emplace(REG32_DEST);

        return set.count(reg) ? true : false;
    }

}; // namespace Assembly
