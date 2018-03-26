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

#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/kernel/registerset.h>
#include <viua/scheduler/vps.h>
#include <viua/types/integer.h>
#include <viua/types/prototype.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opclass(
    viua::internals::types::byte* addr) {
    /** Create a class.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    string class_name;
    tie(addr, class_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    *target = make_unique<viua::types::Prototype>(class_name);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opderive(
    viua::internals::types::byte* addr) {
    /** Push an ancestor class to prototype's inheritance chain.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    string class_name;
    tie(addr, class_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->is_class(class_name)) {
        throw make_unique<viua::types::Exception>(
            "cannot derive from unregistered type: " + class_name);
    }

    static_cast<viua::types::Prototype*>(target->get())->derive(class_name);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opattach(
    viua::internals::types::byte* addr) {
    /** Attach a function to a prototype as a method.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    string function_name, method_name;
    tie(addr, function_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);
    tie(addr, method_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    viua::types::Prototype* proto =
        static_cast<viua::types::Prototype*>(target->get());

    if (not(scheduler->is_native_function(function_name) or
            scheduler->is_foreign_function(function_name))) {
        throw make_unique<viua::types::Exception>(
            "cannot attach undefined function '" + function_name +
            "' as a method '" + method_name + "' of prototype '" +
            proto->get_type_name() + "'");
    }

    proto->attach(function_name, method_name);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opregister(
    viua::internals::types::byte* addr) {
    /** Register a prototype in the typesystem.
     */
    viua::kernel::Register* source = nullptr;
    tie(addr, source) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    unique_ptr<viua::types::Prototype> prototype{
        static_cast<viua::types::Prototype*>(source->release())};
    scheduler->register_prototype(std::move(prototype));

    return addr;
}
