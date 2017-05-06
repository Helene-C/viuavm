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

#include <string>
#include <vector>
#include <viua/types/value.h>
#include <viua/types/string.h>
#include <viua/types/vector.h>
#include <viua/types/exception.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/include/module.h>
using namespace std;


void typeof(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->arguments->at(0) == 0) {
        throw new viua::types::Exception("expected object as parameter 0");
    }
    frame->local_register_set->set(0, unique_ptr<viua::types::Value>{new viua::types::String(frame->arguments->get(0)->type())});
}

void inheritanceChain(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->arguments->at(0) == 0) {
        throw new viua::types::Exception("expected object as parameter 0");
    }

    vector<string> ic = frame->arguments->at(0)->inheritancechain();
    unique_ptr<viua::types::Vector> icv {new viua::types::Vector()};

    for (unsigned i = 0; i < ic.size(); ++i) {
        icv->push(unique_ptr<viua::types::Value>{new viua::types::String(ic[i])});
    }

    frame->local_register_set->set(0, std::move(icv));
}

void bases(Frame* frame, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*) {
    if (frame->arguments->at(0) == 0) {
        throw new viua::types::Exception("expected object as parameter 0");
    }

    viua::types::Value* object = frame->arguments->at(0);
    vector<string> ic = object->bases();
    unique_ptr<viua::types::Vector> icv {new viua::types::Vector()};

    for (unsigned i = 0; i < ic.size(); ++i) {
        icv->push(unique_ptr<viua::types::Value>{new viua::types::String(ic[i])});
    }

    frame->local_register_set->set(0, std::move(icv));
}


const ForeignFunctionSpec functions[] = {
    { "typesystem::typeof/1", &typeof },
    { "typesystem::inheritanceChain/1", &inheritanceChain },
    { "typesystem::bases/1", &bases },
    { NULL, NULL },
};

extern "C" const ForeignFunctionSpec* exports() {
    return functions;
}
