#ifndef GULC_TEMPLATEPARAMETERDECL_HPP
#define GULC_TEMPLATEPARAMETERDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>

namespace gulc {
    class TemplateParameterDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateParameter; }

        enum class TemplateParameterKind {
            // A type `struct Example<T> == Example<int>`
            Typename,
            // A const variable `struct Example<const param: int> == Example<12>`
            Const
        };

        // This is null if the kind is `typename`
        Type* constType;
        TemplateParameterKind templateParameterKind() const { return _templateParameterKind; }

        // Constructor for `typename`
        TemplateParameterDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                              TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TemplateParameter, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, true, std::move(identifier),
                       DeclModifiers::None),
                  constType(nullptr), _templateParameterKind(TemplateParameterKind::Typename),
                  _startPosition(startPosition), _endPosition(endPosition) {}
        // Constructor for `const`
        TemplateParameterDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                              Type* type, TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TemplateParameter, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, true, std::move(identifier),
                       DeclModifiers::None),
                  constType(type), _templateParameterKind(TemplateParameterKind::Const),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            Decl* result = nullptr;

            if (_templateParameterKind == TemplateParameterKind::Const) {
                result = new TemplateParameterDecl(_sourceFileID, copiedAttributes, _identifier,
                                                   constType->deepCopy(), _startPosition, _endPosition);
            } else {
                result = new TemplateParameterDecl(_sourceFileID, copiedAttributes, _identifier,
                                                   _startPosition, _endPosition);
            }

            result->container = container;
            result->containedInTemplate = containedInTemplate;
            return result;
        }

        ~TemplateParameterDecl() override {
            delete constType;
        }

    protected:
        TemplateParameterKind _templateParameterKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATEPARAMETERDECL_HPP
