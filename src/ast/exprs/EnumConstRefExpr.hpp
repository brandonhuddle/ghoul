#ifndef GULC_ENUMCONSTREFEXPR_HPP
#define GULC_ENUMCONSTREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/EnumConstDecl.hpp>

namespace gulc {
    // NOTE: This will NOT be used for `enum union` since an `enum union` can take parameters on the ref.
    //       We will need to create an `EnumUnionConstRef` that is exactly the same as this but with an `parameters`
    //       member.
    class EnumConstRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::EnumConstRef; }

        EnumConstRefExpr(TextPosition startPosition, TextPosition endPosition, EnumConstDecl* enumConst)
                : Expr(Expr::Kind::EnumConstRef),
                  _startPosition(startPosition), _endPosition(endPosition), _enumConst(enumConst) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        EnumConstDecl* enumConst() const { return _enumConst; }

        Expr* deepCopy() const override {
            auto result = new EnumConstRefExpr(_startPosition, _endPosition, _enumConst);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _enumConst->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        // NOTE: We don't own this so we don't free it.
        EnumConstDecl* _enumConst;

    };
}

#endif //GULC_ENUMCONSTREFEXPR_HPP
