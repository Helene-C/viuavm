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

#include <functional>
#include <memory>
#include <viua/assert.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
#include <viua/types/value.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opizero(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    *target = make_unique<viua::types::Integer>(0);
    return addr;
}

viua::internals::types::byte* viua::process::Process::opistore(viua::internals::types::byte* addr) {
    viua::kernel::Register* target = nullptr;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register(addr, this);

    int integer = 0;
    tie(addr, integer) = viua::bytecode::decoder::operands::fetch_primitive_int(addr, this);

    *target = make_unique<viua::types::Integer>(integer);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opiinc(viua::internals::types::byte* addr) {
    viua::types::Integer* target{nullptr};
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    target->increment();

    return addr;
}

viua::internals::types::byte* viua::process::Process::opidec(viua::internals::types::byte* addr) {
    viua::types::Integer* target{nullptr};
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object_of<viua::types::Integer>(addr, this);

    target->decrement();

    return addr;
}
