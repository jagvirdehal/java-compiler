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
    LinkedType(NonArrayLinkedType non_array_type) : is_array{false}, linked_type{non_array_type} {}
    LinkedType(TypeDeclaration type_declaration) {
        std::visit([&](auto type_dec){ this->linked_type = type_dec; }, type_declaration);
    }

    // Return whether the type stored is a primitive type rather than a class/interface link or array
    bool isPrimitive() { return bool(std::get_if<PrimitiveType>(&linked_type)) && !is_array; }
    // Return whether the type stored is an array or a class/interface
    bool isReferenceType() { return !isPrimitive(); }

    // Return whether this linked type is a subtype of another linked type, following Java spec rules.
    bool isSubType(LinkedType other, struct PackageDeclarationObject* default_package);

    // Returns a pointer to the ClassDeclarationObject that is contained in the NonArrayType,
    // if it is, and nullptr otherwise
    ClassDeclarationObject* getIfNonArrayIsClass();

    // Returns a pointer to the InterfaceDeclarationObject that is contained in the NonArrayType,
    // if it is, and nullptr otherwise
    InterfaceDeclarationObject* getIfNonArrayIsInterface();

    // Returns a pointer to the PrimitiveType that is contained in the NonArrayType,
    // if it is, and nullptr otherwise
    PrimitiveType* getIfNonArrayIsPrimitive();

    friend bool operator==(const LinkedType &lhs, const LinkedType &rhs) {
        return (lhs.linked_type == rhs.linked_type) && (lhs.is_array == rhs.is_array);
    }
};
