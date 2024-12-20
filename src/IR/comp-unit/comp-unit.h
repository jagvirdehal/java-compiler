#pragma once

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "IR/ir_variant.h"
#include "IR/func-decl/func-decl.h"
#include "exceptions/exceptions.h"

class CompUnitIR {
    std::string name;
    std::string class_name; // Used for graphing

    std::unordered_map<std::string, FuncDeclIR*> functions;     // These point to functions inside of child_functions
    std::vector<std::unique_ptr<FuncDeclIR>> child_functions;

    std::unordered_map<std::string, ExpressionIR*> static_fields;
    std::vector<std::pair<std::string, std::unique_ptr<ExpressionIR>>> child_static_fields;

    bool staticFieldsCanonicalized = false;

    std::vector<std::pair<std::string, std::unique_ptr<StatementIR>>> child_canonical_static_fields;

  public:
    std::vector<std::unique_ptr<StatementIR>> start_statements;
    CompUnitIR(std::string name) : name(name) {}

    void appendFunc(std::string name, std::unique_ptr<FuncDeclIR> func) {
        child_functions.emplace_back(std::move(func));
        functions[name] = child_functions.back().get();
    }

    void appendField(std::string name, std::unique_ptr<ExpressionIR> value) {
        child_static_fields.emplace_back(std::make_pair(name, std::move(value)));
        static_fields[name] = child_static_fields.back().second.get();
    }

    void appendStartStatement(std::unique_ptr<StatementIR> stmt) {
        start_statements.emplace_back(std::move(stmt));
    }

    FuncDeclIR* getFunc(std::string name) { 
        if (functions.find(name) == functions.end()) {
            THROW_ASTtoIRError("Could not find function with name " + name + " in the IR."); 
        }
        return functions[name];
    }

    std::string getName() { return name; }
    std::unordered_map<std::string, FuncDeclIR*> getFunctions() { return functions; }
    std::unordered_map<std::string, ExpressionIR*> getFields() { return static_fields; }
    std::string label() { return "COMPUNIT (" + class_name + ")"; }

    void setClassName(std::string name) { class_name = name; }
    std::vector<std::unique_ptr<FuncDeclIR>>& getFunctionList() { return child_functions; }

    std::vector<std::pair<std::string, std::unique_ptr<ExpressionIR>>>& getFieldList() { 
        if (staticFieldsCanonicalized) THROW_CompilerError("Static field initalizers are canonicalized; use getCanonFieldList");
        return child_static_fields; 
    }

    std::vector<std::pair<std::string, std::unique_ptr<StatementIR>>>& getCanonFieldList() { 
        if (!staticFieldsCanonicalized) THROW_CompilerError("Static field initalizers are not canonicalized; use getFieldList");
        return child_canonical_static_fields; 
    }

    bool areStaticFieldsCanonicalized() { return staticFieldsCanonicalized; }

    void canonicalizeStaticFields(std::vector<std::pair<std::string, std::unique_ptr<StatementIR>>>& child_canonical_static_fields) {
        this->child_canonical_static_fields = std::move(child_canonical_static_fields);
        staticFieldsCanonicalized = true;
    }
};
