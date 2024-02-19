#pragma once
#include <vector>
#include <algorithm>

namespace util{

// Remove all duplicates from vector
template <typename T> void removeDuplicates(std::vector<T>& vec) {
    std::sort(vec.begin(), vec.end());
    auto last = std::unique(vec.begin(), vec.end());
    vec.erase(last, vec.end());
};

void removeDuplicates(std::vector<TypeDeclaration> &vec) {
     std::sort(vec.begin(), vec.end(), [](const TypeDeclaration& lhs, const TypeDeclaration& rhs) {
        auto getIdentifier = [](const auto& type) -> const std::string& {
            return type->identifier;
        };
        
        const std::string& id1 = std::visit(getIdentifier, lhs);
        const std::string& id2 = std::visit(getIdentifier, rhs);

        return id1 < id2;
    });

    vec.erase(std::unique(vec.begin(), vec.end(), 
    [](const TypeDeclaration& lhs, const TypeDeclaration& rhs) {
        return std::visit([](const auto& type) { return type->identifier; }, lhs) == std::visit([](const auto& type) { return type->identifier; }, rhs);
    }), vec.end());
};

};
