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

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <viua/kernel/registerset.h>
#include <viua/types/exception.h>
#include <viua/types/integer.h>
#include <viua/types/reference.h>
#include <viua/types/value.h>
using namespace std;


void viua::kernel::Register::reset(unique_ptr<viua::types::Value> o) {
    if (dynamic_cast<viua::types::Reference*>(value.get())) {
        static_cast<viua::types::Reference*>(value.get())->rebind(o.release());
    } else {
        value = std::move(o);
    }
}

bool viua::kernel::Register::empty() const {
    return value == nullptr;
}

viua::types::Value* viua::kernel::Register::get() {
    return value.get();
}

viua::types::Value* viua::kernel::Register::release() {
    mask = 0;
    return value.release();
}

std::unique_ptr<viua::types::Value> viua::kernel::Register::give() {
    mask = 0;
    return std::move(value);
}

void viua::kernel::Register::swap(Register& that) {
    value.swap(that.value);
    // FIXME are masks still used?
    auto tmp  = mask;
    mask      = that.mask;
    that.mask = tmp;
}

mask_type viua::kernel::Register::set_mask(mask_type new_mask) {
    auto tmp = mask;
    mask     = new_mask;
    return tmp;
}

mask_type viua::kernel::Register::flag(mask_type new_mask) {
    auto tmp = mask;
    mask     = (mask | new_mask);
    return tmp;
}

mask_type viua::kernel::Register::unflag(mask_type new_mask) {
    auto tmp = mask;
    mask     = (mask ^ new_mask);
    return tmp;
}

mask_type viua::kernel::Register::get_mask() const {
    return mask;
}

bool viua::kernel::Register::is_flagged(mask_type filter) const {
    return (mask & filter);
}

viua::kernel::Register::Register() : value(nullptr), mask(0) {}

viua::kernel::Register::Register(std::unique_ptr<viua::types::Value> o)
        : value(std::move(o)), mask(0) {}

viua::kernel::Register::Register(Register&& that)
        : value(std::move(that.value)), mask(that.mask) {
    that.mask = 0;
}

viua::kernel::Register::operator bool() const {
    return not empty();
}

auto viua::kernel::Register::operator=(Register&& that) -> Register& {
    reset(std::move(that.value));
    mask      = that.mask;
    that.mask = 0;
    return *this;
}

auto viua::kernel::Register::operator=(decltype(value)&& o) -> Register& {
    reset(std::move(o));
    mask = 0;
    return *this;
}


void viua::kernel::RegisterSet::put(
    viua::internals::types::register_index index,
    unique_ptr<viua::types::Value> object) {
    if (index >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: write");
    }
    registers.at(index).reset(std::move(object));
}

unique_ptr<viua::types::Value> viua::kernel::RegisterSet::pop(
    viua::internals::types::register_index index) {
    /** Pop an object from the register.
     */
    unique_ptr<viua::types::Value> object{at(index)};
    if (not object) {
        // FIXME: throw an exception on read from empty register
    }
    empty(index);
    return object;
}

void viua::kernel::RegisterSet::set(
    viua::internals::types::register_index index,
    unique_ptr<viua::types::Value> object) {
    /** Put object inside register specified by given index.
     *
     *  Performs bounds checking.
     */
    if (index >= registers.size()) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: write");
    }

    if (dynamic_cast<viua::types::Reference*>(registers.at(index).get())) {
        static_cast<viua::types::Reference*>(registers.at(index).get())
            ->rebind(object.release());
    } else {
        registers.at(index).reset(std::move(object));
    }
}

viua::types::Value* viua::kernel::RegisterSet::get(
    viua::internals::types::register_index index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) {
        ostringstream emsg;
        emsg << "register access out of bounds: read from " << index;
        throw make_unique<viua::types::Exception>(emsg.str());
    }
    viua::types::Value* optr = registers.at(index).get();
    if (optr == nullptr) {
        ostringstream oss;
        oss << "(get) read from null register: " << index;
        throw make_unique<viua::types::Exception>(oss.str());
    }
    return optr;
}

viua::types::Value* viua::kernel::RegisterSet::at(
    viua::internals::types::register_index index) {
    /** Fetch object from register specified by given index.
     *
     *  Performs bounds checking.
     *  Returns 0 when accessing empty register.
     */
    if (index >= registerset_size) {
        ostringstream emsg;
        emsg << "register access out of bounds: read from " << index;
        throw make_unique<viua::types::Exception>(emsg.str());
    }
    return registers.at(index).get();
}

viua::kernel::Register* viua::kernel::RegisterSet::register_at(
    viua::internals::types::register_index index) {
    if (index >= registerset_size) {
        ostringstream emsg;
        emsg << "register access out of bounds: read from " << index;
        throw make_unique<viua::types::Exception>(emsg.str());
    }
    return &(registers.at(index));
}


void viua::kernel::RegisterSet::move(
    viua::internals::types::register_index src,
    viua::internals::types::register_index dst) {
    /** Move an object from src register to dst register.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: move source");
    }
    if (dst >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: move destination");
    }
    set(dst, pop(src));
}

void viua::kernel::RegisterSet::swap(
    viua::internals::types::register_index src,
    viua::internals::types::register_index dst) {
    /** Swap objects in src and dst registers.
     *
     *  Performs bound checking.
     *  Both source and destination can be empty.
     */
    if (src >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: swap source");
    }
    if (dst >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: swap destination");
    }
    registers[src].swap(registers[dst]);
}

void viua::kernel::RegisterSet::empty(
    viua::internals::types::register_index here) {
    /** Empty a register.
     *
     *  Performs bound checking.
     *  Does not throw if the register is empty.
     */
    if (here >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: empty");
    }
    registers.at(here).release();
}

void viua::kernel::RegisterSet::free(
    viua::internals::types::register_index here) {
    /** Free an object inside a register.
     *
     *  Performs bound checking.
     *  Throws if the register is empty.
     */
    if (here >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: free");
    }
    if (not registers.at(here)) {
        throw make_unique<viua::types::Exception>(
            "invalid free: trying to free a null pointer");
    }
    registers.at(here).give();
}


void viua::kernel::RegisterSet::flag(
    viua::internals::types::register_index index,
    mask_type filter) {
    /** Enable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: mask_enable");
    }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(flag) flagging null register: " << index;
        throw make_unique<viua::types::Exception>(oss.str());
    }
    registers.at(index).flag(filter);
}

void viua::kernel::RegisterSet::unflag(
    viua::internals::types::register_index index,
    mask_type filter) {
    /** Disable masks specified by filter for register at given index.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: mask_disable");
    }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(unflag) unflagging null register: " << index;
        throw make_unique<viua::types::Exception>(oss.str());
    }
    registers.at(index).unflag(filter);
}

void viua::kernel::RegisterSet::clear(
    viua::internals::types::register_index index) {
    /** Clear masks for given register.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: mask_clear");
    }
    registers.at(index).set_mask(0);
}

bool viua::kernel::RegisterSet::isflagged(
    viua::internals::types::register_index index,
    mask_type filter) {
    /** Returns true if given filter is enabled for register specified by given
     * index. Returns false otherwise.
     *
     *  Performs bounds checking.
     */
    if (index >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: mask_isenabled");
    }
    // FIXME: should throw when accessing empty register, but that breaks set()
    if (index >= registers.size()) {
        return registers.at(index).is_flagged(filter);
    } else {
        return false;
    }
}

void viua::kernel::RegisterSet::setmask(
    viua::internals::types::register_index index,
    mask_type mask) {
    /** Set mask for a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: mask_disable");
    }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(setmask) setting mask for null register: " << index;
        throw make_unique<viua::types::Exception>(oss.str());
    }
    registers.at(index).set_mask(mask);
}

mask_type viua::kernel::RegisterSet::getmask(
    viua::internals::types::register_index index) {
    /** Get mask of a register.
     *
     *  Performs bounds checking.
     *  Throws exception when accessing empty register.
     */
    if (index >= registerset_size) {
        throw make_unique<viua::types::Exception>(
            "register access out of bounds: mask_disable");
    }
    if (not registers.at(index)) {
        ostringstream oss;
        oss << "(getmask) getting mask of null register: " << index;
        throw make_unique<viua::types::Exception>(oss.str());
    }
    return registers.at(index).get_mask();
}


void viua::kernel::RegisterSet::drop() {
    /** Drop register set contents.
     *
     *  This function makes the register set drop all objects it holds by
     *  emptying all its available registers.
     *  No objects will be deleted, so use this function carefully.
     */
    for (decltype(size()) i = 0; i < size(); ++i) {
        empty(i);
    }
}


unique_ptr<viua::kernel::RegisterSet> viua::kernel::RegisterSet::copy() {
    auto rscopy = make_unique<viua::kernel::RegisterSet>(size());
    for (decltype(size()) i = 0; i < size(); ++i) {
        if (at(i) == nullptr) {
            continue;
        }
        rscopy->set(i, at(i)->copy());
        rscopy->setmask(i, getmask(i));
    }
    return rscopy;
}

viua::kernel::RegisterSet::RegisterSet(
    viua::internals::types::register_index sz)
        : registerset_size(sz) {
    /** Create register set with specified size.
     */
    registers.reserve(sz);

    for (decltype(sz) i = 0; i < sz; ++i) {
        registers.emplace_back(nullptr);
    }
}
viua::kernel::RegisterSet::~RegisterSet() {}
