#pragma once

#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <filesystem>

#include "IR/ir.h"
#include "IR/ir_visitor.h"
#include "utillities/overload.h"
#include "IR-tiling/tiling/ir-tiling.h"

#include "IR-tiling/register-allocation/brainless-allocator.h"
#include "IR-tiling/register-allocation/noop-allocator.h"

#include "IR-tiling/assembly/assembly.h"
#include "IR-tiling/assembly/registers.h"

#define USED_REG_ALLOCATOR BrainlessRegisterAllocator
// #define USED_REG_ALLOCATOR NoopRegisterAllocator

using namespace Assembly;

// Find methods/static fields from other compilation units that need to be linked to the assembly file for this cu
class DependencyFinder : public IRSkipVisitor {
    std::unordered_set<std::string> required_static_fields;
    std::unordered_set<std::string> required_functions;

    CompUnitIR& cu;

  public:
    using IRSkipVisitor::operator();

    explicit DependencyFinder(CompUnitIR& cu) : cu{cu} {
        this->operator()(cu);
        required_functions.insert("__exception");
        required_functions.insert("__malloc");
    }

    virtual void operator()(CallIR &node) override {
        if (auto name = std::get_if<NameIR>(&node.getTarget())) {
            std::string called_function = name->getName();
            if (!(cu.getFunctions().count(called_function) || required_functions.count(called_function))) {
                // Required function is not from this compilation unit already or already included
                required_functions.insert(called_function);
            }
            this->visit_children(node);
            return;
        }
        THROW_CompilerError("Function call target is not a label");
    }

    virtual void operator()(TempIR &node) override {
        if (!required_static_fields.count(node.getName()) && node.isGlobal) {
            // Required static field is not already included
            required_static_fields.insert(node.getName());
        }
        this->visit_children(node); 
    }

    std::unordered_set<std::string> getRequiredStaticFields() { return required_static_fields; }
    std::unordered_set<std::string> getRequiredFunctions() { return required_functions; }
};

// Class in charge of generating all the output assembly for each compilation unit.
//
// Tiles each compilation unit and creates the files as appropriate.
class AssemblyGenerator {
    IRToTilesConverter converter;

    std::string makeFunctionPrologue(int32_t stack_size) {
        std::string output;
        output += "\t" + Push(REG32_STACKBASEPTR).toString() + "\n";
        output += "\t" + Mov(REG32_STACKBASEPTR, REG32_STACKPTR).toString() + "\n";
        output += "\t" + Sub(REG32_STACKPTR, 4 * stack_size).toString() + "\n";
        return output;
    }

  public:
    void generateCode(std::vector<IR>& ir_trees, std::string entrypoint_method) {
        std::vector<std::pair<std::string, std::list<AssemblyInstruction>>> static_fields;
        
        // Reset output directory
        std::filesystem::create_directory("output");
        for (auto& path: std::filesystem::directory_iterator("output")) {
            std::filesystem::remove_all(path);
        }

        // Emit a file for each compilation unit
        size_t file_id = 0;
        for (auto &ir : ir_trees) {
            std::visit(util::overload {
                [&](CompUnitIR& cu) {
                    // Get static fields to dump in .data section of entrypoint file later
                    for (auto& [field_name, field_initalizer] : cu.getCanonFieldList()) {
                        assert(field_initalizer);

                        auto tile = converter.tile(*field_initalizer);
                        auto instructions = tile->getFullInstructions();

                        static_fields.emplace_back(field_name, std::move(instructions));
                    }

                    // Generate the assembly file for the compilation unit
                    std::ofstream output_file {"output/cu_" + std::to_string(file_id++) + ".s"};
                    output_file << "section .text\n\n";

                    // Export functions as global
                    for (auto& func : cu.getFunctionList()) {
                        output_file << GlobalSymbol(func->getName()).toString() << "\n";
                    }
                    output_file << "\n";

                    // Import required functions/static fields
                    auto dependency_finder = DependencyFinder(cu);
                    for (auto& req_func : dependency_finder.getRequiredFunctions()) {
                        output_file << ExternSymbol(req_func).toString() << "\n";
                    }
                    for (auto& req_field : dependency_finder.getRequiredStaticFields()) {
                        output_file << ExternSymbol(req_field).toString() << "\n";
                    }
                    output_file << "\n";

                    // Tile each method in the compilation unit
                    for (auto& func : cu.getFunctionList()) {
                        // Tile and allocate registers
                        StatementTile body_tile = converter.tile(func->getBody());
                        auto body_instructions = body_tile->getFullInstructions();
                        int32_t stack_size = USED_REG_ALLOCATOR().allocateRegisters(body_instructions);

                        // Function label
                        output_file << Label(func->getName()).toString() << "\n";

                        // Function prologue
                        output_file << makeFunctionPrologue(stack_size);

                        // Function body
                        for (auto& body_instruction : body_instructions) {
                            output_file << "\t" << body_instruction.toString() << "\n";
                        }
                        output_file << "\n";
                    }
                },
                [&](auto&) { THROW_CompilerError("shouldn't happen"); }
            }, ir);
        }

        // Emit a main file for the entrypoint
        std::ofstream start_file {"output/main.s"};
        
        start_file << "section .data\n\n";
        for (auto& [field_name, initalizer_tile] : static_fields) {
            start_file << field_name + ": dd 0" << "\n";
        }
        start_file << "\n"

        << "section .text"                              << "\n\n";

        for (auto& [field_name, initalizer_tile] : static_fields) {
            start_file << GlobalSymbol(field_name).toString() << "\n";
        }
        start_file 
        << GlobalSymbol("_start").toString()             << "\n"
        << ExternSymbol("__exception").toString()        << "\n"
        << ExternSymbol("__malloc").toString()           << "\n"
        << ExternSymbol(entrypoint_method).toString()    << "\n\n"

        << Label("_start").toString() << "\n"
            << "\t" << Comment("Initialize all the static fields of all the compilation units, in order").toString() << "\n";

            // Combine into one list of instructions
            std::list<AssemblyInstruction> static_init;
            for (auto& [field_name, initializer_instructions] : static_fields) {
                for (auto& instr : initializer_instructions) {
                    static_init.push_back(instr);
                }
            }

            // Initialize all, with the same stack frame
            int32_t stack_size_for_initializer = USED_REG_ALLOCATOR().allocateRegisters(static_init);

            start_file << makeFunctionPrologue(stack_size_for_initializer);
            for (auto &instr : static_init) {
                start_file << "\t" << instr.toString() << "\n";
            }

            start_file << "\t" << Mov(REG32_STACKPTR, REG32_STACKBASEPTR).toString() << "\n";
            start_file << "\t" << Pop(REG32_STACKBASEPTR).toString() << "\n";
            start_file << "\n"

            << "\t" << Comment("Call entrypoint method and execute exit() system call with return value in REG32_BASE").toString() << "\n"
            << "\t" << Call(entrypoint_method).toString()        << "\n"
            << "\t" << Mov(REG32_BASE, REG32_ACCUM).toString()   << "\n"
            << "\t" << Mov(REG32_ACCUM, 1).toString()            << "\n"
            << "\t" << SysCall().toString()                      << "\n"
        ;
    }
};
