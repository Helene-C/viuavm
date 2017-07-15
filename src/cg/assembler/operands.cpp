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

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/program.h>
#include <viua/support/string.h>
using namespace std;


static string resolveregister(viua::cg::lex::Token token, const bool allow_bare_integers = false) {
    /*  This function is used to register numbers when a register is accessed, e.g.
     *  in `istore` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to assembler::operands::getint()
     * function which *expects* string.
     */
    ostringstream out;
    string reg = token.str();
    if (reg[0] == '@' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and
         * simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw("register indexes cannot be negative: " + reg);
        }

        // FIXME: analyse source and detect if the referenced register really holds an integer (the only value
        // suitable to use
        // as register reference)
        out.str(reg);
    } else if (reg[0] == '*' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and
         * simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw("register indexes cannot be negative: " + reg);
        }

        out.str(reg);
    } else if (reg[0] == '%' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and
         * simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw("register indexes cannot be negative: " + reg);
        }

        out.str(reg);
    } else if (reg == "void") {
        out << reg;
    } else if (allow_bare_integers and str::isnum(reg)) {
        out << reg;
    } else {
        throw viua::cg::lex::InvalidSyntax(token, ("illegal operand: " + token.str()));
    }
    return out.str();
}


int_op assembler::operands::getint(const string& s, const bool allow_bare_integers) {
    if (s.size() == 0) {
        throw "empty string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(IntegerOperandType::VOID);
    } else if (s.at(0) == '@') {
        return int_op(IntegerOperandType::REGISTER_REFERENCE, stoi(s.substr(1)));
    } else if (s.at(0) == '*') {
        return int_op(IntegerOperandType::POINTER_DEREFERENCE, stoi(s.substr(1)));
    } else if (s.at(0) == '%') {
        return int_op(stoi(s.substr(1)));
    } else if (allow_bare_integers and str::isnum(s)) {
        return int_op(stoi(s));
    } else {
        throw("cannot convert to int operand: " + s);
    }
}

int_op assembler::operands::getint_with_rs_type(const string& s, const viua::internals::RegisterSets rs_type,
                                                const bool allow_bare_integers) {
    if (s.size() == 0) {
        throw "empty string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(IntegerOperandType::VOID);
    } else if (s.at(0) == '@') {
        return int_op(IntegerOperandType::REGISTER_REFERENCE, rs_type, stoi(s.substr(1)));
    } else if (s.at(0) == '*') {
        return int_op(IntegerOperandType::POINTER_DEREFERENCE, rs_type, stoi(s.substr(1)));
    } else if (s.at(0) == '%') {
        return int_op(IntegerOperandType::INDEX, rs_type, stoi(s.substr(1)));
    } else if (allow_bare_integers and str::isnum(s)) {
        return int_op(stoi(s));
    } else {
        throw("cannot convert to int operand: " + s);
    }
}

int_op assembler::operands::getint(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) {
    string s = resolveregister(tokens.at(i));

    if (s.size() == 0) {
        throw "empty string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(IntegerOperandType::VOID);
    }

    int_op iop;
    if (s.at(0) == '@') {
        iop = int_op(IntegerOperandType::REGISTER_REFERENCE, stoi(s.substr(1)));
    } else if (s.at(0) == '*') {
        iop = int_op(IntegerOperandType::POINTER_DEREFERENCE, stoi(s.substr(1)));
    } else if (s.at(0) == '%') {
        iop = int_op(stoi(s.substr(1)));
    } else {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "cannot convert to register index");
    }

    // FIXME set iop.rs_type according to rs specifier for given operand

    return iop;
}

byte_op assembler::operands::getbyte(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, char>(ref, static_cast<char>(stoi(ref ? str::sub(s, 1) : s)));
}

float_op assembler::operands::getfloat(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, float>(ref, stof(ref ? str::sub(s, 1) : s));
}

tuple<string, string> assembler::operands::get2(string s) {
    /** Returns tuple of two strings - two operands chunked from the `s` string.
     */
    string op_a, op_b;
    op_a = str::chunk(s);
    s = str::sub(s, op_a.size());
    op_b = str::chunk(s);
    return tuple<string, string>(op_a, op_b);
}

tuple<string, string, string> assembler::operands::get3(string s, bool fill_third) {
    string op_a, op_b, op_c;

    op_a = str::chunk(s);
    s = str::lstrip(str::sub(s, op_a.size()));

    op_b = str::chunk(s);
    s = str::lstrip(str::sub(s, op_b.size()));

    /* If s is empty and fill_third is true, use first operand as a filler.
     * In any other case, use the chunk of s.
     * The chunk of empty string will give us empty string and
     * it is a valid (and sometimes wanted) value to return.
     */
    op_c = (s.size() == 0 and fill_third ? op_a : str::chunk(s));

    return tuple<string, string, string>(op_a, op_b, op_c);
}

auto assembler::operands::normalise_binary_literal(const string s) -> string {
    ostringstream oss;

    auto n = 0;
    while ((s.size() + n) % 8 != 0) {
        oss << '0';
        ++n;
    }
    oss << s;

    return oss.str();
}
auto assembler::operands::octal_to_binary_literal(const string s) -> string {
    ostringstream oss;
    const static map<const char, const string> lookup = {
        {
            '0', "000",
        },
        {
            '1', "001",
        },
        {
            '2', "010",
        },
        {
            '3', "011",
        },
        {
            '4', "100",
        },
        {
            '5', "101",
        },
        {
            '6', "110",
        },
        {
            '7', "111",
        },
    };
    for (const auto c : s.substr(2)) {
        oss << lookup.at(c);
    }
    return oss.str();
}
auto assembler::operands::hexadecimal_to_binary_literal(const string s) -> string {
    ostringstream oss;
    const static map<const char, const string> lookup = {
        {
            '0', "0000",
        },
        {
            '1', "0001",
        },
        {
            '2', "0010",
        },
        {
            '3', "0011",
        },
        {
            '4', "0100",
        },
        {
            '5', "0101",
        },
        {
            '6', "0110",
        },
        {
            '7', "0111",
        },
        {
            '8', "1000",
        },
        {
            '9', "1001",
        },
        {
            'a', "1010",
        },
        {
            'b', "1011",
        },
        {
            'c', "1100",
        },
        {
            'd', "1101",
        },
        {
            'e', "1110",
        },
        {
            'f', "1111",
        },
    };
    for (const auto c : s.substr(2)) {
        oss << lookup.at(c);
    }
    return oss.str();
}
auto assembler::operands::convert_token_to_bitstring_operand(const viua::cg::lex::Token token)
    -> vector<uint8_t> {
    auto s = token.str();
    string workable_version;
    if (s.at(1) == 'b') {
        workable_version = normalise_binary_literal(s);
    } else if (s.at(1) == 'o') {
        workable_version = normalise_binary_literal(octal_to_binary_literal(s));
    } else if (s.at(1) == 'x') {
        workable_version = normalise_binary_literal(hexadecimal_to_binary_literal(s));
    } else {
        throw viua::cg::lex::InvalidSyntax(token);
    }

    reverse(workable_version.begin(), workable_version.end());

    vector<uint8_t> converted;
    uint8_t part = 0;
    for (decltype(workable_version)::size_type i = 0; i < workable_version.size(); ++i) {
        uint8_t one = 1;
        if (workable_version.at(i) == '1') {
            one = static_cast<uint8_t>(one << (i % 8));
            part = (part | one);
        }
        if ((i + 1) % 8 == 0) {
            converted.push_back(part);
            part = 0;
        }
    }

    return converted;
}
