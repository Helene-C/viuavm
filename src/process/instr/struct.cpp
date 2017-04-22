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

#include <utility>
#include <viua/process.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/assert.h>
#include <viua/types/vector.h>
#include <viua/types/atom.h>
#include <viua/types/struct.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opstruct(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    *target = unique_ptr<viua::types::Type>{ new viua::types::Struct() };

    return addr;
}

viua::internals::types::byte* viua::process::Process::opstructinsert(viua::internals::types::byte* addr) {
    viua::types::Type *struct_operand = nullptr, *key_operand = nullptr;
    tie(addr, struct_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    tie(addr, key_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    viua::assertions::assert_implements<viua::types::Struct>(struct_operand, "viua::types::Struct");
    viua::assertions::assert_implements<viua::types::Atom>(key_operand, "viua::types::Atom");

    if (viua::bytecode::decoder::operands::get_operand_type(addr) == OT_POINTER) {
        viua::types::Type* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);
        static_cast<viua::types::Struct*>(struct_operand)->insert(*static_cast<viua::types::Atom*>(key_operand), source->copy());
    } else {
        viua::kernel::Register* source = nullptr;
        tie(addr, source) = viua::bytecode::decoder::operands::fetch_register(addr, this);
        static_cast<viua::types::Struct*>(struct_operand)->insert(*static_cast<viua::types::Atom*>(key_operand), source->give());
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opstructremove(viua::internals::types::byte* addr) {
    bool void_target = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* target = nullptr;

    if (not void_target) {
        tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Type *struct_operand = nullptr, *key_operand = nullptr;
    tie(addr, struct_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, key_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::assert_implements<viua::types::Struct>(struct_operand, "viua::types::Struct");
    viua::assertions::assert_typeof(key_operand, "viua::types::Atom");

    unique_ptr<viua::types::Type> result { static_cast<viua::types::Struct*>(struct_operand)->remove(*static_cast<viua::types::Atom*>(key_operand)) };
    if (not void_target) {
        *target = std::move(result);
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opstructkeys(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    viua::types::Type *struct_operand = nullptr;
    tie(addr, struct_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    viua::assertions::assert_implements<viua::types::Struct>(struct_operand, "viua::types::Struct");

    auto struct_keys = static_cast<viua::types::Struct*>(struct_operand)->keys();
    unique_ptr<viua::types::Vector> vec { new viua::types::Vector() };
    for (const auto& each : struct_keys) {
        vec->push(unique_ptr<viua::types::Atom>{ new viua::types::Atom(each) });
    }

    *target = std::move(vec);

    return addr;
}