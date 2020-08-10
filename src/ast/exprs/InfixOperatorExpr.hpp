#ifndef GULC_INFIXOPERATOREXPR_HPP
#define GULC_INFIXOPERATOREXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    enum class InfixOperators {
        Unknown,
        Add, // +
        Subtract, // -
        Multiply, // *
        Divide, // /
        Remainder, // %
        Power, // ^^ (exponents)

        BitwiseAnd, // &
        BitwiseOr, // |
        BitwiseXor, // ^

        BitshiftLeft, // << (logical shift left)
        BitshiftRight, // >> (logical shift right OR arithmetic shift right, depending on the type)

        LogicalAnd, // &&
        LogicalOr, // ||

        EqualTo, // ==
        NotEqualTo, // !=

        GreaterThan, // >
        LessThan, // <
        GreaterThanEqualTo, // >=
        LessThanEqualTo, // <=

        // TODO: Implement this
        Spaceship, // <=>
    };

    inline std::string getInfixOperatorStringValue(InfixOperators infixOperator) {
        switch (infixOperator) {
            default:
                return "[UNKNOWN]";
            case InfixOperators::Add:
                return "+";
            case InfixOperators::Subtract:
                return "-";
            case InfixOperators::Multiply:
                return "*";
            case InfixOperators::Divide:
                return "/";
            case InfixOperators::Remainder:
                return "%";
            case InfixOperators::Power:
                return "^^";
            case InfixOperators::BitwiseAnd:
                return "&";
            case InfixOperators::BitwiseOr:
                return "|";
            case InfixOperators::BitwiseXor:
                return "^";
            case InfixOperators::BitshiftLeft:
                return "<<";
            case InfixOperators::BitshiftRight:
                return ">>";
            case InfixOperators::LogicalAnd:
                return "&&";
            case InfixOperators::LogicalOr:
                return "||";
            case InfixOperators::EqualTo:
                return "==";
            case InfixOperators::NotEqualTo:
                return "!=";
            case InfixOperators::GreaterThan:
                return ">";
            case InfixOperators::LessThan:
                return "<";
            case InfixOperators::GreaterThanEqualTo:
                return ">=";
            case InfixOperators::LessThanEqualTo:
                return "<=";
        }
    }

    class InfixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::InfixOperator; }

        Expr* leftValue;
        Expr* rightValue;

        InfixOperators infixOperator() const { return _infixOperator; }

        InfixOperatorExpr(InfixOperators infixOperator, Expr* leftValue, Expr* rightValue)
                : Expr(Expr::Kind::InfixOperator),
                  leftValue(leftValue), rightValue(rightValue), _infixOperator(infixOperator) {}

        TextPosition startPosition() const override { return leftValue->startPosition(); }
        TextPosition endPosition() const override { return rightValue->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new InfixOperatorExpr(_infixOperator, leftValue->deepCopy(), rightValue->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return leftValue->toString() + " " + getInfixOperatorStringValue(_infixOperator) + " " + rightValue->toString();
        }

        ~InfixOperatorExpr() override {
            delete leftValue;
            delete rightValue;
        }

    protected:
        InfixOperators _infixOperator;

    };
}

#endif //GULC_INFIXOPERATOREXPR_HPP
