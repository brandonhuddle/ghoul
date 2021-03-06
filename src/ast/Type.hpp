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
#ifndef GULC_TYPE_HPP
#define GULC_TYPE_HPP

#include <string>
#include "Node.hpp"

namespace gulc {
    class Type : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Type; }

        enum class Kind {
            Alias,
            Bool,
            BuiltIn,
            Dependent,
            Dimension,
            Enum,
            FlatArray,
            FunctionPointer,
            Imaginary,
            LabeledType,
            Pointer,
            Reference,
            Self,
            Struct,
            Templated,
            TemplateStruct,
            TemplateTrait,
            TemplateTypenameRef,
            Trait,
            Unresolved,
            UnresolvedNested,
            VTable
        };

        enum class Qualifier {
            Unassigned,
            Mut,
            Immut
        };

        Kind getTypeKind() const { return _typeKind; }
        Qualifier qualifier() const { return _qualifier; }
        /// NOTE: Function return values are always stored into temporary values, making them an `lvalue` but they are
        ///       ALWAYS `const`, making them an unassignable `lvalue`.
        bool isLValue() const { return _isLValue; }
        void setQualifier(Qualifier qualifier) { _qualifier = qualifier; }
        void setIsLValue(bool isLValue) { _isLValue = isLValue; }

        virtual std::string toString() const = 0;
        virtual Type* deepCopy() const = 0;

    protected:
        Kind _typeKind;
        Qualifier _qualifier;
        bool _isLValue;

        Type(Type::Kind typeKind, Qualifier qualifier, bool isLValue)
                : Type(Node::Kind::Type, typeKind, qualifier, isLValue) {}
        Type(Node::Kind nodeKind, Type::Kind typeKind, Qualifier qualifier, bool isLValue)
                : Node(nodeKind), _typeKind(typeKind), _qualifier(qualifier), _isLValue(isLValue) {}

    };
}

#endif //GULC_TYPE_HPP
