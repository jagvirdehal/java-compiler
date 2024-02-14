#include <memory>
#include <vector>
#include <gtest/gtest.h>

#include "variant-ast/astnode.h"
#include "weeder/astweeder.h"

TEST(AstWeeder, weedOutNoModifiers) {
    ClassDeclaration no_modifiers_class = ClassDeclaration(
        std::vector<Modifier>(),
        std::make_unique<Identifier>("foo"),
        std::unique_ptr<QualifiedIdentifier>(),
        std::vector<QualifiedIdentifier>(),
        std::vector<FieldDeclaration>(),
        std::vector<MethodDeclaration>()
    );
    std::vector<ClassDeclaration> classes;
    classes.push_back(std::move(no_modifiers_class));

    AstNodeVariant root = CompilationUnit(
        std::make_unique<QualifiedIdentifier>(
            std::vector<Identifier>()
        ),
        std::vector<QualifiedIdentifier>(), 
        std::vector<QualifiedIdentifier>(), 
        std::move(classes), 
        std::vector<InterfaceDeclaration>()
    );

    auto weeder = AstWeeder();
    int return_code = weeder.weed(root, "foo");
    std::cout << "Weeder returned " << return_code << "\n";
    EXPECT_TRUE(return_code == 42) << "Weeder failed to detect violation";
}
