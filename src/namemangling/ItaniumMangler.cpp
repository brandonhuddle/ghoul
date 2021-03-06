/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <ast/types/BuiltInType.hpp>
#include <ast/types/EnumType.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <iostream>
#include <ast/types/TraitType.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include "ItaniumMangler.hpp"

void gulc::ItaniumMangler::mangleDecl(gulc::EnumDecl* enumDecl) {
    mangleDeclEnum(enumDecl, "", "");
}

void gulc::ItaniumMangler::mangleDecl(gulc::StructDecl* structDecl) {
    mangleDeclStruct(structDecl, "", "");
}

void gulc::ItaniumMangler::mangleDecl(gulc::TraitDecl* traitDecl) {
    mangleDeclTrait(traitDecl, "", "");
}

void gulc::ItaniumMangler::mangleDecl(gulc::NamespaceDecl* namespaceDecl) {
    mangleDeclNamespace(namespaceDecl, "");
}

void gulc::ItaniumMangler::mangleDecl(gulc::TemplateStructDecl* templateStructDecl) {
    mangleDeclTemplateStruct(templateStructDecl, "", "");
}

void gulc::ItaniumMangler::mangleDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    mangleDeclTemplateTrait(templateTraitDecl, "", "");
}

void gulc::ItaniumMangler::mangleDeclEnum(gulc::EnumDecl* enumDecl, std::string const& prefix,
                                          std::string const& nameSuffix) {
    std::string nPrefix = prefix + sourceName(enumDecl->identifier().name());
    enumDecl->setMangledName(nPrefix + nameSuffix);

    // TODO: Support nested `Struct` and `Trait`
    for (Decl* decl : enumDecl->ownedMembers()) {
//        if (llvm::isa<OperatorDecl>(decl)) {
//            mangleOperator(llvm::dyn_cast<OperatorDecl>(decl), "N" + nPrefix, "E");
//        } else if (llvm::isa<CastOperatorDecl>(decl)) {
//            mangleCastOperator(llvm::dyn_cast<CastOperatorDecl>(decl), "N" + nPrefix, "E");
//        } else
        if (llvm::isa<CallOperatorDecl>(decl)) {
            mangleCallOperator(llvm::dyn_cast<CallOperatorDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<FunctionDecl>(decl)) {
            mangleFunction(llvm::dyn_cast<FunctionDecl>(decl), "N" + nPrefix, "E");
        }
    }
}

void gulc::ItaniumMangler::mangleDeclStruct(gulc::StructDecl* structDecl, std::string const& prefix,
                                            std::string const& nameSuffix) {
    std::string nPrefix = prefix + sourceName(structDecl);
    structDecl->setMangledName(nPrefix + nameSuffix);
}

void gulc::ItaniumMangler::mangleDeclTrait(gulc::TraitDecl* traitDecl, std::string const& prefix,
                                           std::string const& nameSuffix) {
    std::string nPrefix = prefix + sourceName(traitDecl);
    traitDecl->setMangledName(nPrefix + nameSuffix);
}

void gulc::ItaniumMangler::mangleDeclNamespace(gulc::NamespaceDecl* namespaceDecl, std::string const& prefix) {
    std::string nPrefix = prefix + sourceName(namespaceDecl->identifier().name());

    for (Decl* decl : namespaceDecl->nestedDecls()) {
        if (llvm::isa<EnumDecl>(decl)) {
            mangleDeclEnum(llvm::dyn_cast<EnumDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<NamespaceDecl>(decl)) {
            mangleDeclNamespace(llvm::dyn_cast<NamespaceDecl>(decl), nPrefix);
        } else if (llvm::isa<StructDecl>(decl)) {
            mangleDeclStruct(llvm::dyn_cast<StructDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<TraitDecl>(decl)) {
            mangleDeclTrait(llvm::dyn_cast<TraitDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<TemplateStructDecl>(decl)) {
            mangleDeclTemplateStruct(llvm::dyn_cast<TemplateStructDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<TemplateTraitDecl>(decl)) {
            mangleDeclTemplateTrait(llvm::dyn_cast<TemplateTraitDecl>(decl), "N" + nPrefix, "E");
        }
    }
}

void gulc::ItaniumMangler::mangleDeclTemplateStruct(gulc::TemplateStructDecl* templateStructDecl,
                                                    std::string const& prefix, std::string const& nameSuffix) {
    for (auto templateInst : templateStructDecl->templateInstantiations()) {
        mangleDeclTemplateStructInst(templateInst, prefix, nameSuffix);
    }
}

void gulc::ItaniumMangler::mangleDeclTemplateTrait(gulc::TemplateTraitDecl* templateTraitDecl,
                                                   std::string const& prefix, std::string const& nameSuffix) {
    for (auto templateInst : templateTraitDecl->templateInstantiations()) {
        mangleDeclTemplateTraitInst(templateInst, prefix, nameSuffix);
    }
}

void gulc::ItaniumMangler::mangleDeclTemplateStructInst(gulc::TemplateStructInstDecl* templateStructInstDecl,
                                                        std::string const& prefix, std::string const& nameSuffix) {
    // The `sourceName(StructDecl*)` function will handle setting the proper template struct instantiation name.
    mangleDeclStruct(templateStructInstDecl, prefix, nameSuffix);
}

void gulc::ItaniumMangler::mangleDeclTemplateTraitInst(gulc::TemplateTraitInstDecl* templateTraitInstDecl,
                                                       std::string const& prefix, std::string const& nameSuffix) {
    // The `sourceName(TraitDecl*)` function will handle setting the proper template trait instantiation name.
    mangleDeclTrait(templateTraitInstDecl, prefix, nameSuffix);
}

void gulc::ItaniumMangler::mangle(gulc::FunctionDecl* functionDecl) {
    mangleFunction(functionDecl, "", "");
}

void gulc::ItaniumMangler::mangle(gulc::VariableDecl* variableDecl) {
    mangleVariable(variableDecl, "", "");
}

void gulc::ItaniumMangler::mangle(gulc::NamespaceDecl* namespaceDecl) {
    mangleNamespace(namespaceDecl, "");
}

void gulc::ItaniumMangler::mangle(gulc::StructDecl* structDecl) {
    mangleStruct(structDecl, "");
}

void gulc::ItaniumMangler::mangle(gulc::TraitDecl* traitDecl) {
    mangleTrait(traitDecl, "");
}

void gulc::ItaniumMangler::mangle(gulc::CallOperatorDecl* callOperatorDecl) {
    mangleCallOperator(callOperatorDecl, "", "");
}

void gulc::ItaniumMangler::mangle(gulc::PropertyDecl* propertyDecl) {
    mangleProperty(propertyDecl, "", "");
}

void gulc::ItaniumMangler::mangle(gulc::TemplateStructDecl* templateStructDecl) {
    mangleTemplateStruct(templateStructDecl, "");
}

void gulc::ItaniumMangler::mangle(gulc::TemplateTraitDecl* templateTraitDecl) {
    mangleTemplateTrait(templateTraitDecl, "");
}

void gulc::ItaniumMangler::mangle(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    mangleTemplateFunction(templateFunctionDecl, "", "");
}

void gulc::ItaniumMangler::mangleFunction(gulc::FunctionDecl* functionDecl, std::string const& prefix,
                                          std::string const& nameSuffix) {
    // All mangled names start with "_Z"...
    std::string mangledName = "_Z" + prefix + unqualifiedName(functionDecl) + nameSuffix;

    if (llvm::isa<TemplateFunctionInstDecl>(functionDecl)) {
        auto templateInst = llvm::dyn_cast<TemplateFunctionInstDecl>(functionDecl);

        mangledName += templateArgs(templateInst->parentTemplateStruct()->templateParameters(),
                                    templateInst->templateArguments());
    }

    mangledName += bareFunctionType(functionDecl->parameters());

    functionDecl->setMangledName(mangledName);

    // TODO: At some point we will need to account for a function that checks the contracts and one that doesn't
    //       We need that because the compiler will optimize out some contracts, which will require us to move the
    //       contracts outside of the function. But the default function should handle the contracts to allow calling
    //       the function from C
}

void gulc::ItaniumMangler::mangleTemplateFunction(gulc::TemplateFunctionDecl* templateFunctionDecl,
                                                  std::string const& prefix, std::string const& nameSuffix) {
    for (auto templateInst : templateFunctionDecl->templateInstantiations()) {
        // `mangleFunction` handles template arguments properly for us
        mangleFunction(templateInst, prefix, nameSuffix);
    }
}

void gulc::ItaniumMangler::mangleVariable(gulc::VariableDecl* variableDecl, std::string const& prefix,
                                          std::string const& nameSuffix) {
    // All mangled names start with "_Z"...
    variableDecl->setMangledName("_Z" + prefix + unqualifiedName(variableDecl) + nameSuffix);
}

void gulc::ItaniumMangler::mangleNamespace(gulc::NamespaceDecl* namespaceDecl, std::string const& prefix) {
    std::string nPrefix = prefix + sourceName(namespaceDecl->identifier().name());

    for (Decl* decl : namespaceDecl->nestedDecls()) {
        if (llvm::isa<FunctionDecl>(decl)) {
            mangleFunction(llvm::dyn_cast<FunctionDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<VariableDecl>(decl)) {
            mangleVariable(llvm::dyn_cast<VariableDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<NamespaceDecl>(decl)) {
            mangleNamespace(llvm::dyn_cast<NamespaceDecl>(decl), nPrefix);
        } else if (llvm::isa<StructDecl>(decl)) {
            mangleStruct(llvm::dyn_cast<StructDecl>(decl), nPrefix);
        } else if (llvm::isa<TraitDecl>(decl)) {
            mangleTrait(llvm::dyn_cast<TraitDecl>(decl), nPrefix);
        } else if (llvm::isa<PropertyDecl>(decl)) {
            mangleProperty(llvm::dyn_cast<PropertyDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<TemplateStructDecl>(decl)) {
            mangleTemplateStruct(llvm::dyn_cast<TemplateStructDecl>(decl), nPrefix);
        } else if (llvm::isa<TemplateTraitDecl>(decl)) {
            mangleTemplateTrait(llvm::dyn_cast<TemplateTraitDecl>(decl), nPrefix);
        } else if (llvm::isa<TemplateFunctionDecl>(decl)) {
            mangleTemplateFunction(llvm::dyn_cast<TemplateFunctionDecl>(decl), "N" + nPrefix, "E");
        }
    }
}

void gulc::ItaniumMangler::mangleStruct(gulc::StructDecl* structDecl, std::string const& prefix) {
    std::string nPrefix = prefix + sourceName(structDecl);

    for (ConstructorDecl* constructor : structDecl->constructors()) {
        mangleConstructor(constructor, "N" + nPrefix, "E");
    }

    // TODO: Support nested `Struct` and `Trait`
    for (Decl* decl : structDecl->ownedMembers()) {
//        if (llvm::isa<OperatorDecl>(decl)) {
//            mangleOperator(llvm::dyn_cast<OperatorDecl>(decl), "N" + nPrefix, "E");
//        } else if (llvm::isa<CastOperatorDecl>(decl)) {
//            mangleCastOperator(llvm::dyn_cast<CastOperatorDecl>(decl), "N" + nPrefix, "E");
//        } else
        if (llvm::isa<CallOperatorDecl>(decl)) {
            mangleCallOperator(llvm::dyn_cast<CallOperatorDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<OperatorDecl>(decl)) {
            mangleOperator(llvm::dyn_cast<OperatorDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<FunctionDecl>(decl)) {
            mangleFunction(llvm::dyn_cast<FunctionDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<PropertyDecl>(decl)) {
            mangleProperty(llvm::dyn_cast<PropertyDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<SubscriptOperatorDecl>(decl)) {
            mangleSubscript(llvm::dyn_cast<SubscriptOperatorDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<TemplateFunctionDecl>(decl)) {
            mangleTemplateFunction(llvm::dyn_cast<TemplateFunctionDecl>(decl), "N" + nPrefix, "E");
        }
    }

    if (structDecl->destructor != nullptr) {
        mangleDestructor(structDecl->destructor, "N" + nPrefix, "E");
    }

    // Set vtable mangled name
    if (prefix.empty()) {
        structDecl->vtableName = "_ZTVN" + nPrefix + "E";
    } else {
        structDecl->vtableName = "_ZTV" + nPrefix;
    }
}

void gulc::ItaniumMangler::mangleTemplateStruct(gulc::TemplateStructDecl* templateStructDecl,
                                                std::string const& prefix) {
    for (auto templateInst : templateStructDecl->templateInstantiations()) {
        // `mangleStruct` handles template arguments properly for us
        mangleStruct(templateInst, prefix);
    }
}

void gulc::ItaniumMangler::mangleTrait(gulc::TraitDecl* traitDecl, std::string const& prefix) {
    std::string nPrefix = prefix + sourceName(traitDecl);

    // TODO: Support nested `Struct` and `Trait`
    for (Decl* decl : traitDecl->ownedMembers()) {
//        if (llvm::isa<OperatorDecl>(decl)) {
//            mangleOperator(llvm::dyn_cast<OperatorDecl>(decl), "N" + nPrefix, "E");
//        } else if (llvm::isa<CastOperatorDecl>(decl)) {
//            mangleCastOperator(llvm::dyn_cast<CastOperatorDecl>(decl), "N" + nPrefix, "E");
//        } else
        if (llvm::isa<CallOperatorDecl>(decl)) {
            mangleCallOperator(llvm::dyn_cast<CallOperatorDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<OperatorDecl>(decl)) {
            mangleOperator(llvm::dyn_cast<OperatorDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<FunctionDecl>(decl)) {
            mangleFunction(llvm::dyn_cast<FunctionDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<PropertyDecl>(decl)) {
            mangleProperty(llvm::dyn_cast<PropertyDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<SubscriptOperatorDecl>(decl)) {
            mangleSubscript(llvm::dyn_cast<SubscriptOperatorDecl>(decl), "N" + nPrefix, "E");
        } else if (llvm::isa<TemplateFunctionDecl>(decl)) {
            mangleTemplateFunction(llvm::dyn_cast<TemplateFunctionDecl>(decl), "N" + nPrefix, "E");
        }
    }
}

void gulc::ItaniumMangler::mangleTemplateTrait(gulc::TemplateTraitDecl* templateTraitDecl,
                                               std::string const& prefix) {
    for (auto templateInst : templateTraitDecl->templateInstantiations()) {
        // `mangleTrait` handles template arguments properly for us
        mangleTrait(templateInst, prefix);
    }
}

void gulc::ItaniumMangler::mangleCallOperator(gulc::CallOperatorDecl* callOperatorDecl, std::string const& prefix,
                                              std::string const& nameSuffix) {
    std::string mangledName = "_Z" + prefix + "cl" + nameSuffix;

    mangledName += bareFunctionType(callOperatorDecl->parameters());

    callOperatorDecl->setMangledName(mangledName);
}

void gulc::ItaniumMangler::mangleOperator(gulc::OperatorDecl* operatorDecl, std::string const& prefix,
                                          std::string const& nameSuffix) {
    // All mangled names start with "_Z"...
    std::string mangledName = "_Z" + prefix +
            operatorName(operatorDecl->operatorType(), operatorDecl->operatorIdentifier().name()) + nameSuffix;

    mangledName += bareFunctionType(operatorDecl->parameters());

    operatorDecl->setMangledName(mangledName);

    // TODO: At some point we will need to account for a function that checks the contracts and one that doesn't
    //       We need that because the compiler will optimize out some contracts, which will require us to move the
    //       contracts outside of the function. But the default function should handle the contracts to allow calling
    //       the function from C
}

void gulc::ItaniumMangler::mangleProperty(gulc::PropertyDecl* propertyDecl, std::string const& prefix,
                                          std::string const& nameSuffix) {
    std::string nPrefix = prefix + sourceName(propertyDecl->identifier().name());

    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        manglePropertyGet(getter, nPrefix, nameSuffix);
    }

    if (propertyDecl->hasSetter()) {
        manglePropertySet(propertyDecl->setter(), nPrefix, nameSuffix);
    }
}

void gulc::ItaniumMangler::manglePropertyGet(gulc::PropertyGetDecl* propertyGetDecl, std::string const& prefix,
                                             std::string const& nameSuffix) {
    switch (propertyGetDecl->getResultType()) {
        case PropertyGetDecl::GetResult::Normal:
            if (propertyGetDecl->isMutable()) {
                std::string mangledName = "_Z" + prefix + "pg" + nameSuffix + "v";
                propertyGetDecl->setMangledName(mangledName);
            } else {
                // `K` for C++ const/immut
                std::string mangledName = "_Z" + prefix + "Kpg" + nameSuffix + "v";
                propertyGetDecl->setMangledName(mangledName);
            }
            break;
        case PropertyGetDecl::GetResult::Ref:
            if (propertyGetDecl->isMutable()) {
                std::string mangledName = "_Z" + prefix + "pgr" + nameSuffix + "v";
                propertyGetDecl->setMangledName(mangledName);
            } else {
                // `K` for C++ const/immut
                std::string mangledName = "_Z" + prefix + "Kpgr" + nameSuffix + "v";
                propertyGetDecl->setMangledName(mangledName);
            }
            break;
        case PropertyGetDecl::GetResult::RefMut:
            if (propertyGetDecl->isMutable()) {
                std::string mangledName = "_Z" + prefix + "pgrm" + nameSuffix + "v";
                propertyGetDecl->setMangledName(mangledName);
            } else {
                // `K` for C++ const/immut
                std::string mangledName = "_Z" + prefix + "Kpgrm" + nameSuffix + "v";
                propertyGetDecl->setMangledName(mangledName);
            }
            break;
    }
}

void gulc::ItaniumMangler::manglePropertySet(gulc::PropertySetDecl* propertySetDecl, std::string const& prefix,
                                             std::string const& nameSuffix) {
    std::string mangledName = "_Z" + prefix + "ps" + nameSuffix;

    mangledName += bareFunctionType(propertySetDecl->parameters());

    propertySetDecl->setMangledName(mangledName);
}

void gulc::ItaniumMangler::mangleSubscript(gulc::SubscriptOperatorDecl* subscriptOperatorDecl,
                                           std::string const& prefix, std::string const& nameSuffix) {
    std::string nPrefix = prefix + sourceName(subscriptOperatorDecl->identifier().name());

    for (SubscriptOperatorGetDecl* getter : subscriptOperatorDecl->getters()) {
        mangleSubscriptGet(getter, nPrefix, nameSuffix);
    }

    if (subscriptOperatorDecl->hasSetter()) {
        mangleSubscriptSet(subscriptOperatorDecl->setter(), nPrefix, nameSuffix);
    }
}

void gulc::ItaniumMangler::mangleSubscriptGet(gulc::SubscriptOperatorGetDecl* subscriptOperatorGetDecl,
                                              std::string const& prefix, std::string const& nameSuffix) {
    switch (subscriptOperatorGetDecl->getResultType()) {
        case SubscriptOperatorGetDecl::GetResult::Normal:
            if (subscriptOperatorGetDecl->isMutable()) {
                std::string mangledName = "_Z" + prefix + "ixg" + nameSuffix;
                mangledName += bareFunctionType(subscriptOperatorGetDecl->parameters());
                subscriptOperatorGetDecl->setMangledName(mangledName);
            } else {
                // `K` for C++ const/immut
                std::string mangledName = "_Z" + prefix + "Kixg" + nameSuffix;
                mangledName += bareFunctionType(subscriptOperatorGetDecl->parameters());
                subscriptOperatorGetDecl->setMangledName(mangledName);
            }
            break;
        case SubscriptOperatorGetDecl::GetResult::Ref:
            if (subscriptOperatorGetDecl->isMutable()) {
                std::string mangledName = "_Z" + prefix + "ixgr" + nameSuffix;
                mangledName += bareFunctionType(subscriptOperatorGetDecl->parameters());
                subscriptOperatorGetDecl->setMangledName(mangledName);
            } else {
                // `K` for C++ const/immut
                std::string mangledName = "_Z" + prefix + "Kixgr" + nameSuffix;
                mangledName += bareFunctionType(subscriptOperatorGetDecl->parameters());
                subscriptOperatorGetDecl->setMangledName(mangledName);
            }
            break;
        case SubscriptOperatorGetDecl::GetResult::RefMut:
            if (subscriptOperatorGetDecl->isMutable()) {
                std::string mangledName = "_Z" + prefix + "ixgrm" + nameSuffix;
                mangledName += bareFunctionType(subscriptOperatorGetDecl->parameters());
                subscriptOperatorGetDecl->setMangledName(mangledName);
            } else {
                // `K` for C++ const/immut
                std::string mangledName = "_Z" + prefix + "Kixgrm" + nameSuffix;
                mangledName += bareFunctionType(subscriptOperatorGetDecl->parameters());
                subscriptOperatorGetDecl->setMangledName(mangledName);
            }
            break;
    }
}

void gulc::ItaniumMangler::mangleSubscriptSet(gulc::SubscriptOperatorSetDecl* subscriptOperatorSetDecl,
                                              std::string const& prefix, std::string const& nameSuffix) {
    std::string mangledName = "_Z" + prefix + "ixs" + nameSuffix;

    mangledName += bareFunctionType(subscriptOperatorSetDecl->parameters());

    subscriptOperatorSetDecl->setMangledName(mangledName);
}

void gulc::ItaniumMangler::mangleConstructor(gulc::ConstructorDecl* constructorDecl, std::string const& prefix,
                                             std::string const& nameSuffix) {
    // All mangled names start with "_Z"...
    std::string mangledName = "_Z" + prefix + "C2";
    std::string mangledNameVTable = "_Z" + prefix + "C1";

    mangledName += nameSuffix;
    mangledNameVTable += nameSuffix;

    std::string bareFunctionTypeResult;

    switch (constructorDecl->constructorType()) {
        case ConstructorType::Normal:
            bareFunctionTypeResult = bareFunctionType(constructorDecl->parameters());
            break;
        case ConstructorType::Copy:
            bareFunctionTypeResult = "RKS_";
            break;
        case ConstructorType::Move:
            bareFunctionTypeResult = "OS_";
            break;
    }

    // We only have to use <bare-function-name> since there isn't a namespace yet.
    mangledName += bareFunctionTypeResult;
    mangledNameVTable += bareFunctionTypeResult;

    constructorDecl->setMangledName(mangledName);
    constructorDecl->setMangledNameVTable(mangledNameVTable);
}

void gulc::ItaniumMangler::mangleDestructor(gulc::DestructorDecl* destructorDecl, std::string const& prefix,
                                            std::string const& nameSuffix) {
    // All mangled names start with "_Z"...
    std::string mangledName = "_Z" + prefix;

    mangledName += "D2";

    mangledName += nameSuffix;

    // We only have to use <bare-function-name> since there isn't a namespace yet.
    // NOTE: Destructors cannot have parameters but are considered functions so they have to have the 'v' specifier to
    //  show it doesn't accept any parameters here
    mangledName += "v";

    destructorDecl->setMangledName(mangledName);
}

std::string gulc::ItaniumMangler::unqualifiedName(gulc::FunctionDecl* functionDecl) {
    return sourceName(functionDecl->identifier().name());
}

std::string gulc::ItaniumMangler::unqualifiedName(gulc::VariableDecl* variableDecl) {
    return sourceName(variableDecl->identifier().name());
}

std::string gulc::ItaniumMangler::sourceName(std::string const& s) {
    return std::to_string(s.length()) + s;
}

std::string gulc::ItaniumMangler::sourceName(gulc::StructDecl* structDecl) {
    auto result = sourceName(structDecl->identifier().name());

    // If the struct is a template instantiation we add the template arguments to the end of the source-name
    if (llvm::isa<TemplateStructInstDecl>(structDecl)) {
        auto templateStructInst = llvm::dyn_cast<TemplateStructInstDecl>(structDecl);

        result += templateArgs(templateStructInst->parentTemplateStruct()->templateParameters(),
                templateStructInst->templateArguments());
    }

    return result;
}

std::string gulc::ItaniumMangler::sourceName(gulc::TraitDecl* traitDecl) {
    auto result = sourceName(traitDecl->identifier().name());

    // If the trait is a template instantiation we add the template arguments to the end of the source-name
    if (llvm::isa<TemplateTraitInstDecl>(traitDecl)) {
        auto templateTraitInstDecl = llvm::dyn_cast<TemplateTraitInstDecl>(traitDecl);

        result += templateArgs(templateTraitInstDecl->parentTemplateTrait()->templateParameters(),
                               templateTraitInstDecl->templateArguments());
    }

    return result;
}

std::string gulc::ItaniumMangler::bareFunctionType(std::vector<ParameterDecl*>& params) {
    std::string result;

    if (params.empty()) {
        return "v";
    }

    for (ParameterDecl* param : params) {
        // NOTE: I couldn't think of a better way to match the Itanium ABI while also supporting the argument labels
        //       so we use "U" for `vendor` qualifier then the actual argument source name. This might break some tools
        //       expecting C++ (which should be minimal, tools actually demangling themselves) but within a tool like
        //       "c++filt" this will output our Ghoul signature as a C++ signature that looks like valid C++.
        // TODO: Should we go the `Ual` route instead? Still use `U` so some tools should be ok but then `al` for
        //       "argument label" then the source name of the argument label. Leading us to: `Ual3arg`?
        result += "U" + sourceName(param->argumentLabel().name());

        if (param->parameterKind() == ParameterDecl::ParameterKind::In) {
            result+= "U2in";
        } else if (param->parameterKind() == ParameterDecl::ParameterKind::Out) {
            result+= "U3out";
        }

        // Parameters that reference template type parameters have to use the template reference strings `T_` and `T{n}_`
//        if (param->typeTemplateParamNumber > 0) {
//            if (param->typeTemplateParamNumber - 1 == 0) {
//                result += "T_";
//            } else {
//                result += "T" + std::to_string(param->typeTemplateParamNumber - 2) + "_";
//            }
//
//            continue;
//        }

        // TODO: Should we ignore `mut` here?
        result += typeName(param->type);
    }

    return result;
}

std::string gulc::ItaniumMangler::typeName(gulc::Type* type) {
    std::string prefix;

    if (type->qualifier() == Type::Qualifier::Immut) {
        prefix = "K";
    }

    if (llvm::isa<BuiltInType>(type)) {
        auto builtInType = llvm::dyn_cast<BuiltInType>(type);
        std::string const& checkName = builtInType->name();

        if (checkName == "void") {
            return prefix + "v";
        } else if (checkName == "bool") {
            return prefix + "b";
        } else {
            return prefix + sourceName(checkName);
        }
    } else if (llvm::isa<EnumType>(type)) {
        auto enumType = llvm::dyn_cast<EnumType>(type);

        // TODO: When do we add 'Te' in front of this?? Neither clang nor gcc seem to do it in my tests
        return prefix + /*"Te" + */enumType->decl()->mangledName();
    } else if (llvm::isa<StructType>(type)) {
        auto structType = llvm::dyn_cast<StructType>(type);

        if (structType->decl()->structKind() == StructDecl::Kind::Union) {
            // TODO: When do we add 'Tu' in front of this?? Neither clang nor gcc seem to do it in my tests
            return prefix + /*"Tu" + */structType->decl()->mangledName();
        } else {
            // TODO: When do we add 'Ts' in front of this?? Neither clang nor gcc seem to do it in my tests
            return prefix + /*"Ts" + */structType->decl()->mangledName();
        }
    } else if (llvm::isa<TraitType>(type)) {
        auto traitType = llvm::dyn_cast<TraitType>(type);

        // TODO: If we do `Te` and `Ts` then should we do `Tt`, `Ti`, or `Tp`? (for `trait`, `interface`, or `protocol`)
        return prefix + /*"Tt" + */traitType->decl()->mangledName();
    } else if (llvm::isa<PointerType>(type)) {
        return prefix + "P" + typeName(llvm::dyn_cast<PointerType>(type)->nestedType);
    } else if (llvm::isa<ReferenceType>(type)) {
        return prefix + "R" + typeName(llvm::dyn_cast<ReferenceType>(type)->nestedType);
    } else {
        std::cerr << "[INTERNAL NAME MANGLING ERROR] type `" << type->toString() << "` not supported!" << std::endl;
        std::exit(1);
    }

    return "[ERROR]";
}

std::string gulc::ItaniumMangler::templateArgs(std::vector<TemplateParameterDecl*>& templateParams,
                                               std::vector<Expr*>& templateArgs) {
    std::string result = "I";

    for (std::size_t i = 0; i < templateParams.size(); ++i) {
        const Expr* templateArgExpr = nullptr;

        if (i >= templateArgs.size()) {
            // TODO: Template default value
//            templateArgExpr = templateParams[i]->defaultArgument();
        } else {
            templateArgExpr = templateArgs[i];
        }

        result += templateArg(templateArgExpr);
    }

    return result + "E";
}

std::string gulc::ItaniumMangler::templateArg(gulc::Expr const* expr) {
    if (llvm::isa<TypeExpr>(expr)) {
        auto resolvedType = llvm::dyn_cast<TypeExpr>(expr);
        return typeName(resolvedType->type);
    } else if (llvm::isa<ValueLiteralExpr>(expr)) {
        return exprPrimary(expr);
    } else {
        std::cerr << "[INTERNAL NAME MANGLING ERROR] template argument not supported!" << std::endl;
        std::exit(1);
    }
}

std::string gulc::ItaniumMangler::exprPrimary(gulc::Expr const* expr) {
    if (llvm::isa<ValueLiteralExpr>(expr)) {
        auto valueLiteral = llvm::dyn_cast<ValueLiteralExpr>(expr);
        // TODO: If the integer is negative it needs to be lead by an `n`
        // TODO: We need to convert to decimal if number base isn't 10

        switch (valueLiteral->literalType()) {
            case ValueLiteralExpr::LiteralType::Integer:
            case ValueLiteralExpr::LiteralType::Float:
                return "L" + typeName(valueLiteral->valueType) + valueLiteral->value() + "E";
            default:
                break;
        }
    }

    std::cerr << "[INTERNAL NAME MANGLING ERROR] expr-primary not supported!" << std::endl;
    std::exit(1);
}

std::string gulc::ItaniumMangler::operatorName(gulc::OperatorType operatorType, std::string const& operatorText) {
    if (operatorType == OperatorType::Postfix) {
        if (operatorText == "++") {
            return "pp";
        } else if (operatorText == "--") {
            return "mm";
        }
    } else if (operatorType == OperatorType::Prefix) {
        if (operatorText == "++") {
            return "pp";
        } else if (operatorText == "--") {
            return "mm";
        } else if (operatorText == "+") {
            return "ps";
        } else if (operatorText == "-") {
            return "ng";
        } else if (operatorText == "!") {
            return "nt";
        } else if (operatorText == "~") {
            return "co";
        } else if (operatorText == "*") {
            return "de";
        } else if (operatorText == "&") {
            return "ad";
        }
    } else if (operatorType == OperatorType::Infix) {
        if (operatorText == "+") {
            return "pl";
        } else if (operatorText == "-") {
            return "mi";
        } else if (operatorText == "*") {
            return "ml";
        } else if (operatorText == "/") {
            return "dv";
        } else if (operatorText == "%") {
            return "rm";
        } else if (operatorText == "^^") {
            // TODO: Is this correct? It says "v" + operandCount + sourceName making it "v23pow"?
            return "v23pow";
        } else if (operatorText == "&") {
            return "an";
        } else if (operatorText == "|") {
            return "or";
        } else if (operatorText == "^") {
            return "eo";
        } else if (operatorText == "<<") {
            return "ls";
        } else if (operatorText == ">>") {
            return "rs";
        } else if (operatorText == "&&") {
            return "aa";
        } else if (operatorText == "||") {
            return "oo";
        } else if (operatorText == "==") {
            return "eq";
        } else if (operatorText == "!=") {
            return "ne";
        } else if (operatorText == ">") {
            return "gt";
        } else if (operatorText == "<") {
            return "lt";
        } else if (operatorText == ">=") {
            return "ge";
        } else if (operatorText == "<=") {
            return "le";
        } else if (operatorText == "<=>") {
            return "ss";
        }
    }

    std::cerr << "[INTERNAL NAME MANGLING ERROR] operator type not supported!" << std::endl;
    std::exit(1);
}
