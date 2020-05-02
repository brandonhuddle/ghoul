#ifndef GULC_DECL_HPP
#define GULC_DECL_HPP

#include <string>
#include <vector>
#include <cassert>
#include "Node.hpp"
#include "Identifier.hpp"
#include "Attr.hpp"
#include "DeclModifiers.hpp"

namespace gulc {
    class Decl : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Decl; }

        enum class Kind {
            Import,

            Function,
            TemplateFunction,

            Property,
            PropertyGet,
            PropertySet,

            Operator,
            CastOperator,
            CallOperator,
            SubscriptOperator,
            SubscriptOperatorGet,
            SubscriptOperatorSet,

            Constructor,
            Destructor,

            Struct,
            TemplateStruct,
            TemplateStructInst,
            Trait,
            TemplateTrait,
            TemplateTraitInst,

            Extension,

            Attribute,

            TypeAlias,
            TypeSuffix,

            Namespace,

            Enum,
            EnumConst,

            Variable,

            Parameter,
            TemplateParameter
        };

        enum class Visibility {
            Unassigned,
            Public,
            Private,
            Protected,
            Internal,
            ProtectedInternal
        };

        Kind getDeclKind() const { return _declKind; }
        unsigned int sourceFileID() const { return _sourceFileID; }
        std::vector<Attr*>& attributes() { return _attributes; }
        std::vector<Attr*> const& attributes() const { return _attributes; }
        Visibility visibility() const { return _declVisibility; }
        Identifier const& identifier() const { return _identifier; }
        // `const` in GUL, `constexpr` in ulang
        bool isConstExpr() const { return _isConstExpr; }

        virtual Decl* deepCopy() const = 0;

        void setAttributes(std::vector<Attr*> attributes) {
            assert(_attributes.empty());

            _attributes = std::move(attributes);
        }

        bool isStatic() const { return (_declModifiers & DeclModifiers::Static) == DeclModifiers::Static; }
        bool isMutable() const { return (_declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut; }
        bool isVolatile() const { return (_declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile; }
        bool isAbstract() const { return (_declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract; }
        bool isVirtual() const { return (_declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual; }
        bool isOverride() const { return (_declModifiers & DeclModifiers::Override) == DeclModifiers::Override; }
        bool isExtern() const { return (_declModifiers & DeclModifiers::Extern) == DeclModifiers::Extern; }
        bool isPrototype() const { return (_declModifiers & DeclModifiers::Prototype) == DeclModifiers::Prototype; }

        // Makes checking if it is virtual at all easier
        bool isAnyVirtual() const { return isVirtual() || isAbstract() || isOverride(); }

    protected:
        Kind _declKind;
        unsigned int _sourceFileID;
        std::vector<Attr*> _attributes;
        Visibility _declVisibility;
        Identifier _identifier;
        bool _isConstExpr;
        DeclModifiers _declModifiers;

        Decl(Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes, Visibility declVisibility,
             bool isConstExpr, Identifier identifier, DeclModifiers declModifiers)
                : Decl(Node::Kind::Decl, declKind, sourceFileID, std::move(attributes), declVisibility,
                       isConstExpr, std::move(identifier), declModifiers) {}
        Decl(Node::Kind nodeKind, Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes,
             Visibility declVisibility, bool isConstExpr, Identifier identifier, DeclModifiers declModifiers)
                : Node(nodeKind), _declKind(declKind), _declVisibility(declVisibility), _sourceFileID(sourceFileID),
                  _attributes(std::move(attributes)), _isConstExpr(isConstExpr), _identifier(std::move(identifier)),
                  _declModifiers(declModifiers) {}

    };
}

#endif //GULC_DECL_HPP
