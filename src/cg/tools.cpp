/*
 *  Copyright (C) 2016, 2017 Marek Marecki
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
#include <tuple>
#include <viua/bytecode/bytetypedef.h>
#include <viua/support/string.h>
#include <viua/cg/tools.h>
using namespace std;


namespace viua {
    namespace cg {
        namespace tools {
            static auto looks_like_timeout(const viua::cg::lex::Token& token) -> bool {
                string s = token;
                if (s == "infinity") {
                    return true;
                }

                const auto size = s.size();
                if (size < 2) {
                    return false;
                }
                if (s.at(size-2) == 'm' and s.at(size-1) == 's' and str::isnum(s.substr(0, size-2))) {
                    return true;
                }
                if (s.at(size-1) == 's' and str::isnum(s.substr(0, size-1))) {
                    return true;
                }
                return false;
            }
            static auto size_of_register_index_operand_with_rs_type(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = 0;

                if (tokens.at(i) == "void") {
                    calculated_size += sizeof(viua::internals::types::byte);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '%' and str::isnum(tokens.at(i).str().substr(1))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;

                    if (tokens.at(i) == "current" or tokens.at(i) == "local" or tokens.at(i) == "static" or tokens.at(i) == "global") {
                        ++i;
                    }
                } else if (tokens.at(i).str().at(0) == '@') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;

                    if (tokens.at(i) == "current" or tokens.at(i) == "local" or tokens.at(i) == "static" or tokens.at(i) == "global") {
                        ++i;
                    }
                } else if (tokens.at(i).str().at(0) == '*') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;

                    if (tokens.at(i) == "current" or tokens.at(i) == "local" or tokens.at(i) == "static" or tokens.at(i) == "global") {
                        ++i;
                    }
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("invalid operand token: " + tokens.at(i).str()));
                }

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_register_index_operand(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = 0;

                if (tokens.at(i) == "static" or tokens.at(i) == "local" or tokens.at(i) == "global") {
                    ++i;
                }

                if (tokens.at(i) == "void") {
                    calculated_size += sizeof(viua::internals::types::byte);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '%' and str::isnum(tokens.at(i).str().substr(1))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '@') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '*') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("invalid operand token: " + tokens.at(i).str()));
                }

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }

            static auto size_of_instruction_with_one_ri_operand(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_one_ri_operand_with_rs_type(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_two_ri_operands_with_rs_types(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_two_ri_operands(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_three_ri_operands_with_rs_types(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 1st source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 2nd source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_four_ri_operands_with_rs_types(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 1st source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 2nd source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 3rd source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_three_ri_operands(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for 1st source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for 2nd source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_alu(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                ++i;
                auto sz = size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
                ++get<0>(sz);
                return sz;
            }

            static auto size_of_nop(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);
                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_izero(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_istore(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = 0;
                tie(calculated_size, i) = size_of_instruction_with_one_ri_operand(tokens, i);

                calculated_size += sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::plain_int);
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_iinc(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_idec(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_fstore(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                calculated_size += sizeof(viua::internals::types::plain_float);
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_itof(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ftoi(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_stoi(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_stof(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_add(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_sub(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_mul(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_div(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_lt(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_lte(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_gt(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_gte(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_eq(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_strstore(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i++).str().size() + 1 - 2; // +1 for null terminator, -2 for quotes

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_streq(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands(tokens, i);
            }

            static auto size_of_text(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i++).str().size() + 1 - 2; // +1 for null terminator, -2 for quotes

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_texteq(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textat(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textsub(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_four_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textlength(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textcommonprefix(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textcommonsuffix(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textconcat(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }

            static auto size_of_vec(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for pack start register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for pack count register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_vinsert(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for position index
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_vpush(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_vpop(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_vat(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_vlen(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bool(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_not(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_and(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_or(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_move(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_copy(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ptr(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_swap(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_delete(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_isnull(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ress(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::registerset_type_marker);
                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_print(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_echo(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_capture(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for inside-closure register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_capturecopy(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for inside-closure register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_capturemove(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for inside-closure register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_closure(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_function(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_frame(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands(tokens, i);
            }
            static auto size_of_param(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_pamv(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_call(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                    calculated_size += size_increment;
                } else {
                    calculated_size += tokens.at(i).str().size() + 1;
                    ++i;
                }

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_tailcall(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    decltype(calculated_size) size_increment = 0;
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                    calculated_size += size_increment;
                } else {
                    calculated_size += tokens.at(i).str().size() + 1;
                    ++i;
                }

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_arg(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_argc(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_process(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_self(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_join(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = 0;
                tie(calculated_size, i) = size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);

                if (looks_like_timeout(tokens.at(i))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::types::timeout);
                    ++i;
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid timeout token in 'join'");
                }

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_send(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_receive(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = 0;
                tie(calculated_size, i) = size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);

                if (looks_like_timeout(tokens.at(i))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::types::timeout);
                    ++i;
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid timeout token in 'receive'");
                }

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_watchdog(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_jump(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::bytecode_size);
                ++i;
                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_if(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;
                // for source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                calculated_size += 2 * sizeof(viua::internals::types::bytecode_size);
                i += 2;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_throw(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_catch(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                calculated_size += tokens.at(i++).str().size() + 1 - 2; // +1 for null terminator, -2 for quotes
                calculated_size += tokens.at(i++).str().size() + 1; // +1 for null terminator

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_pull(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_try(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);
                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_enter(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                calculated_size += tokens.at(i++).str().size() + 1; // +1 for null terminator

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_leave(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);
                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_import(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                calculated_size += tokens.at(i++).str().size() + 1 - 2; // +1 for null terminator, -2 for quotes

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_link(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                calculated_size += tokens.at(i++).str().size() + 1; // +1 for null terminator

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_class(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i++).str().size() + 1; // +1 for null terminator

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_prototype(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i++).str().size() + 1; // +1 for null terminator

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_derive(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for class name, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_attach(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for real name of aliased function, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                // for name of the alias, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_register(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }

            static auto size_of_struct(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }

            static auto size_of_new(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for class name, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_msg(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                    calculated_size += size_increment;
                } else {
                    calculated_size += tokens.at(i).str().size() + 1;
                    ++i;
                }

                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_insert(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_remove(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_return(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);    // start with the size of a single opcode
                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }
            static auto size_of_halt(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> tuple<viua::internals::types::bytecode_size, decltype(i)> {
                viua::internals::types::bytecode_size calculated_size = sizeof(viua::internals::types::byte);
                return tuple<viua::internals::types::bytecode_size, decltype(i)>(calculated_size, i);
            }

            viua::internals::types::bytecode_size calculate_bytecode_size_of_first_n_instructions2(const vector<viua::cg::lex::Token>& tokens, const std::remove_reference<decltype(tokens)>::type::size_type instructions_counter) {
                viua::internals::types::bytecode_size bytes = 0;

                const auto limit = tokens.size();
                std::remove_const<decltype(instructions_counter)>::type counted_instructions = 0;
                for (decltype(tokens.size()) i = 0; (i < limit) and (counted_instructions < instructions_counter); ++i) {
                    auto token = tokens.at(i);

                    if (token == ".function:" or token == ".closure:" or token == ".block:" or token == ".mark:" or token == ".link:" or token == ".signature:" or token == ".bsignature:" or token == ".unused:") {
                        ++i;
                        continue;
                    }
                    if (token == ".info:") {
                        i += 2;
                        continue;
                    }
                    if (token == ".name:") {
                        i += 2;
                        continue;
                    }
                    if (token == ".end") {
                        continue;
                    }
                    if (token == "\n") {
                        continue;
                    }

                    viua::internals::types::bytecode_size increase = 0;
                    if (tokens.at(i) == "nop") {
                        ++i;
                        tie(increase, i) = size_of_nop(tokens, i);
                    } else if (tokens.at(i) == "izero") {
                        ++i;
                        tie(increase, i) = size_of_izero(tokens, i);
                    } else if (tokens.at(i) == "istore") {
                        ++i;
                        tie(increase, i) = size_of_istore(tokens, i);
                    } else if (tokens.at(i) == "iinc") {
                        ++i;
                        tie(increase, i) = size_of_iinc(tokens, i);
                    } else if (tokens.at(i) == "idec") {
                        ++i;
                        tie(increase, i) = size_of_idec(tokens, i);
                    } else if (tokens.at(i) == "fstore") {
                        ++i;
                        tie(increase, i) = size_of_fstore(tokens, i);
                    } else if (tokens.at(i) == "itof") {
                        ++i;
                        tie(increase, i) = size_of_itof(tokens, i);
                    } else if (tokens.at(i) == "ftoi") {
                        ++i;
                        tie(increase, i) = size_of_ftoi(tokens, i);
                    } else if (tokens.at(i) == "stoi") {
                        ++i;
                        tie(increase, i) = size_of_stoi(tokens, i);
                    } else if (tokens.at(i) == "stof") {
                        ++i;
                        tie(increase, i) = size_of_stof(tokens, i);
                    } else if (tokens.at(i) == "add") {
                        ++i;
                        tie(increase, i) = size_of_add(tokens, i);
                    } else if (tokens.at(i) == "sub") {
                        ++i;
                        tie(increase, i) = size_of_sub(tokens, i);
                    } else if (tokens.at(i) == "mul") {
                        ++i;
                        tie(increase, i) = size_of_mul(tokens, i);
                    } else if (tokens.at(i) == "div") {
                        ++i;
                        tie(increase, i) = size_of_div(tokens, i);
                    } else if (tokens.at(i) == "lt") {
                        ++i;
                        tie(increase, i) = size_of_lt(tokens, i);
                    } else if (tokens.at(i) == "lte") {
                        ++i;
                        tie(increase, i) = size_of_lte(tokens, i);
                    } else if (tokens.at(i) == "gt") {
                        ++i;
                        tie(increase, i) = size_of_gt(tokens, i);
                    } else if (tokens.at(i) == "gte") {
                        ++i;
                        tie(increase, i) = size_of_gte(tokens, i);
                    } else if (tokens.at(i) == "eq") {
                        ++i;
                        tie(increase, i) = size_of_eq(tokens, i);
                    } else if (tokens.at(i) == "strstore") {
                        ++i;
                        tie(increase, i) = size_of_strstore(tokens, i);
                    } else if (tokens.at(i) == "text") {
                        ++i;
                        tie(increase, i) = size_of_text(tokens, i);
                    } else if (tokens.at(i) == "texteq") {
                        ++i;
                        tie(increase, i) = size_of_texteq(tokens, i);
                    } else if (tokens.at(i) == "textat") {
                        ++i;
                        tie(increase, i) = size_of_textat(tokens, i);
                    } else if (tokens.at(i) == "textsub") {
                        ++i;
                        tie(increase, i) = size_of_textsub(tokens, i);
                    } else if (tokens.at(i) == "textlength") {
                        ++i;
                        tie(increase, i) = size_of_textlength(tokens, i);
                    } else if (tokens.at(i) == "textcommonprefix") {
                        ++i;
                        tie(increase, i) = size_of_textcommonprefix(tokens, i);
                    } else if (tokens.at(i) == "textcommonsuffix") {
                        ++i;
                        tie(increase, i) = size_of_textcommonsuffix(tokens, i);
                    } else if (tokens.at(i) == "textconcat") {
                        ++i;
                        tie(increase, i) = size_of_textconcat(tokens, i);
                    } else if (tokens.at(i) == "streq") {
                        ++i;
                        tie(increase, i) = size_of_streq(tokens, i);
                    } else if (tokens.at(i) == "vec") {
                        ++i;
                        tie(increase, i) = size_of_vec(tokens, i);
                    } else if (tokens.at(i) == "vinsert") {
                        ++i;
                        tie(increase, i) = size_of_vinsert(tokens, i);
                    } else if (tokens.at(i) == "vpush") {
                        ++i;
                        tie(increase, i) = size_of_vpush(tokens, i);
                    } else if (tokens.at(i) == "vpop") {
                        ++i;
                        tie(increase, i) = size_of_vpop(tokens, i);
                    } else if (tokens.at(i) == "vat") {
                        ++i;
                        tie(increase, i) = size_of_vat(tokens, i);
                    } else if (tokens.at(i) == "vlen") {
                        ++i;
                        tie(increase, i) = size_of_vlen(tokens, i);
                    } else if (tokens.at(i) == "bool") {
                        ++i;
                        tie(increase, i) = size_of_bool(tokens, i);
                    } else if (tokens.at(i) == "not") {
                        ++i;
                        tie(increase, i) = size_of_not(tokens, i);
                    } else if (tokens.at(i) == "and") {
                        ++i;
                        tie(increase, i) = size_of_and(tokens, i);
                    } else if (tokens.at(i) == "or") {
                        ++i;
                        tie(increase, i) = size_of_or(tokens, i);
                    } else if (tokens.at(i) == "move") {
                        ++i;
                        tie(increase, i) = size_of_move(tokens, i);
                    } else if (tokens.at(i) == "copy") {
                        ++i;
                        tie(increase, i) = size_of_copy(tokens, i);
                    } else if (tokens.at(i) == "ptr") {
                        ++i;
                        tie(increase, i) = size_of_ptr(tokens, i);
                    } else if (tokens.at(i) == "swap") {
                        ++i;
                        tie(increase, i) = size_of_swap(tokens, i);
                    } else if (tokens.at(i) == "delete") {
                        ++i;
                        tie(increase, i) = size_of_delete(tokens, i);
                    } else if (tokens.at(i) == "isnull") {
                        ++i;
                        tie(increase, i) = size_of_isnull(tokens, i);
                    } else if (tokens.at(i) == "ress") {
                        ++i;
                        tie(increase, i) = size_of_ress(tokens, i);
                    } else if (tokens.at(i) == "print") {
                        ++i;
                        tie(increase, i) = size_of_print(tokens, i);
                    } else if (tokens.at(i) == "echo") {
                        ++i;
                        tie(increase, i) = size_of_echo(tokens, i);
                    } else if (tokens.at(i) == "capture") {
                        ++i;
                        tie(increase, i) = size_of_capture(tokens, i);
                    } else if (tokens.at(i) == "capturecopy") {
                        ++i;
                        tie(increase, i) = size_of_capturecopy(tokens, i);
                    } else if (tokens.at(i) == "capturemove") {
                        ++i;
                        tie(increase, i) = size_of_capturemove(tokens, i);
                    } else if (tokens.at(i) == "closure") {
                        ++i;
                        tie(increase, i) = size_of_closure(tokens, i);
                    } else if (tokens.at(i) == "function") {
                        ++i;
                        tie(increase, i) = size_of_function(tokens, i);
                    } else if (tokens.at(i) == "frame") {
                        ++i;
                        tie(increase, i) = size_of_frame(tokens, i);
                    } else if (tokens.at(i) == "param") {
                        ++i;
                        tie(increase, i) = size_of_param(tokens, i);
                    } else if (tokens.at(i) == "pamv") {
                        ++i;
                        tie(increase, i) = size_of_pamv(tokens, i);
                    } else if (tokens.at(i) == "call") {
                        ++i;
                        tie(increase, i) = size_of_call(tokens, i);
                    } else if (tokens.at(i) == "tailcall") {
                        ++i;
                        tie(increase, i) = size_of_tailcall(tokens, i);
                    } else if (tokens.at(i) == "arg") {
                        ++i;
                        tie(increase, i) = size_of_arg(tokens, i);
                    } else if (tokens.at(i) == "argc") {
                        ++i;
                        tie(increase, i) = size_of_argc(tokens, i);
                    } else if (tokens.at(i) == "process") {
                        ++i;
                        tie(increase, i) = size_of_process(tokens, i);
                    } else if (tokens.at(i) == "self") {
                        ++i;
                        tie(increase, i) = size_of_self(tokens, i);
                    } else if (tokens.at(i) == "join") {
                        ++i;
                        tie(increase, i) = size_of_join(tokens, i);
                    } else if (tokens.at(i) == "send") {
                        ++i;
                        tie(increase, i) = size_of_send(tokens, i);
                    } else if (tokens.at(i) == "receive") {
                        ++i;
                        tie(increase, i) = size_of_receive(tokens, i);
                    } else if (tokens.at(i) == "watchdog") {
                        ++i;
                        tie(increase, i) = size_of_watchdog(tokens, i);
                    } else if (tokens.at(i) == "jump") {
                        ++i;
                        tie(increase, i) = size_of_jump(tokens, i);
                    } else if (tokens.at(i) == "if") {
                        ++i;
                        tie(increase, i) = size_of_if(tokens, i);
                    } else if (tokens.at(i) == "throw") {
                        ++i;
                        tie(increase, i) = size_of_throw(tokens, i);
                    } else if (tokens.at(i) == "catch") {
                        ++i;
                        tie(increase, i) = size_of_catch(tokens, i);
                    } else if (tokens.at(i) == "draw") {
                        ++i;
                        tie(increase, i) = size_of_pull(tokens, i);
                    } else if (tokens.at(i) == "try") {
                        ++i;
                        tie(increase, i) = size_of_try(tokens, i);
                    } else if (tokens.at(i) == "enter") {
                        ++i;
                        tie(increase, i) = size_of_enter(tokens, i);
                    } else if (tokens.at(i) == "leave") {
                        ++i;
                        tie(increase, i) = size_of_leave(tokens, i);
                    } else if (tokens.at(i) == "import") {
                        ++i;
                        tie(increase, i) = size_of_import(tokens, i);
                    } else if (tokens.at(i) == "link") {
                        ++i;
                        tie(increase, i) = size_of_link(tokens, i);
                    } else if (tokens.at(i) == "class") {
                        ++i;
                        tie(increase, i) = size_of_class(tokens, i);
                    } else if (tokens.at(i) == "prototype") {
                        ++i;
                        tie(increase, i) = size_of_prototype(tokens, i);
                    } else if (tokens.at(i) == "derive") {
                        ++i;
                        tie(increase, i) = size_of_derive(tokens, i);
                    } else if (tokens.at(i) == "attach") {
                        ++i;
                        tie(increase, i) = size_of_attach(tokens, i);
                    } else if (tokens.at(i) == "register") {
                        ++i;
                        tie(increase, i) = size_of_register(tokens, i);
                    } else if (tokens.at(i) == "struct") {
                        ++i;
                        tie(increase, i) = size_of_struct(tokens, i);
                    } else if (tokens.at(i) == "new") {
                        ++i;
                        tie(increase, i) = size_of_new(tokens, i);
                    } else if (tokens.at(i) == "msg") {
                        ++i;
                        tie(increase, i) = size_of_msg(tokens, i);
                    } else if (tokens.at(i) == "insert") {
                        ++i;
                        tie(increase, i) = size_of_insert(tokens, i);
                    } else if (tokens.at(i) == "remove") {
                        ++i;
                        tie(increase, i) = size_of_remove(tokens, i);
                    } else if (tokens.at(i) == "return") {
                        ++i;
                        tie(increase, i) = size_of_return(tokens, i);
                    } else if (tokens.at(i) == "halt") {
                        ++i;
                        tie(increase, i) = size_of_halt(tokens, i);
                    } else {
                        throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("failed to calculate size of: " + tokens.at(i).str()));
                    }

                    while (i < limit and tokens[i].str() != "\n") {
                        ++i;
                    }

                    ++counted_instructions;

                    bytes += increase;
                }

                return bytes;
            }
            viua::internals::types::bytecode_size calculate_bytecode_size2(const vector<viua::cg::lex::Token>& tokens) {
                return calculate_bytecode_size_of_first_n_instructions2(tokens, tokens.size());
            }
        }
    }
}
