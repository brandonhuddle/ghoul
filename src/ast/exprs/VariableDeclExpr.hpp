#ifndef GULC_VARIABLEDECLEXPR_HPP
#define GULC_VARIABLEDECLEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Identifier.hpp>
#include <ast/Type.hpp>

namespace gulc {
    class VariableDeclExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::VariableDecl; }

        Type* type;
        Expr* initialValue;

        VariableDeclExpr(Identifier identifier, Type* type, Expr* initialValue, bool isAssignable,
                         TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::VariableDecl),
                  type(type), initialValue(initialValue), _identifier(std::move(identifier)),
                  _isAssignable(isAssignable), _startPosition(startPosition), _endPosition(endPosition) {}

        Identifier const& identifier() const { return _identifier; }
        bool isAssignable() const { return _isAssignable; }

        TextPosition startPosition() const { return _startPosition; }
        TextPosition endPosition() const { return _endPosition; }

        ~VariableDeclExpr() override {
            delete type;
            delete initialValue;
        }

    protected:
        Identifier _identifier;

        // `let mut`
        bool _isAssignable;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_VARIABLEDECLEXPR_HPP