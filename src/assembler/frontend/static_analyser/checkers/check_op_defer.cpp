/*
 *  Copyright (C) 2018 Marek Marecki
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
#include <viua/assembler/frontend/static_analyser.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_op_defer(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void {
    using viua::assembler::frontend::parser::AtomLiteral;
    using viua::assembler::frontend::parser::FunctionNameLiteral;

    auto fn = instruction.operands.at(0).get();
    if ((not dynamic_cast<AtomLiteral*>(fn))
        and (not dynamic_cast<FunctionNameLiteral*>(fn))
        and (not dynamic_cast<RegisterIndex*>(fn))) {
        throw invalid_syntax(instruction.operands.at(1)->tokens,
                             "invalid operand")
            .note("expected function name, atom literal, or register index");
    }
    if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
        check_use_of_register(register_usage_profile, *r);
        assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(
            register_usage_profile, *r);
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
