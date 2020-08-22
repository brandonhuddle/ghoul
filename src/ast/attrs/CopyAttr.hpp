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
#ifndef GULC_COPYATTR_HPP
#define GULC_COPYATTR_HPP

#include <ast/Attr.hpp>

namespace gulc {
    /**
     * The `@copy` attribute makes it so the struct it is attached to is copy-by-default instead of move-by-default
     */
    class CopyAttr : public Attr {
    public:
        static bool classof(const Attr* attr) { return attr->getAttrKind() == Attr::Kind::Copy; }

        CopyAttr(TextPosition startPosition, TextPosition endPosition)
                : Attr(Attr::Kind::Copy, startPosition, endPosition) {}

        Attr* deepCopy() const override {
            return new CopyAttr(_startPosition, _endPosition);
        }

    };
}

#endif //GULC_COPYATTR_HPP