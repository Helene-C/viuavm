/*
 *  Copyright (C) 2017 Marek Marecki
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

#include <string>
#include <viua/types/number.h>
using namespace std;


const string viua::types::numeric::Number::type_name = "Number";

string viua::types::numeric::Number::type() const {
    return "Number";
}

bool viua::types::numeric::Number::negative() const {
    return (as_integer() < 0);
}

viua::types::numeric::Number::Number() {}

viua::types::numeric::Number::~Number() {}
