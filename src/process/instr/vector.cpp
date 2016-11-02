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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/assert.h>
#include <viua/types/type.h>
#include <viua/types/integer.h>
#include <viua/types/vector.h>
#include <viua/kernel/registerset.h>
#include <viua/kernel/kernel.h>
using namespace std;


byte* viua::process::Process::opvec(byte* addr) {
    unsigned register_index = 0, pack_start_index = 0, pack_length = 0;
    tie(addr, register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, pack_start_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, pack_length) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    if ((register_index > pack_start_index) and (register_index < (pack_start_index+pack_length))) {
        // FIXME vector is inserted into a register after packing, so this exception is not entirely well thought-out
        // allow packing target register
        throw new viua::types::Exception("vec would pack itself");
    }
    if ((pack_start_index+pack_length) >= uregset->size()) {
        throw new viua::types::Exception("vec: packing outside of register set range");
    }
    for (decltype(pack_length) i = 0; i < pack_length; ++i) {
        if (uregset->at(pack_start_index+i) == nullptr) {
            throw new viua::types::Exception("vec: cannot pack null register");
        }
    }

    unique_ptr<viua::types::Vector> v(new viua::types::Vector());
    for (decltype(pack_length) i = 0; i < pack_length; ++i) {
        v->push(pop(pack_start_index+i));
    }

    place(register_index, v.release());

    return addr;
}

byte* viua::process::Process::opvinsert(byte* addr) {
    /*  Run vinsert instruction.
     */
    viua::types::Type* vector_operand = nullptr;
    unsigned object_operand_index = 0, position_operand_index = 0;

    tie(addr, vector_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, object_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, position_operand_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::assertions::assert_implements<viua::types::Vector>(vector_operand, "viua::types::Vector");

    static_cast<viua::types::Vector*>(vector_operand)->insert(position_operand_index, pop(object_operand_index));

    return addr;
}

byte* viua::process::Process::opvpush(byte* addr) {
    /*  Run vpush instruction.
     */
    viua::types::Type* target = nullptr;
    unsigned source = 0;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::assertions::assert_implements<viua::types::Vector>(target, "viua::types::Vector");
    static_cast<viua::types::Vector*>(target)->push(pop(source));

    return addr;
}

byte* viua::process::Process::opvpop(byte* addr) {
    /*  Run vpop instruction.
     */
    unsigned destination_register_index = 0;
    viua::types::Type* vector_operand = nullptr;
    int position_operand_index = 0;

    tie(addr, destination_register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, vector_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, position_operand_index) = viua::bytecode::decoder::operands::fetch_primitive_int(addr, this);

    viua::assertions::assert_implements<viua::types::Vector>(vector_operand, "viua::types::Vector");
    /*  1) fetch vector,
     *  2) pop value at given index,
     *  3) put it in a register,
     */
    viua::types::Type* ptr = static_cast<viua::types::Vector*>(vector_operand)->pop(position_operand_index);
    place(destination_register_index, ptr);

    return addr;
}

byte* viua::process::Process::opvat(byte* addr) {
    /*  Run vat instruction.
     *
     *  viua::types::Vector always returns a copy of the object in a register.
     */
    unsigned destination_register_index = 0;
    viua::types::Type* vector_operand = nullptr;
    int position_operand_index = 0;

    tie(addr, destination_register_index) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, vector_operand) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    tie(addr, position_operand_index) = viua::bytecode::decoder::operands::fetch_primitive_int(addr, this);

    viua::assertions::assert_implements<viua::types::Vector>(vector_operand, "viua::types::Vector");
    viua::types::Type* ptr = static_cast<viua::types::Vector*>(vector_operand)->at(position_operand_index);
    place(destination_register_index, ptr->copy());

    return addr;
}

byte* viua::process::Process::opvlen(byte* addr) {
    /*  Run vlen instruction.
     */
    unsigned target = 0;
    viua::types::Type* source = nullptr;

    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);
    tie(addr, source) = viua::bytecode::decoder::operands::fetch_object(addr, this);

    viua::assertions::assert_implements<viua::types::Vector>(source, "viua::types::Vector");
    place(target, new viua::types::Integer(static_cast<viua::types::Vector*>(source)->len()));

    return addr;
}
