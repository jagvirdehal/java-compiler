#pragma once
#include <variant>
#include "type-decl/type_declaration.h"
#include "variant-ast/primitivetype.h"

using NonArrayLinkedType 
    = std::variant<PrimitiveType, struct ClassDeclarationObject*, struct InterfaceDeclarationObject*, std::nullptr_t>;

struct LinkedType {
    bool is_array;
    NonArrayLinkedType linked_type;

    LinkedType() : is_array{false}, linked_type{nullptr} {}
    LinkedType(TypeDeclaration type_declaration) {
        std::visit([&](auto type_dec){ this->linked_type = type_dec; }, type_declaration);
    }
    // Return whether the type stored is a primitive type rather than a class/interface link
    bool isPrimitive() {
        return bool(std::get_if<PrimitiveType>(&linked_type));
    }
    
    // Returns whether the type stored is INT, SHORT, BYTE, or CHAR
    bool isNumeric() {
        if(isPrimitive()) {
            auto typeEnum = std::get<PrimitiveType>(linked_type);
            if(typeEnum == PrimitiveType::INT || typeEnum == PrimitiveType::SHORT || typeEnum == PrimitiveType::BYTE || 
                typeEnum == PrimitiveType::CHAR) {
                return true;
            }
        }
        return false;
    }

    // Returns whether the type stored is BOOLEAN
    bool isBoolean() {
        if(isPrimitive()) {
            auto typeEnum = std::get<PrimitiveType>(linked_type);
            if(typeEnum == PrimitiveType::BOOLEAN) {
                return true;
            }
        }
        return false;
    }

    // Returns whether the type stored is VOID
    bool isVoid() {
        if(isPrimitive()) {
            auto typeEnum = std::get<PrimitiveType>(linked_type);
            if(typeEnum == PrimitiveType::VOID) {
                return true;
            }
        }
        return false;
    }

    bool operator==(const LinkedType& other) {
        if(this->is_array != other.is_array) {
            return false;
        }
        if(this->linked_type.index() != other.linked_type.index()) {
            return false;
        }
        switch(this->linked_type.index()) {
            case 0: // PrimitiveType
                return std::get<PrimitiveType>(this->linked_type) == std::get<PrimitiveType>(other.linked_type);
            case 1: // ClassDeclarationObject*
                return std::get<ClassDeclarationObject*>(this->linked_type) == std::get<ClassDeclarationObject*>(other.linked_type);
            case 2: // InterfaceDeclarationObject*
                return std::get<InterfaceDeclarationObject*>(this->linked_type) == std::get<InterfaceDeclarationObject*>(other.linked_type);
            case 3: // std::nullptr_t
                return true;
        }
        return true;
    }

    bool isNull() {
        return this->linked_type.index() == 3;
    }

    // Returns whether the type stored is STRING
    bool isString() {}
};
