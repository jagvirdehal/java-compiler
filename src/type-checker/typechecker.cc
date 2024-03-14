#include "type-checker/typechecker.h"
#include "utillities/util.h"
#include "exceptions/exceptions.h"
#include "environment-builder/symboltable.h"

LinkedType TypeChecker::getLink(Expression &node) {
    return std::visit(util::overload {
        // Literal handled seperately as it is not a class
        [&] (Literal expr_type) -> LinkedType {
            return std::visit(util::overload {
                [&] (int64_t literal_type) -> LinkedType {
                    return LinkedType(PrimitiveType::INT);
                },
                [&] (bool literal_type) -> LinkedType {
                    return LinkedType(PrimitiveType::BOOLEAN);
                },
                [&] (char literal_type) -> LinkedType {
                    return LinkedType(PrimitiveType::CHAR);
                },
                [&] (std::string& literal_type) -> LinkedType {
                    NonArrayLinkedType string_type = default_package->findClassDeclaration("java.lang.String");
                    return LinkedType(string_type);
                },
                [&] (std::nullptr_t literal_type) -> LinkedType {
                    return LinkedType(PrimitiveType::NULL_T);
                },
            }, expr_type);
        },
        // Any non-literal subexpression
        [] (auto& expr_type) -> LinkedType {
            return expr_type.link;
        }
    }, node);
}

LinkedType TypeChecker::getLink(std::unique_ptr<Expression>& node_ptr) {
    return getLink(*node_ptr);
}

ClassDeclarationObject* TypeChecker::getStringClass(LinkedType &link) {
    if (link.getIfNonArrayIsClass() == default_package->findClassDeclaration("java.lang.String")) {
        return link.getIfNonArrayIsClass();
    }
    return nullptr;
}

void TypeChecker::operator()(CompilationUnit &node) {
    this->compilation_unit_namespace = node.cu_namespace;
    // Skip package declaration and imports
    for (auto &child : node.class_declarations) {
        this->operator()(child);
    }
    for (auto &child : node.interface_declarations) {
        this->operator()(child);
    }
}

void TypeChecker::operator()(ClassDeclaration &node) {
    this->current_class = node.environment;
    visit_children(node);
}

void TypeChecker::operator()(MethodDeclaration &node) {
    current_method = node.environment;
    current_method->scope_manager.closeAllScopes();
    visit_children(node);
    current_method->scope_manager.closeAllScopes();
    current_method = nullptr;
}

void TypeChecker::operator()(Block &node) {
    current_method->scope_manager.openScope(node.scope_id);
    visit_children(node);
    current_method->scope_manager.closeScope(node.scope_id);
}

void TypeChecker::operator()(LocalVariableDeclaration &node) {
    current_method->scope_manager.declareVariable(node.variable_declarator->variable_name->name);
    visit_children(node);
}

// TODO : It's probably nicer to refactor checkIfFieldIsAccessible and checkIfMethodIsAccessible
// so they work the same way.

// Return whether the class_with_field has a field with simple name field_simple_name
// that is accessible by current_class
//
// Returns pointer to FieldDeclarationObject if accessible, and nullptr otherwise
FieldDeclarationObject* checkIfFieldIsAccessible(
    ClassDeclarationObject* current_class,
    ClassDeclarationObject* class_with_field,
    std::string& field_simple_name,
    bool must_be_static = false
) {
    // First, see if the field even exists
    if (!class_with_field) { return nullptr; }
    auto possible_field = class_with_field->accessible_fields[field_simple_name];
    if (!possible_field) { return nullptr; }

    // Second, if the field must be static, ensure it is static
    if (must_be_static && !possible_field->ast_reference->hasModifier(Modifier::STATIC)) {
        return nullptr;
    }

    // Third, if the field is protected, the current_class must be a subclass or in the same package of class declaring it
    ClassDeclarationObject* class_that_declared_field = possible_field->containing_class;
    if (possible_field->ast_reference->hasModifier(Modifier::PROTECTED)) {
        if (
            !current_class->isSubType(class_that_declared_field) && 
            !(current_class->package_contained_in == class_that_declared_field->package_contained_in)
        ) {
            std::cerr << "Protected field not accessible\n";
            return nullptr;
        }
    }

    return possible_field;
}

// Return true iff method_to_access is accessible when called on an object of type
// type_method_called_on within current_class.
//
// This function assumes the method already exists on the object it's called on.
bool TypeChecker::checkifMethodIsAccessible(
    MethodDeclarationObject* method_to_access,
    LinkedType type_method_called_on
) {
    if (method_to_access->ast_reference->hasModifier(Modifier::PROTECTED)) {
        // JLS 6.6.2
        PackageDeclarationObject* current_package = compilation_unit_namespace.getCurrentPackage();
        // If the access to a protected member is in the same package as the declaration, it is accessible
        if (current_package == method_to_access->containing_type->package_contained_in) {
            return true;
        }
        // JLS: Let C be the class in which a protected member is declared
        // Access is permitted within the body of a subclass S of C,
        // with Q.id, iff the type of Q is S or a subclass of S

        // S = current_class
        // C = method_to_access->containing_type
        // Q = type_method_called_on
        NonArrayLinkedType non_array_curr_class = current_class;
        auto current_class_as_linked_type = LinkedType(non_array_curr_class);

        // S is a subclass of C
        // Class in which the method is being called in must be subclass of declaring class
        if (!current_class_as_linked_type.isSubType(LinkedType(method_to_access->containing_type), default_package)) {
            return false;
        }

        // Q is a subclass of S
        // Class of the object the method is called on must be subclass of class the method
        // is called in
        if (!type_method_called_on.isSubType(current_class_as_linked_type, default_package)) {
            return false;
        }

        return true;
    } else {
        // Method is public
        return true;
    }
}

void TypeChecker::operator()(QualifiedIdentifier &qid) {
    switch (qid.getClassification()) {
        case EXPRESSION_NAME: 
            if (qid.isSimple()) {
                std::string& name = qid.identifiers.back().name;
                // 1. Look up in local vars scope
                if (current_method) {
                    auto possible_var = current_method->scope_manager.lookupDeclaredVariable(name);
                    if (possible_var) {
                        qid.link = possible_var->type;
                        return;
                    }
                }
                // 2. Look up in parameters
                if (current_method) {
                    auto possible_param 
                        = current_method->parameters->lookupUniqueSymbol<FormalParameterDeclarationObject>(name);
                    if (possible_param) {
                        qid.link = possible_param->type;
                        return;
                    }
                }
                // 3. Look up in fields
                auto decl_type = compilation_unit_namespace.getDeclaredType();
                if (auto cls = std::get_if<ClassDeclarationObject*>(&decl_type)) {
                    if (auto possible_field = checkIfFieldIsAccessible(current_class, *cls, name)) {
                        qid.link = possible_field->type;
                        return;
                    }
                }
                THROW_TypeCheckerError("Undefined reference to " + qid.getQualifiedName());
            } else {
                // 6.5.6.2: Expression name is of the form Q.id
                QualifiedIdentifier Q = qid.getQualifiedIdentifierWithoutLast();
                std::string& id = qid.identifiers.back().name;
                switch (Q.getClassification()) {
                    case PACKAGE_NAME:
                        THROW_TypeCheckerError("PackageName precedes ExpressionName in " + qid.getQualifiedName());
                    case TYPE_NAME: {
                        // Q must be a class, and id must be a static field 
                        auto cls_lookup = compilation_unit_namespace.lookupQualifiedType(Q);
                        if (auto cls = std::get_if<ClassDeclarationObject*>(&cls_lookup)) {
                            if (auto possible_field = checkIfFieldIsAccessible(current_class, *cls, id, true)) {
                                qid.link = possible_field->type;
                                return;
                            }
                            THROW_TypeCheckerError(
                                "Class " + Q.getQualifiedName() + " does not have an accessible field named " + id
                            );
                        }
                        // Q is not a class
                        THROW_TypeCheckerError(
                            "Attempted to access field " + id + " in interface " + Q.getQualifiedName());
                    }
                    case EXPRESSION_NAME: {
                        // Must determine type T of Q
                        this->operator()(Q);
                        LinkedType T = Q.link;
                        if (!T.isReferenceType()) {
                            THROW_TypeCheckerError(
                                "Primitive ExpressionName precedes ExpressionName in " + Q.getQualifiedName());
                        }
                        if (T.is_array && id == "length") {
                            // Special case: "length" field of an array
                            qid.link = LinkedType(PrimitiveType::INT);
                            return;
                        }
                        if (auto class_type = T.getIfNonArrayIsClass()) {
                            if (auto possible_field = checkIfFieldIsAccessible(current_class, class_type, id)) {
                                qid.link = possible_field->type;
                                return;
                            }
                            THROW_TypeCheckerError(
                                "No accessible field " + id + " in " + Q.getQualifiedName() + " from class " + current_class->identifier
                            );
                        }
                        THROW_TypeCheckerError(
                                "Interface ExpressionName " + Q.getQualifiedName() + " precedes ExpressionName " + id);
                    }
                    case METHOD_NAME:
                        THROW_CompilerError(
                            "MethodName precedes ExpressionName in " 
                            + qid.getQualifiedName() + " which should never happen");
                }
                THROW_TypeCheckerError("Undefined reference to " + qid.getQualifiedName());
            }
        case TYPE_NAME: {
            // Link this even though its not an expression, as it will be used for static method calls 
            auto type_lookup = compilation_unit_namespace.lookupQualifiedType(qid);
            qid.link = LinkedType(type_lookup, false, true);
        }
        default:
            // Not an expression
            return;
    }
}

bool checkAssignability(LinkedType& linkedType1, LinkedType& linkedType2, PackageDeclarationObject* default_package) {
    if(linkedType1.isPrimitive()) {
        auto typeEnum1 = std::get<PrimitiveType>(linkedType1.linked_type);
        if(linkedType2.isPrimitive()) {
            auto typeEnum2 = std::get<PrimitiveType>(linkedType2.linked_type);
            if(typeEnum1 == typeEnum2) {
                return true;
            }
            else if(((typeEnum1 == PrimitiveType::INT && linkedType2.isNumeric()) || 
                    (typeEnum1 == PrimitiveType::SHORT && typeEnum2 == PrimitiveType::BYTE)) && 
                    !linkedType1.is_array && !linkedType2.is_array) {
                return true;
            }
        }
    }
    else if((linkedType1.getIfNonArrayIsClass() == default_package->getJavaLangObject())) {
        return true;
    }
    else {
        if(linkedType2.isNull() || linkedType1.isSubType(linkedType2, default_package)) {
            return true;
        }
    }
    return false;
}

void TypeChecker::operator()(Assignment &node) {
    this->visit_children(node);

    LinkedType linkedType1 = getLink(node.assigned_to);
    LinkedType linkedType2 = getLink(node.assigned_from);
    
    if (checkAssignability(linkedType1, linkedType2, default_package)) {
        node.link = linkedType1;
    }
    else {
        THROW_TypeCheckerError("Invalid type for assignment operation");
    }
}

void TypeChecker::operator()(ParenthesizedExpression &node) {
    this->visit_children(node);
    node.link = getLink(node.expression);
}

void TypeChecker::operator()(InfixExpression &node) {
    this->visit_children(node);

    LinkedType linkedType1 = getLink(node.expression1);
    LinkedType linkedType2 = getLink(node.expression1);

    switch (node.op) {
        case InfixOperator::PLUS:
            if(linkedType1.isNumeric() && linkedType2.isNumeric()) {
                node.link = LinkedType(PrimitiveType::INT);
            }
            else if((getStringClass(linkedType1) && !linkedType2.isVoid()) || (getStringClass(linkedType2) && !linkedType1.isVoid())) {
                node.link = LinkedType(NonArrayLinkedType(default_package->findClassDeclaration("java.lang.String")));
            } 
            else {
                THROW_TypeCheckerError("Invalid type for addition operation");
            }
            break;
        case InfixOperator::MINUS:
        case InfixOperator::MULTIPLY:
        case InfixOperator::DIVIDE:
        case InfixOperator::MODULO:
            if(linkedType1.isNumeric() && linkedType2.isNumeric()) {
                node.link = LinkedType(PrimitiveType::INT);
            }
            else {
                THROW_TypeCheckerError("Invalid type for arithmetic operation");
            }
            break;
        case InfixOperator::LESS_THAN:
        case InfixOperator::GREATER_THAN:
        case InfixOperator::LESS_THAN_EQUAL:
        case InfixOperator::GREATER_THAN_EQUAL:
            if(linkedType1.isNumeric() && linkedType2.isNumeric()) {
                node.link = LinkedType(PrimitiveType::BOOLEAN);
            }
            else {
                THROW_TypeCheckerError("Invalid type for comparison operation");
            }
            break;
        case InfixOperator::BOOLEAN_EQUAL:
        case InfixOperator::BOOLEAN_NOT_EQUAL:
            if(linkedType1.isSubType(linkedType2, default_package) || linkedType2.isSubType(linkedType1, default_package) ||
                (linkedType1.isNull() && (linkedType2.isReferenceType() || linkedType2.is_array))  
                || (linkedType2.isNull() && (linkedType1.isReferenceType() || linkedType1.is_array))
                || (linkedType1.isNumeric() && linkedType2.isNumeric())) {
                node.link = LinkedType(PrimitiveType::BOOLEAN);
            }
            else {
                THROW_TypeCheckerError("Invalid type for comparison operation");
            }
            break;
        case InfixOperator::BOOLEAN_AND:
        case InfixOperator::BOOLEAN_OR:
        case InfixOperator::EAGER_AND:
        case InfixOperator::EAGER_OR:
            if(linkedType1.isBoolean() && linkedType2.isBoolean()) {
                node.link = LinkedType(PrimitiveType::BOOLEAN);
            }
            else {
                THROW_TypeCheckerError("Invalid type for boolean operation");
            }
            break;
        default:
            THROW_TypeCheckerError("Invalid type for InfixExpression");
    }
}

void TypeChecker::operator()(MethodInvocation &node) {

    /* JLS 15.12: Method Invocation Expressions */
    std::string& method_name = node.method_name->name;
    this->visit_children(node);

    // JLS 15.12.1: Compile-Time Step 1 - Determine Class or Interface to Search
    LinkedType type_to_search;
    if (!node.parent_expr) {
        // Simple MethodName
        NonArrayLinkedType current_class_casted = current_class;
        type_to_search = LinkedType(current_class_casted);
    } else {
        type_to_search = getLink(node.parent_expr);
    }
    // Static methods can only be called on TypeName that does not refer to an interface
    if (type_to_search.not_expression && type_to_search.getIfIsInterface()) {
        THROW_TypeCheckerError("Interface type cannot perform static method calls"); 
    }

    // JLS 15.12.2: Compile Time Step 2 - Determine Method Signature
    std::list<MethodDeclarationObject*> invoked_method_candidates = type_to_search.getAllMethods(method_name);
    // Find all applicable & accessible methods
    std::vector<MethodDeclarationObject*> found_methods;
    for (auto candidate : invoked_method_candidates) {
        auto parameters = candidate->getParameters();
        auto &arguments = node.arguments;

        if (parameters.size() != arguments.size()) {
            // Method is not applicable; mismatched number of arguments
            continue;
        }

        for (size_t i = 0; i < parameters.size(); ++i) {
            LinkedType param_type = parameters[i]->type;
            LinkedType arg_type = getLink(arguments[i]);
            if (param_type != arg_type) {
                goto not_applicable_or_accessible;
            }
        }

        if (!checkifMethodIsAccessible(candidate, type_to_search)) {
            // Method is applicable, but not accessible
            continue;
        }

        found_methods.push_back(candidate);
        not_applicable_or_accessible:;
    }

    MethodDeclarationObject* determined_method = nullptr;
    if (found_methods.empty()) {
        THROW_TypeCheckerError(
            "No method declaration for " + method_name + " is applicable and accessible in " + type_to_search.toSimpleString()
        );
    } else if (found_methods.size() > 1) {
        // JLS 15.12.2.2: If one of the methods is not declared abstract, it is the most specific method
        for (auto found_method : found_methods) {
            if (!found_method->ast_reference->hasModifier(Modifier::ABSTRACT)) {
                determined_method = found_method;
            }
        }
        if (!determined_method) {
            // JLS 15.12.2.2: All the methods are necessarily abstract, the method is chosen arbitrarily
            determined_method = found_methods[0];
        }
    } else {
        // Exactly one method is applicable and accessible
        determined_method = found_methods[0];
    }

    // JLS 15.12.3 Compile-Time Step 3 - Is the Chosen Method Appropriate?
    if (type_to_search.not_expression) {
        // Static method call
        if (!determined_method->ast_reference->hasModifier(Modifier::STATIC)) {
            THROW_TypeCheckerError("Static method call invoked on instance method"); 
        }
    } else {
        // Instance method call
        if (current_class == nullptr) {
            // Instance method call in static-only context; e.g. member initialization
            THROW_TypeCheckerError("Instance method call invoked in static context"); 
        }
    }

    node.link = determined_method->return_type;
}

void TypeChecker::operator()(QualifiedThis &node) {
    if (current_method && current_method->ast_reference->hasModifier(Modifier::STATIC)) {
        THROW_TypeCheckerError("Static method cannot call 'this'");
    }

    // Type of 'this' is type of enclosing class
    NonArrayLinkedType non_array_current = current_class;
    node.link = LinkedType(non_array_current);
    this->visit_children(node);
}

void TypeChecker::operator()(PrefixExpression &node) {
    this->visit_children(node);

    LinkedType linkedType = getLink(node.expression);
    if(linkedType.is_array) {
        THROW_TypeCheckerError("Prefix operators are not defined for array types");
    }

    switch (node.op) {
        case PrefixOperator::MINUS:
            if(linkedType.isNumeric()) {
                node.link.linked_type = linkedType.linked_type;
            }
            else {
                THROW_TypeCheckerError("Invalid type for arithmetic operation");
            }
            break;
        case PrefixOperator::NEGATE:
            if(linkedType.isBoolean()) {
                node.link.linked_type = linkedType.linked_type;
            }
            else {
                THROW_TypeCheckerError("Invalid type for boolean operation");
            }
            break;
        default:
            THROW_TypeCheckerError("Invalid type for PrefixExpression");
    }
}

void TypeChecker::operator()(ArrayCreationExpression &node) {
    this->visit_children(node);

    LinkedType type = node.type.get()->link;
    LinkedType expression = getLink(node.expression);
    if(expression.isNumeric()) {
        node.link.linked_type = type.linked_type;
        node.link.is_array = true;
    }
    else {
        THROW_TypeCheckerError("Invalid type for ArrayCreationExpression");
    }
}

void TypeChecker::operator()(ClassInstanceCreationExpression &node) {
    this->visit_children(node);
    node.link = node.linked_class_type;
}

void TypeChecker::operator()(FieldAccess &node) {
    this->visit_children(node);

    std::string& field_name = node.identifier->name;
    FieldDeclarationObject* resolved_field = nullptr;

    // Type that the field is being accessed on
    LinkedType object_type = getLink(node.expression);

    // If the type is an array, only length is accessible
    if (object_type.is_array) {
        if (field_name == "length") {
            node.link = LinkedType(PrimitiveType::INT);
            return;
        }
    } else {
        resolved_field = checkIfFieldIsAccessible(
            current_class,
            object_type.getIfNonArrayIsClass(),
            field_name,
            object_type.not_expression
        );
        if (resolved_field) {
            node.link = resolved_field->type;
            return;
        }
    }

    THROW_TypeCheckerError("No accessible field " + field_name + " in " + object_type.toSimpleString());
}

void TypeChecker::operator()(ArrayAccess &node) {
    this->visit_children(node);

    LinkedType typeArray = getLink(node.array);
    LinkedType typeSelector = getLink(node.selector);
    if(typeArray.is_array && typeSelector.isNumeric()) {
        node.link.linked_type = typeArray.linked_type;
    }
    else {
        THROW_TypeCheckerError("Invalid type for ArrayAccess");
    }
}

bool isFinal(LinkedType type) {
    if(type.isReferenceType()) {
        ClassDeclarationObject* class_decl = type.getIfNonArrayIsClass();
        if(class_decl != nullptr) {
            return class_decl->ast_reference->hasModifier(Modifier::FINAL);
        }
    }
    return false;
}

// Checking (type)expression castability
bool checkCastability(LinkedType& type, LinkedType& expression, PackageDeclarationObject* default_package) {
    if(type.isNumeric() && expression.isNumeric() && !type.is_array && !expression.is_array) {
        return true;
    }
    else if((expression.getIfNonArrayIsInterface() && (type.getIfNonArrayIsInterface() || !isFinal(type))) 
            || (type.getIfNonArrayIsInterface() && (expression.getIfNonArrayIsInterface() || !isFinal(expression)))) {
        return true;
    }
    else if(expression.isNull() && (type.isReferenceType() || type.is_array)) {
        return true;
    } 
    // If this is java.lang.object return true or if other is true and this is a reference type/array return true
    else if((type.getIfNonArrayIsClass() == default_package->getJavaLangObject()) ||
       (expression.getIfNonArrayIsClass() == default_package->getJavaLangObject() && (type.is_array || type.isReferenceType()))) {
        return true;
    }
    else if(type.isSubType(expression, default_package)) {
        return true;
    }
    return false;
}

void TypeChecker::operator()(CastExpression &node) {
    this->visit_children(node);

    LinkedType type = node.type.get()->link;
    LinkedType expression = getLink(node.expression);

    if(checkCastability(type, expression, default_package)) {
        node.link = type;
    }
    else {
        THROW_TypeCheckerError("Invalid type for CastExpression");
    }
}

void TypeChecker::operator()(InstanceOfExpression &node) {
    this->visit_children(node);

    LinkedType expression = getLink(node.expression);
    LinkedType type = node.type.get()->link;
    if(type.isReferenceType() && checkCastability(type, expression, default_package)) {
        node.link = LinkedType(PrimitiveType::BOOLEAN);
    }
    else {
        THROW_TypeCheckerError("Invalid type for InstanceOfExpression");
    }
}

TypeChecker::TypeChecker(PackageDeclarationObject &default_package) 
    : default_package{&default_package}, current_method{nullptr} {}
