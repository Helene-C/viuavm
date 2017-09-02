/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIUA_OPERAND_TYPES_H
#define VIUA_OPERAND_TYPES_H

#include <stdint.h>

#pragma once

enum OperandType : uint8_t {
    OT_REGISTER_INDEX,            // register index
    OT_REGISTER_REFERENCE,        // register reference (indirect register index)
    OT_REGISTER_INDEX_ANNOTATED,  // register index with an annotation from which
                                  // register set it should be taken

    OT_POINTER,  // index of register containing a pointer that
                 // should be dereferenced (should be decoded to
                 // dereferenced value)

    OT_ATOM,    // null-terminated ASCII string
    OT_TEXT,    // UTF-8 encoded Unicode string, null-terminated
    OT_STRING,  // size-prefixed byte string
    OT_BITS,    // size-prefixed bit string

    OT_INT,    // unlimited size signed integer
    OT_INT8,   // 8bit signed integer
    OT_INT16,  // 16bit signed integer
    OT_INT32,  // 32bit signed integer
    OT_INT64,  // 64bit signed integer

    OT_UINT,    // unlimited size unsigned integer
    OT_UINT8,   // 8bit unsigned integer
    OT_UINT16,  // 16bit unsigned integer
    OT_UINT32,  // 32bit unsigned integer
    OT_UINT64,  // 64bit unsigned integer

    OT_FLOAT,    // unlimited precision floating point number
    OT_FLOAT32,  // 32bit floating point number
    OT_FLOAT64,  // 64bit floating point number

    OT_VOID,  // encodes the abstract concept of "nothing"

    OT_TRUE,   // encodes literal true
    OT_FALSE,  // encodes literal false
};

namespace viua {
    namespace internals {
        enum class AccessSpecifier {
            DIRECT,
            REGISTER_INDIRECT,
            POINTER_DEREFERENCE,
        };

        enum class ValueTypes {
            UNDEFINED = 0,
            VOID = 1 << 0,

            INTEGER = 1 << 2,
            FLOAT = 1 << 3,

            BOOLEAN = 1 << 4,

            TEXT,
        };
    }
}

#endif
