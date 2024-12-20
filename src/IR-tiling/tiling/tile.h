#pragma once

#include <string>
#include <list>
#include <vector>
#include <variant>
#include <utility>

#include "IR-tiling/assembly/assembly-instruction.h"

class Tile;

using StatementTile = Tile*;
using ExpressionTile = std::pair<Tile*, std::string>;

using Instruction = std::variant<AssemblyInstruction, StatementTile, ExpressionTile>;

// A sequence of instructions that implements a subtree of the IR AST.
//
// Specified in terms of assembly instructions and other tiles.
class Tile {
    // The instructions that implement this tile
    std::vector<Instruction> instructions;

    friend class RegisterAllocator;

    // The cost of the tile
    int cost;
    // Whether the cost has already been calculated, and doesn't need to be recalculated
    bool cost_calculated;

    // The cost function for calculating cost of a tile
    void calculateCost();

  public:
    // Represents a placeholder abstract register within the assembly instructions
    static const inline std::string ABSTRACT_REG = "\%\%PLACEHOLDER_ABSTRACT_REG\%\%";

    // Replaces uses of Tile::ABSTRACT_REG with reg
    void assignAbstract(std::string reg);
    
    // Creates copy of tile with replaced uses of Tile::ABSTRACT_REG with reg
    Tile assignAbstractToCopy(std::string reg);

    // Produce a pair with the tile and the abstract reg it uses
    ExpressionTile pairWith(std::string abstract_reg);

    // Get cost of the tile
    int getCost();

    // Add instructions to the tile, which sets the cost to uncalculated
    void add_instruction(Instruction);
    void add_instructions_after(std::vector<Instruction>);
    void add_instructions_before(std::vector<Instruction>);

    // Get the full assembly instructions for this tile, expanding other tiles it uses
    std::list<AssemblyInstruction> getFullInstructions();

    // Construct tile with instructions
    Tile(std::vector<Instruction>);

    // Default construct max cost tile
    Tile();
};
