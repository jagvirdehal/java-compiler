#include <memory>
#include <variant>
#include <vector>

struct QualifiedIdentifier;
struct Identifier;
struct Expression;
struct Modifier;
struct FormalParameter;
struct Block;
struct LocalVariableDeclaration;
struct Statement;

enum PrimitiveType {
    BYTE, SHORT, INT, CHAR, BOOLEAN, VOID
};

typedef std::variant<PrimitiveType, QualifiedIdentifier> NonArrayType;

struct Type {
    std::unique_ptr<NonArrayType> non_array_type;
    bool is_array;

    Type(std::unique_ptr<NonArrayType>& non_array_type, bool is_array) :
        non_array_type{std::move(non_array_type)}, is_array{is_array} {}
};

struct VariableDeclarator {
    std::unique_ptr<Identifier> id;
    std::unique_ptr<Expression> expression;

    VariableDeclarator(std::unique_ptr<Identifier>& id, std::unique_ptr<Expression>& expression) :
        id{std::move(id)}, expression{std::move(expression)} {}
};

struct MethodDeclaration {
    std::vector<Modifier> modifiers;
    std::unique_ptr<Type> type;
    std::unique_ptr<Identifier> function_name;
    std::vector<FormalParameter> parameters;
    std::unique_ptr<Block> body;

    MethodDeclaration(
        std::vector<Modifier>& modifiers,
        std::unique_ptr<Type>& type,
        std::unique_ptr<Identifier>& function_name,
        std::vector<FormalParameter>& parameters,
        std::unique_ptr<Block>& body
    ) :
        modifiers{std::move(modifiers)}, 
        type{std::move(type)},
        function_name{std::move(function_name)},
        parameters{std::move(parameters)},
        body{std::move(body)} {}
};

struct Block {
    std::vector<LocalVariableDeclaration> variable_declarations;
    std::vector<Statement> statements;

    Block(
        std::vector<LocalVariableDeclaration>& variable_declarations, 
        std::vector<Statement>& statements
    ) :
        variable_declarations{std::move(variable_declarations)}, 
        statements{std::move(statements)} {}
};

struct FormalParameter {
    std::unique_ptr<Type> type;
    std::unique_ptr<Identifier> parameter_name;

    FormalParameter(
        std::unique_ptr<Type>& type, 
        std::unique_ptr<Identifier>& parameter_name
    ) :
        type{std::move(type)}, 
        parameter_name{std::move(parameter_name)} {}
};
