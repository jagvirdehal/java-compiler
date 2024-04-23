#pragma once

#include <string>
#include "assembly-common.h"

// File that contains the classes for each used x86 assembly instruction.
//
// Each added instruction should be added to the variant in assembly-instruction.h.
// Each added instruction should implement toString(), and define which operands are read/written to (can be both)

namespace AssemblyRefactor {

/* Assorted instructions */

struct Mov : public AssemblyCommon {
    Mov(Operand dest, Operand src) {
        useOperands(dest.write(), src.read());
    }

    std::string toString() {
        return "mov " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Jump : public AssemblyCommon {
    Jump(Operand target) {
        useOperands(target.read());
    }

    std::string toString() {
        return "jmp " + getOp(1).toString();
    }
};

struct Je : public AssemblyCommon {
    Je(Operand target) {
        useOperands(target.read());
    }

    std::string toString() {
        return "je " + getOp(1).toString();
    }
};

struct JumpIfNZ : public AssemblyCommon {
    JumpIfNZ(Operand target) {
        useOperands(target.read());
    }

    std::string toString() {
        return "jnz " + getOp(1).toString();
    }
};

struct Lea : public AssemblyCommon {
    Lea(Operand dest, Operand src) {
        useOperands(dest.write(), src.read());

        if (!std::get_if<EffectiveAddress>(&src)) {
            THROW_CompilerError("Lea source must be effective address!");
        }
    }

    std::string toString() {
        return "lea " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Add : public AssemblyCommon {
    Add(Operand arg1, Operand arg2) {
        useOperands(arg1.readwrite(), arg2.read());
    }

    std::string toString() {
        return "add " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Sub : public AssemblyCommon {
    Sub(Operand arg1, Operand arg2) {
        useOperands(arg1.readwrite(), arg2.read());
    }

    std::string toString() {
        return "sub " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Xor : public AssemblyCommon {
    Xor(Operand arg1, Operand arg2) {
        useOperands(arg1.readwrite(), arg2.read());
    }

    std::string toString() {
        return "xor " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct And : public AssemblyCommon {
    And(Operand arg1, Operand arg2) {
        useOperands(arg1.readwrite(), arg2.read());
    }

    std::string toString() {
        return "and " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Or : public AssemblyCommon {
    Or(Operand arg1, Operand arg2) {
        useOperands(arg1.readwrite(), arg2.read());
    }

    std::string toString() {
        return "or " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct MovZX : public AssemblyCommon {
    MovZX(Operand arg1, Operand arg2) {
        useOperands(arg1.write(), arg2.read());
    }

    std::string toString() {
        return "movzx " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Cmp : public AssemblyCommon {
    Cmp(Operand arg1, Operand arg2) {
        useOperands(arg1.read(), arg2.read());
    }

    std::string toString() {
        return "cmp " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Test : public AssemblyCommon {
    Test(Operand arg1, Operand arg2) {
        useOperands(arg1.read(), arg2.read());
    }

    std::string toString() {
        return "test " + getOp(1).toString() + ", " + getOp(2).toString();
    }
};

struct Push : public AssemblyCommon {
    Push(Operand arg) {
        useOperands(arg.read());
    }

    std::string toString() {
        return "push " + getOp(1).toString();
    }
};

struct Pop : public AssemblyCommon {
    Pop(Operand arg) {
        useOperands(arg.write());
    }

    std::string toString() {
        return "pop " + getOp(1).toString();
    }
};

/* Instructions without operands */

struct NoOperandInstruction : public AssemblyCommon {
    std::string static_instruction;

    NoOperandInstruction(std::string static_instruction) : static_instruction{static_instruction} {}

    std::string toString() {
        return static_instruction;
    }
};

struct Cdq : public NoOperandInstruction {
    Cdq() : NoOperandInstruction{"cdq"} {}
};

struct Ret : public NoOperandInstruction {
    Ret(unsigned int bytes = 0) 
        : NoOperandInstruction{bytes > 0 ? "ret " + std::to_string(bytes) : "ret"} 
    {}
};

struct Call : public NoOperandInstruction {
    Call(std::string static_label) 
        : NoOperandInstruction{"call " + static_label} 
    {}
};

struct SysCall : public NoOperandInstruction {
    SysCall() 
        : NoOperandInstruction{"int 0x80"} 
    {}
};

struct Comment : public NoOperandInstruction {
    Comment(std::string text) 
        : NoOperandInstruction{"; " + text} 
    {}
};

struct Label : public NoOperandInstruction {
    Label(std::string label) 
        : NoOperandInstruction{label + ":"} 
    {}
};

struct GlobalSymbol : public NoOperandInstruction {
    GlobalSymbol(std::string symbol) 
        : NoOperandInstruction{"global " + symbol} 
    {}
};

struct ExternSymbol : public NoOperandInstruction {
    ExternSymbol(std::string symbol) 
        : NoOperandInstruction{"extern " + symbol} 
    {}
};

struct LineBreak : public NoOperandInstruction {
    LineBreak() 
        : NoOperandInstruction{""} 
    {}
};

/* SetX instructions which set destination to 1 or 0 based on flags from Cmp */

struct BoolSetInstruction : public AssemblyCommon {
    std::string instruction_name;

    BoolSetInstruction(std::string name, Operand dest) 
        : instruction_name{name}
    {
        useOperands(dest.write());
    }

    std::string toString() {
        return instruction_name + " " + getOp(1).toString();
    }
};

struct SetZ : public BoolSetInstruction {
    SetZ(Operand dest) : BoolSetInstruction{"setz", dest} {}
};

struct SetNZ : public BoolSetInstruction {
    SetNZ(Operand dest) : BoolSetInstruction{"setnz", dest} {}
};

struct SetL : public BoolSetInstruction {
    SetL(Operand dest) : BoolSetInstruction{"setl", dest} {}
};

struct SetG : public BoolSetInstruction {
    SetG(Operand dest) : BoolSetInstruction{"setg", dest} {}
};

struct SetLE : public BoolSetInstruction {
    SetLE(Operand dest) : BoolSetInstruction{"setle", dest} {}
};

struct SetGE : public BoolSetInstruction {
    SetGE(Operand dest) : BoolSetInstruction{"setge", dest} {}
};

/* IMul/IDiv */

struct IMul : public AssemblyCommon {
    IMul(Operand multiplicand) {
        useOperands(multiplicand.read());
    }

    std::string toString() {
        return "imul " + getOp(1).toString();
    }
};

struct IDiv : public AssemblyCommon {
    IDiv(Operand divisor) {
        useOperands(divisor.read());
    }

    std::string toString() {
        return "idiv " + getOp(1).toString();
    }
};

}; // namespace Assembly
