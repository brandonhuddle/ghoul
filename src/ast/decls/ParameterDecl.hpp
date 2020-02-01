#ifndef GULC_PARAMETERDECL_HPP
#define GULC_PARAMETERDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <optional>

namespace gulc {
    class ParameterDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Parameter; }

        enum class ParameterKind {
            Val, // The normal, unlabeled parameter
            In, // An `in` reference parameter
            Out, // An `out` reference parameter, requires the parameter to be written on all codepaths
            InOut, // A reference parameter that is treated as both `in` and `out` (it must be initialized when coming
                   // in and must be set before returning)
        };

        Type* type;
        Expr* defaultValue;
        ParameterKind parameterKind() const { return _parameterKind; }

        ParameterDecl(unsigned int sourceFileID, std::vector<Attr*> attributes,
                      Identifier argumentLabel, Identifier identifier,
                      Type* type, Expr* defaultValue, ParameterKind parameterKind,
                      TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::Parameter, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier)),
                  type(type), defaultValue(defaultValue), _argumentLabel(std::move(argumentLabel)),
                  _parameterKind(parameterKind), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Identifier const& argumentLabel() const { return _argumentLabel; }

        ~ParameterDecl() override {
            delete type;
            delete defaultValue;
        }

    protected:
        Identifier _argumentLabel;
        ParameterKind _parameterKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_PARAMETERDECL_HPP
