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
#include <viua/assert.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/kernel/registerset.h>
#include <viua/scheduler/vps.h>
#include <viua/types/closure.h>
#include <viua/types/function.h>
#include <viua/types/integer.h>
#include <viua/types/object.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::opnew(
    viua::internals::types::byte* addr) {
    /** Create new instance of specified class.
     */
    viua::kernel::Register* target = nullptr;
    tie(addr, target) =
        viua::bytecode::decoder::operands::fetch_register(addr, this);

    string class_name;
    tie(addr, class_name) =
        viua::bytecode::decoder::operands::fetch_atom(addr, this);

    if (not scheduler->is_class(class_name)) {
        throw make_unique<viua::types::Exception>(
            "cannot create new instance of unregistered type: " + class_name);
    }

    *target = make_unique<viua::types::Object>(class_name);

    return addr;
}

viua::internals::types::byte* viua::process::Process::opmsg(
    viua::internals::types::byte* addr) {
    /** Send a message to an object.
     *
     *  This instruction is used to perform a method call on an object using
     * dynamic dispatch. To call a method using static dispatch (where a correct
     * function is resolved during compilation) use "call" instruction.
     */
    bool return_void = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* return_register = nullptr;

    if (not return_void) {
        tie(addr, return_register) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    string method_name;
    auto ot = viua::bytecode::decoder::operands::get_operand_type(addr);
    if (ot == OT_REGISTER_INDEX or ot == OT_POINTER) {
        viua::types::Function* fn = nullptr;
        tie(addr, fn) = viua::bytecode::decoder::operands::fetch_object_of<
            viua::types::Function>(addr, this);

        method_name = fn->name();

        if (fn->type() == "Closure") {
            stack->frame_new->set_local_register_set(
                static_cast<viua::types::Closure*>(fn)->rs(), false);
        }
    } else {
        tie(addr, method_name) =
            viua::bytecode::decoder::operands::fetch_atom(addr, this);
    }

    auto obj = stack->frame_new->arguments->at(0);
    if (auto ptr = dynamic_cast<viua::types::Pointer*>(obj)) {
        obj = ptr->to(this);
    }
    if (not scheduler->is_class(obj->type())) {
        throw make_unique<viua::types::Exception>(
            "unregistered type cannot be used for dynamic dispatch: "
            + obj->type());
    }
    vector<string> mro = scheduler->inheritance_chain_of(obj->type());
    mro.insert(mro.begin(), obj->type());

    string function_name = "";
    for (decltype(mro.size()) i = 0; i < mro.size(); ++i) {
        if (not scheduler->is_class(mro[i])) {
            throw make_unique<viua::types::Exception>(
                "unavailable base type in inheritance hierarchy of " + mro[0]
                + ": " + mro[i]);
        }
        if (scheduler->class_accepts(mro[i], method_name)) {
            function_name = scheduler->resolve_method_name(mro[i], method_name);
            break;
        }
    }
    if (function_name.size() == 0) {
        throw make_unique<viua::types::Exception>("class '" + obj->type()
                                                  + "' does not accept method '"
                                                  + method_name + "'");
    }

    bool is_native         = scheduler->is_native_function(function_name);
    bool is_foreign        = scheduler->is_foreign_function(function_name);
    bool is_foreign_method = scheduler->is_foreign_method(function_name);

    if (not(is_native or is_foreign or is_foreign_method)) {
        throw make_unique<viua::types::Exception>(
            "method '" + method_name + "' resolves to undefined function '"
            + function_name + "' on class '" + obj->type() + "'");
    }

    if (is_foreign_method) {
        return call_foreign_method(
            addr, obj, function_name, return_register, method_name);
    }

    auto caller = (is_native ? &viua::process::Process::call_native
                             : &viua::process::Process::call_foreign);
    return (this->*caller)(addr, function_name, return_register, method_name);
}

viua::internals::types::byte* viua::process::Process::opinsert(
    viua::internals::types::byte* addr) {
    /** Insert an object as an attribute of another object.
     */
    viua::types::Object* object = nullptr;
    tie(addr, object) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Object>(
            addr, this);

    viua::types::String* key = nullptr;
    tie(addr, key) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::String>(
            addr, this);

    if (viua::bytecode::decoder::operands::get_operand_type(addr)
        == OT_POINTER) {
        viua::types::Value* source = nullptr;
        tie(addr, source) =
            viua::bytecode::decoder::operands::fetch_object(addr, this);
        object->insert(key->str(), source->copy());
    } else {
        viua::kernel::Register* source = nullptr;
        tie(addr, source) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
        object->insert(key->str(), source->give());
    }

    return addr;
}

viua::internals::types::byte* viua::process::Process::opremove(
    viua::internals::types::byte* addr) {
    /** Remove an attribute of another object.
     */
    bool void_target = viua::bytecode::decoder::operands::is_void(addr);
    viua::kernel::Register* target = nullptr;

    if (not void_target) {
        tie(addr, target) =
            viua::bytecode::decoder::operands::fetch_register(addr, this);
    } else {
        addr = viua::bytecode::decoder::operands::fetch_void(addr);
    }

    viua::types::Object* object = nullptr;
    tie(addr, object) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::Object>(
            addr, this);

    viua::types::String* key = nullptr;
    tie(addr, key) =
        viua::bytecode::decoder::operands::fetch_object_of<viua::types::String>(
            addr, this);

    unique_ptr<viua::types::Value> result{object->remove(key->str())};
    if (not void_target) {
        *target = std::move(result);
    }

    return addr;
}
