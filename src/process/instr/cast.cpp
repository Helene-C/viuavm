/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/kernel/kernel.h>
#include <viua/types/exception.h>
#include <viua/types/float.h>
#include <viua/types/integer.h>
#include <viua/types/string.h>
#include <viua/types/value.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opitof(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Value* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = make_unique<viua::types::Float>(static_cast<viua::types::Integer*>(source)->as_float());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opftoi(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Value* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    *target = make_unique<viua::types::Integer>(static_cast<viua::types::Float*>(source)->as_integer());

    return addr;
}

viua::internals::types::byte* viua::process::Process::opstoi(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Value* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    int result_integer = 0;
    string supplied_string = static_cast<viua::types::String*>(source)->value();
    try {
        result_integer = std::stoi(supplied_string);
    } catch (const std::out_of_range& e) {
        throw new viua::types::Exception("out of range: " + supplied_string);
    } catch (const std::invalid_argument& e) {
        throw new viua::types::Exception("invalid argument: " + supplied_string);
    }

    *target = make_unique<viua::types::Integer>(result_integer);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opstof(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Value* source = nullptr;
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    string supplied_string = static_cast<viua::types::String*>(source)->value();
    double convert_from = std::stod(supplied_string);
    *target = make_unique<viua::types::Float>(convert_from);

    return addr;
}
