#pragma once

#include <unordered_map>
#include <string>

#include "environment-builder/symboltableentry.h"
#include "variant-ast/classes.h"

// Conventions that need to be the same across different code gen components
class CGConstants {
    static size_t next_method_id;
    static std::unordered_map<MethodDeclarationObject*, std::string> method_labels;

    static size_t next_field_id;
    static std::unordered_map<FieldDeclarationObject*, std::string> field_labels;

    static size_t next_variable_id;
    static std::unordered_map<LocalVariableDeclarationObject*, std::string> variable_labels;

    static size_t next_parameter_id;
    static std::unordered_map<FormalParameterDeclarationObject*, std::string> parameter_labels;

    static inline size_t next_class_id;
    static inline std::unordered_map<ClassDeclarationObject*, std::string> class_labels;

    // Unified way of generating unique labels for objects
    template <typename declObj>
    static std::string generateUniqueLabel(
      declObj obj,
      const std::string &actual_name, 
      const std::string &prefix,
      size_t &id_counter, 
      std::unordered_map<declObj, std::string> &labels
    ) {
        if (!labels.count(obj)) {
            std::string label = global_data_prefix + prefix + "_ID" + std::to_string(id_counter++) + "__#" + actual_name;
            labels[obj] = label;
        }
        return labels[obj];
    }

  public:
    // String global data is prepended with, that non global data cannot start with
    const static inline std::string global_data_prefix = "_#";

    // Produce a label for the declaration object, that is guaranteed to not conflict with any other object.
    //
    // Returns the same label for the same object every time.
    static std::string uniqueMethodLabel(MethodDeclarationObject* method) {
        if (method->ast_reference->hasModifier(Modifier::NATIVE)) {
            return "NATIVE" + method->full_qualified_name;
        }
        return generateUniqueLabel(method, method->full_qualified_name, "_METHOD", next_method_id, method_labels);
    };

    static std::string uniqueStaticMethodLabel(MethodDeclarationObject* method) {
        if (method->ast_reference->hasModifier(Modifier::NATIVE)) {
            return "NATIVE" + method->full_qualified_name;
        }
        return generateUniqueLabel(method, method->full_qualified_name, "_STATIC_METHOD", next_method_id, method_labels);
    };

    static std::string uniqueFieldLabel(FieldDeclarationObject* field) {
        return generateUniqueLabel(field, field->full_qualified_name, "_FIELD", next_field_id, field_labels);
    };

    static std::string uniqueStaticFieldLabel(FieldDeclarationObject* field) {
        return generateUniqueLabel(field, field->full_qualified_name, "_STATIC_FIELD", next_field_id, field_labels);
    };

    static std::string uniqueLocalVariableLabel(LocalVariableDeclarationObject* variable) {
        return generateUniqueLabel(variable, variable->identifier, "_LOCAL_VARIABLE", next_variable_id, variable_labels);
    };

    static std::string uniqueParameterLabel(FormalParameterDeclarationObject* parameter) {
        return generateUniqueLabel(parameter, parameter->identifier, "_PARAMETER", next_parameter_id, parameter_labels);
    };

    static std::string uniqueClassLabel(ClassDeclarationObject* class_obj) {
        return generateUniqueLabel(class_obj, class_obj->identifier, "_CLASS", next_class_id, class_labels);
    }

    // The prefix for abstract argument registers
    // Abstract argument registers are _ARG0, _ARG1, etc.
    const static inline std::string ABSTRACT_ARG_PREFIX = "_ARG";

    // The abstract return register
    // Placing values in this register returns it to the caller.
    const static inline std::string ABSTRACT_RET = "_RET";
};
