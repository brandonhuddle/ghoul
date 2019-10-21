// Copyright (C) 2019 Michael Brandon Huddle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <AST/Types/PointerType.hpp>
#include <AST/Types/ReferenceType.hpp>
#include <AST/Types/RValueReferenceType.hpp>
#include <AST/Types/ConstType.hpp>
#include <AST/Types/MutType.hpp>
#include <AST/Types/ImmutType.hpp>
#include <AST/Types/BuiltInType.hpp>
#include <iostream>
#include <AST/Types/EnumType.hpp>
#include "ItaniumMangler.hpp"

using namespace gulc;
//https://itanium-cxx-abi.github.io/cxx-abi/abi.html
// TODO: If we every want to allow `extern` to a `C++` function we will need to support substitution.
//  Even in the areas where substitution makes the result function longer than it would be without substitution using clang v6 and gcc v7.4.0

std::string ItaniumMangler::mangle(FunctionDecl *functionDecl) {
    // All mangled names start with "_Z"...
    std::string mangledName = "_Z" + unqualifiedName(functionDecl);

    // We only have to use <bare-function-name> since there isn't a namespace yet.
    mangledName += bareFunctionType(functionDecl->parameters);

    return mangledName;
}

std::string ItaniumMangler::mangle(GlobalVariableDecl *globalVariableDecl) {
    // All mangled names start with "_Z"...
    return "_Z" + unqualifiedName(globalVariableDecl);
}

void ItaniumMangler::mangle(NamespaceDecl *namespaceDecl, const std::string& prefix) {
    std::string nPrefix = prefix + sourceName(namespaceDecl->name());

    for (Decl* decl : namespaceDecl->nestedDecls()) {
        if (llvm::isa<FunctionDecl>(decl)) {
            auto functionDecl = llvm::dyn_cast<FunctionDecl>(decl);
            functionDecl->setMangledName("_ZN" + nPrefix + unqualifiedName(functionDecl) + "E" + bareFunctionType(functionDecl->parameters));
        } else if (llvm::isa<GlobalVariableDecl>(decl)) {
            auto globalVariableDecl = llvm::dyn_cast<GlobalVariableDecl>(decl);
            globalVariableDecl->setMangledName("_ZN" + nPrefix + unqualifiedName(globalVariableDecl) + "E");
        } else if (llvm::isa<NamespaceDecl>(decl)) {
            mangle(llvm::dyn_cast<NamespaceDecl>(decl), nPrefix);
        } else if (llvm::isa<TemplateFunctionDecl>(decl)) {
            auto templateFunctionDecl = llvm::dyn_cast<TemplateFunctionDecl>(decl);

            for (FunctionDecl* implementedFunction : templateFunctionDecl->implementedFunctions()) {
                std::string templatePrefix = unqualifiedName(implementedFunction);
                std::string templateArgsStr = templateArgs(templateFunctionDecl->templateParameters,
                                                           implementedFunction->templateArguments);
                std::string funcType = bareFunctionType(implementedFunction->parameters);

                std::string mangledName = "_ZN" + nPrefix;
                mangledName += templatePrefix;
                mangledName += templateArgsStr + "E";
                // NOTE: I can't seem to find where in the Itanium spec it says we do this, but GCC and clang both put the
                //  function result type before the function type on template functions? But not normal functions?
                mangledName += typeName(implementedFunction->resultType);
                mangledName += funcType;

                implementedFunction->setMangledName(mangledName);
            }
        }
    }
}

void ItaniumMangler::mangle(TemplateFunctionDecl *templateFunctionDecl) {
    for (FunctionDecl* implementedFunction : templateFunctionDecl->implementedFunctions()) {
        std::string templatePrefix = unqualifiedName(implementedFunction);
        std::string templateArgsStr = templateArgs(templateFunctionDecl->templateParameters,
                                                   implementedFunction->templateArguments);
        std::string funcType = bareFunctionType(implementedFunction->parameters);;

        std::string mangledName = "_Z" + templatePrefix;
        mangledName += templateArgsStr;
        // NOTE: I can't seem to find where in the Itanium spec it says we do this, but GCC and clang both put the
        //  function result type before the function type on template functions? But not normal functions?
        mangledName += typeName(implementedFunction->resultType);
        mangledName += funcType;

        implementedFunction->setMangledName(mangledName);
    }
}

std::string ItaniumMangler::unqualifiedName(FunctionDecl *functionDecl) {
    return sourceName(functionDecl->name());
}

std::string ItaniumMangler::unqualifiedName(GlobalVariableDecl *globalVariableDecl) {
    return sourceName(globalVariableDecl->name());
}

std::string ItaniumMangler::bareFunctionType(std::vector<ParameterDecl*> &params) {
    std::string result;

    if (params.empty()) {
        return "v";
    }

    for (ParameterDecl* param : params) {
        // Parameters that reference template type parameters have to use the template reference strings `T_` and `T{n}_`
        if (param->typeTemplateParamNumber > 0) {
            if (param->typeTemplateParamNumber - 1 == 0) {
                result += "T_";
            } else {
                result += "T" + std::to_string(param->typeTemplateParamNumber - 2) + "_";
            }

            continue;
        }

        Type* genType = param->type;

        // TODO: Should we ignore `mut` and `immut` here?
        if (llvm::isa<ConstType>(genType)) {
            genType = llvm::dyn_cast<ConstType>(genType)->pointToType;
        } else if (llvm::isa<ImmutType>(genType)) {
            genType = llvm::dyn_cast<ImmutType>(genType)->pointToType;
        } else if (llvm::isa<MutType>(genType)) {
            genType = llvm::dyn_cast<MutType>(genType)->pointToType;
        }

        result += typeName(genType);
    }

    return result;
}

std::string ItaniumMangler::typeName(gulc::Type *type) {
    if (llvm::isa<BuiltInType>(type)) {
        auto builtInType = llvm::dyn_cast<BuiltInType>(type);
        const std::string checkName = builtInType->name();

        if (checkName == "void") {
            return "v";
        } else if (checkName == "bool") {
            return "b";
        } else {
            return sourceName(checkName);
        }
    } else if (llvm::isa<EnumType>(type)) {
        auto enumType = llvm::dyn_cast<EnumType>(type);
        return "Te" + sourceName(enumType->name());
    } else if (llvm::isa<PointerType>(type)) {
        return "P" + typeName(llvm::dyn_cast<PointerType>(type)->pointToType);
    } else if (llvm::isa<ReferenceType>(type)) {
        return "R" + typeName(llvm::dyn_cast<ReferenceType>(type)->referenceToType);
    } else if (llvm::isa<RValueReferenceType>(type)) {
        return "O" + typeName(llvm::dyn_cast<RValueReferenceType>(type)->referenceToType);
    } else if (llvm::isa<ConstType>(type)) {
        return "K" + typeName(llvm::dyn_cast<ConstType>(type)->pointToType);
    } else if (llvm::isa<MutType>(type)) {
        return "Umut" + typeName(llvm::dyn_cast<MutType>(type)->pointToType);
    } else if (llvm::isa<ImmutType>(type)) {
        return "Uimmut" + typeName(llvm::dyn_cast<ImmutType>(type)->pointToType);
    } else {
        std::cerr << "[INTERNAL NAME MANGLING ERROR] type `" << type->getString() << "` not supported!" << std::endl;
        std::exit(1);
    }
    return "[ERROR]";
}

std::string ItaniumMangler::sourceName(const std::string& s) {
    return std::to_string(s.length()) + s;
}

std::string ItaniumMangler::templateArgs(std::vector<TemplateParameterDecl*>& templateParams,
                                         std::vector<Expr*>& templateArgs) {
    std::string result = "I";

    for (std::size_t i = 0; i < templateParams.size(); ++i) {
        const Expr* templateArgExpr = nullptr;

        if (i >= templateArgs.size()) {
            templateArgExpr = templateParams[i]->defaultArgument();
        } else {
            templateArgExpr = templateArgs[i];
        }

        result += templateArg(templateArgExpr);
    }

    return result + "E";
}

std::string ItaniumMangler::templateArg(const Expr *expr) {
    if (llvm::isa<ResolvedTypeRefExpr>(expr)) {
        auto resolvedType = llvm::dyn_cast<ResolvedTypeRefExpr>(expr);
        return typeName(resolvedType->resolvedType);
    } else if (llvm::isa<IntegerLiteralExpr>(expr) || llvm::isa<FloatLiteralExpr>(expr)) {
        return exprPrimary(expr);
    } else {
        std::cerr << "[INTERNAL NAME MANGLING ERROR] template argument not supported!" << std::endl;
        std::exit(1);
    }
}

std::string ItaniumMangler::exprPrimary(const Expr *expr) {
    if (llvm::isa<IntegerLiteralExpr>(expr)) {
        auto integerLiteral = llvm::dyn_cast<IntegerLiteralExpr>(expr);
        // TODO: If the integer is negative it needs to be lead by an `n`
        // TODO: We need to convert to decimal if number base isn't 10
        return "L" + typeName(integerLiteral->resultType) + integerLiteral->numberString + "E";
    } else if (llvm::isa<FloatLiteralExpr>(expr)) {
        auto floatLiteral = llvm::dyn_cast<FloatLiteralExpr>(expr);
        return "L" + typeName(floatLiteral->resultType) + floatLiteral->numberValue() + "E";
    } else {
        std::cerr << "[INTERNAL NAME MANGLING ERROR] expr-primary not supported!" << std::endl;
        std::exit(1);
    }
}