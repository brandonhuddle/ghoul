#ifndef GULC_UNRESOLVEDTYPE_HPP
#define GULC_UNRESOLVEDTYPE_HPP

#include <ast/Type.hpp>
#include <vector>
#include <ast/Identifier.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class UnresolvedType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Unresolved; }

        UnresolvedType(Qualifier qualifier, std::vector<Identifier> namespacePath, Identifier identifier,
                       std::vector<Expr*> templateArguments)
                : Type(Type::Kind::Unresolved, qualifier, false),
                  _namespacePath(std::move(namespacePath)), _identifier(std::move(identifier)),
                  templateArguments(std::move(templateArguments)) {}

        std::vector<Identifier> const& namespacePath() const { return _namespacePath; }
        Identifier const& identifier() const { return _identifier; }
        std::vector<Expr*> templateArguments;
        bool hasTemplateArguments() const { return !templateArguments.empty(); }

        TextPosition startPosition() const override {
            if (!_namespacePath.empty()) {
                return _namespacePath.front().startPosition();
            } else {
                return _identifier.startPosition();
            }
        }

        // TODO: Should we differentiate between the ending `>` in a template type?
        TextPosition endPosition() const override {
            return _identifier.endPosition();
        }

        std::string toString() const override {
            std::string result;

            for (Identifier const& namespaceIdentifier : _namespacePath) {
                result += namespaceIdentifier.name() + ".";
            }

            return result + _identifier.name();
        }

        ~UnresolvedType() override {
            for (Expr* templateArgument : templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        std::vector<Identifier> _namespacePath;
        Identifier _identifier;

    };
}

#endif //GULC_UNRESOLVEDTYPE_HPP