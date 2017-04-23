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

#ifndef VIUA_TYPES_BOOLEAN_H
#define VIUA_TYPES_BOOLEAN_H

#pragma once

#include <string>
#include <sstream>
#include <viua/types/type.h>


namespace viua {
    namespace types {
        class Boolean : public viua::types::Type {
            /** Boolean object.
             *
             *  This type is used to hold true and false values.
             */
            bool b;

            public:
                static const std::string type_name;

                std::string type() const override {
                    return "Boolean";
                }
                std::string str() const override {
                    return ( b ? "true" : "false" );
                }
                bool boolean() const override {
                    return b;
                }

                bool& value() { return b; }

                virtual std::vector<std::string> bases() const override {
                    return std::vector<std::string>{"Number"};
                }
                virtual std::vector<std::string> inheritancechain() const override {
                    return std::vector<std::string>{"Number", "Type"};
                }

                std::unique_ptr<Type> copy() const override {
                    return std::unique_ptr<viua::types::Type>{new Boolean(b)};
                }

                Boolean(bool v = false): b(v) {}
        };
    }
}


#endif
