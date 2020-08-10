#include <ast/types/ReferenceType.hpp>
#include "SignatureComparer.hpp"
#include "TypeHelper.hpp"
#include "TypeCompareUtil.hpp"
#include <algorithm>
#include <ast/exprs/TypeExpr.hpp>

using namespace gulc;

SignatureComparer::CompareResult SignatureComparer::compareFunctions(const FunctionDecl* left,
                                                                     const FunctionDecl* right,
                                                                     bool checkSimilar) {
    if (left->isStatic() != right->isStatic() || left->isMutable() != right->isMutable() ||
        left->identifier().name() != right->identifier().name()) {
        return CompareResult::Different;
    }

    return compareParameters(left->parameters(), right->parameters(), checkSimilar);
}

SignatureComparer::CompareResult SignatureComparer::compareParameters(std::vector<ParameterDecl*> const& left,
                                                                      std::vector<ParameterDecl*> const& right,
                                                                      bool checkSimilar,
                                                                      TemplateComparePlan templateComparePlan) {
    // If `checkSimilar` is enabled then we will still check similarity even if the functions have different numbers of
    // parameters
    if (left.size() != right.size() && !checkSimilar) {
        // They are only `Different` if `checkSimilar` is false
        return CompareResult::Different;
    }

    std::size_t parameterSize = std::max(left.size(), right.size());

    for (std::size_t i = 0; i < parameterSize; ++i) {
        if (i >= left.size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // If the right parameter is optional then the functions are similar
            if (right[i]->defaultValue != nullptr) {
                return CompareResult::Similar;
            } else {
                return CompareResult::Different;
            }
        } else if (i >= right.size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // If the left parameter is optional then the functions are similar
            if (left[i]->defaultValue != nullptr) {
                return CompareResult::Similar;
            } else {
                return CompareResult::Different;
            }
        } else {
            // If one side is optional and the other isn't then they are different BUT they could still be similar
            // so we only return `Different` if we're not checking for similarities
            if (!checkSimilar && (left[i]->defaultValue == nullptr) !=
                                 (right[i]->defaultValue == nullptr)) {
                return CompareResult::Different;
            }

            auto leftParam = left[i];
            auto rightParam = right[i];

            // If the labels aren't the same then the functions aren't the same. ("_" must be for both for that to
            // affect it)
            if (leftParam->argumentLabel().name() != rightParam->argumentLabel().name()) {
                return CompareResult::Different;
            }

            Type* leftType = leftParam->type;
            Type* rightType = rightParam->type;

            // NOTE: `ref int` == `int` are the same BUT `ref mut int` != `int` and `ref mut int` != `ref int`
            //       This is because `func ex(_ param: ref int);` is callable as `ex(12)`
            if (leftType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(leftType)) {
                leftType = llvm::dyn_cast<ReferenceType>(leftType)->nestedType;
            }

            if (rightType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(rightType)) {
                rightType = llvm::dyn_cast<ReferenceType>(rightType)->nestedType;
            }

            TypeCompareUtil typeCompareUtil;
            if (!typeCompareUtil.compareAreSame(leftType, rightType, templateComparePlan)) {
                return CompareResult::Different;
            }
        }
    }

    // If we make it to this point the functions are exactly the same
    return CompareResult::Exact;
}

SignatureComparer::CompareResult SignatureComparer::compareTemplateFunctions(TemplateFunctionDecl const* left,
                                                                             TemplateFunctionDecl const* right,
                                                                             bool checkSimilar) {
    // Name, `static`, and `mut` must all match to be the same.
    if (left->isStatic() != right->isStatic() || left->isMutable() != right->isMutable() ||
        left->identifier().name() != right->identifier().name()) {
        return CompareResult::Different;
    }

    // If `checkSimilar` is enabled then we will still check similarity even if the functions have different numbers of
    // template parameters
    if (left->templateParameters().size() != right->templateParameters().size() && !checkSimilar) {
        // They are only `Different` if `checkSimilar` is false
        return CompareResult::Different;
    }

    std::size_t templateParameterSize = std::max(left->templateParameters().size(), right->templateParameters().size());

    for (std::size_t i = 0; i < templateParameterSize; ++i) {
        if (i >= left->templateParameters().size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // TODO: Once we support default template parameter values we need to reenable this
            // If the right parameter is optional then the functions are similar
//            if (right->templateParameters()[i]->defaultValue != nullptr) {
//                return CompareResult::Similar;
//            } else {
                return CompareResult::Different;
//            }
        } else if (i >= right->templateParameters().size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // TODO: Once we support default template parameter values we need to reenable this
            // If the left parameter is optional then the functions are similar
//            if (left->templateParameters()[i]->defaultValue != nullptr) {
//                return CompareResult::Similar;
//            } else {
                return CompareResult::Different;
//            }
        } else {
            // If one side is optional and the other isn't then they are different BUT they could still be similar
            // so we only return `Different` if we're not checking for similarities
            // TODO: Once we support default template parameter values we need to reenable this
//            if (!checkSimilar && (left->templateParameters()[i]->defaultValue == nullptr) !=
//                                 (right->templateParameters()[i]->defaultValue == nullptr)) {
//                return CompareResult::Different;
//            }

            auto leftParam = left->templateParameters()[i];
            auto rightParam = right->templateParameters()[i];

            // The template kinds must match (`<T>` vs `<const size: usize>`)
            if (leftParam->templateParameterKind() != rightParam->templateParameterKind()) {
                return CompareResult::Different;
            }

            // NOTE: We only check further into the template parameter if they are `const`, if they're a `typename` we
            //       do no further checking.
            if (leftParam->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
                Type* leftType = leftParam->constType;
                Type* rightType = rightParam->constType;

                // NOTE: `ref int` == `int` are the same BUT `ref mut int` != `int` and `ref mut int` != `ref int`
                //       This is because `func ex(_ param: ref int);` is callable as `ex(12)`
                if (leftType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(leftType)) {
                    leftType = llvm::dyn_cast<ReferenceType>(leftType)->nestedType;
                }

                if (rightType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(rightType)) {
                    rightType = llvm::dyn_cast<ReferenceType>(rightType)->nestedType;
                }

                TypeCompareUtil typeCompareUtil;
                // We check if the types are the same BUT `<G>` == `<T>`, any use of a template type will equal any
                // other template type
                if (!typeCompareUtil.compareAreSame(leftType, rightType,
                                                    TemplateComparePlan::AllTemplatesAreSame)) {
                    return CompareResult::Different;
                }
            }
        }
    }

    // Next we check the normal parameters. If we've made it this far then the functions are exact so far
    // NOTE: We consider any template type to equal any other template type for this comparison.
    return compareParameters(left->parameters(), right->parameters(), checkSimilar,
                             TemplateComparePlan::AllTemplatesAreSame);
}

SignatureComparer::ArgMatchResult SignatureComparer::compareArgumentsToParameters(std::vector<ParameterDecl*> const& parameters,
                                                                                  std::vector<LabeledArgumentExpr*> const& arguments) {
    return compareArgumentsToParameters(parameters, arguments, {}, {});
}

SignatureComparer::ArgMatchResult SignatureComparer::compareArgumentsToParameters(std::vector<ParameterDecl*> const& parameters,
                                                                                  std::vector<LabeledArgumentExpr*> const& arguments,
                                                                                  std::vector<TemplateParameterDecl*> const& templateParameters,
                                                                                  std::vector<Expr*> const& templateArguments) {
    // If there are more parameters than parameters then we can immediately fail
    if (arguments.size() > parameters.size()) {
        return ArgMatchResult::Fail;
    }

    ArgMatchResult currentResult = ArgMatchResult::Match;

    TypeCompareUtil typeCompareUtil(&templateParameters, &templateArguments);

    for (std::size_t i = 0; i < parameters.size(); ++i) {
        if (i >= arguments.size()) {
            // If there are more parameters than parameters then the parameter immediately after the last argument MUST
            // be optional
            if (parameters[i]->defaultValue != nullptr) {
                return currentResult;
            } else {
                return ArgMatchResult::Fail;
            }
        } else {
            // If the argument label doesn't match the one provided by the parameter then we fail...
            // NOTE: For parameters without labels we implicitly add `_` to the argument expression as the label
            if (arguments[i]->label().name() != parameters[i]->argumentLabel().name()) {
                return ArgMatchResult::Fail;
            }

            // If the argument is a reference but the parameter is not then we remove the reference type for comparison
            gulc::Type* checkArgType = arguments[i]->valueType;

            if (llvm::isa<ReferenceType>(checkArgType) && !llvm::isa<ReferenceType>(parameters[i]->type)) {
                checkArgType = llvm::dyn_cast<ReferenceType>(checkArgType)->nestedType;
            }

            // Compare the type of the argument to the type of the parameter...
            if (!typeCompareUtil.compareAreSame(checkArgType, parameters[i]->type)) {
                // TODO: Check if the argument type can be casted to the parameter type...
                return ArgMatchResult::Fail;
            }
        }
    }

    return currentResult;
}

SignatureComparer::ArgMatchResult SignatureComparer::compareArgumentsToParameters(std::vector<LabeledType*> const& parameters,
                                                                                  std::vector<LabeledArgumentExpr*> const& arguments) {
    if (arguments.size() > parameters.size()) {
        return ArgMatchResult::Fail;
    }

    TypeCompareUtil typeCompareUtil;

    for (std::size_t i = 0; i < parameters.size(); ++i) {
        if (i >= arguments.size()) {
            // NOTE: `LabeledType` CANNOT have default values.
            return ArgMatchResult::Fail;
        } else {
            if (arguments[i]->label().name() != parameters[i]->label()) {
                return ArgMatchResult::Fail;
            }

            // If the argument is a reference but the parameter is not then we remove the reference type for comparison
            gulc::Type* checkArgType = arguments[i]->valueType;

            if (llvm::isa<ReferenceType>(checkArgType) && !llvm::isa<ReferenceType>(parameters[i]->type)) {
                checkArgType = llvm::dyn_cast<ReferenceType>(checkArgType)->nestedType;
            }

            if (!typeCompareUtil.compareAreSame(checkArgType, parameters[i]->type)) {
                // TODO: Check if the argument type can be casted to the parameter type...
                return ArgMatchResult::Fail;
            }
        }
    }

    return ArgMatchResult::Match;
}

SignatureComparer::ArgMatchResult SignatureComparer::compareTemplateArgumentsToParameters(
        std::vector<TemplateParameterDecl*> const& templateParameters,
        std::vector<Expr*> const& templateArguments) {
    // If there are more parameters than parameters then we can immediately fail
    if (templateArguments.size() > templateParameters.size()) {
        return ArgMatchResult::Fail;
    }

    ArgMatchResult currentResult = ArgMatchResult::Match;
    TypeCompareUtil typeCompareUtil;

    for (std::size_t i = 0; i < templateParameters.size(); ++i) {
        if (i >= templateArguments.size()) {
            // If there are more parameters than parameters then the parameter immediately after the last argument MUST
            // be optional
            // TODO: Support default values for template parameters
//            if (parameters[i]->defaultValue != nullptr) {
//                return currentResult;
//            } else {
                return ArgMatchResult::Fail;
//            }
        } else {
            if (templateParameters[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
                // Compare the type of the argument to the const type of the template parameter...
                if (!typeCompareUtil.compareAreSame(templateArguments[i]->valueType,
                        templateParameters[i]->constType)) {
                    // TODO: Check if the argument type can be casted to the parameter type...
                    return ArgMatchResult::Fail;
                }
            } else {
                // Template argument MUST be a `TypeExpr` for the template `typename`
                if (!llvm::isa<TypeExpr>(templateArguments[i])) {
                    return ArgMatchResult::Fail;
                }
            }
        }
    }

    return currentResult;
}
