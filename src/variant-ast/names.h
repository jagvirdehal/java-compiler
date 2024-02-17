#pragma once

#include <string>
#include <vector>
#include "astnodecommon.h"
#include "astnode.h"


struct Identifier: public AstNodeCommon {
    std::string name; // Identifier name

    Identifier(std::string& name) : name(std::move(name)) {}
    Identifier(std::string&& name) : name(std::move(name)) {}
};

struct QualifiedIdentifier: public AstNodeCommon {
    AstNodeCommon* type_link;

    std::vector<Identifier> identifiers; // Vector of identifiers for this qualitfed identifier

    QualifiedIdentifier() {}
    QualifiedIdentifier(std::vector<Identifier>& identifiers) : identifiers(std::move(identifiers)) {}
    QualifiedIdentifier(std::vector<Identifier>&& identifiers) : identifiers(std::move(identifiers)) {}
public: 
    std::string getQualifiedName() {
        std::string result = "";
        for(auto &id : identifiers) {
            result += id.name + ".";
        }
        return result;
    }

    std::string getPackagePrefix() {
        std::string result = "";
        for (int i = 0; i < identifiers.size() - 1; i++) {
            result += identifiers[i].name + ".";
        }

        return result;
    }
};
