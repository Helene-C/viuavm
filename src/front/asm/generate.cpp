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

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <viua/machine.h>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/program.h>
#include <viua/cg/tools.h>
#include <viua/cg/tokenizer.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
using namespace std;


extern bool VERBOSE;
extern bool DEBUG;
extern bool SCREAM;


using Token = viua::cg::lex::Token;


template<class T> void bwrite(ofstream& out, const T& object) {
    out.write(reinterpret_cast<const char*>(&object), sizeof(T));
}
static void strwrite(ofstream& out, const string& s) {
    out.write(s.c_str(), static_cast<std::streamsize>(s.size()));
    out.put('\0');
}


static tuple<uint64_t, enum JUMPTYPE> resolvejump(Token token, const map<string, int>& marks, uint64_t instruction_index) {
    /*  This function is used to resolve jumps in `jump` and `branch` instructions.
     */
    string jmp = token.str();
    uint64_t addr = 0;
    enum JUMPTYPE jump_type = JMP_RELATIVE;
    if (str::isnum(jmp, false)) {
        addr = stoul(jmp);
    } else if (jmp[0] == '.' and str::isnum(str::sub(jmp, 1LU))) {
        addr = stoul(str::sub(jmp, 1));
        jump_type = JMP_ABSOLUTE;
    } else if (jmp.substr(0, 2) == "0x") {
        stringstream ss;
        ss << hex << jmp;
        ss >> addr;
        jump_type = JMP_TO_BYTE;
    } else if (jmp[0] == '-') {
        int jump_value = stoi(jmp);
        if (instruction_index < static_cast<decltype(addr)>(-1 * jump_value)) {
            // FIXME: generate line numbers in error message
            // FIXME: move jump verification to assembler::verify namespace function
            ostringstream oss;
            oss << "use of relative jump results in a jump to negative index: ";
            oss << "jump_value = " << jump_value << ", ";
            oss << "instruction_index = " << instruction_index;
            throw viua::cg::lex::InvalidSyntax(token, oss.str());
        }
        addr = (instruction_index - static_cast<uint64_t>(-1 * jump_value));
    } else if (jmp[0] == '+') {
        addr = (instruction_index + stoul(jmp.substr(1)));
    } else if (jmp[0] == '.') {
        // FIXME
        cout << "FIXME: global marker jumps (jumps to functions) are not implemented yet" << endl;
        exit(1);
    } else {
        try {
            // FIXME: markers map should use uint64_t to avoid the need for casting
            addr = static_cast<uint64_t>(marks.at(jmp));
        } catch (const std::out_of_range& e) {
            throw viua::cg::lex::InvalidSyntax(token, ("jump to unrecognised marker: " + str::enquote(str::strencode(jmp))));
        }
    }

    // FIXME: check if the jump is within the size of bytecode
    return tuple<uint64_t, enum JUMPTYPE>(addr, jump_type);
}

static string resolveregister(Token token, const map<string, int>& names) {
    /*  This function is used to register numbers when a register is accessed, e.g.
     *  in `istore` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to assembler::operands::getint() function which *expects* string.
     */
    ostringstream out;
    string reg = token.str();
    if (str::isnum(reg)) {
        /*  Basic case - the register is accessed as real index, everything is nice and simple.
         */
        out.str(reg);
    } else if (reg[0] == '@' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw ("register indexes cannot be negative: " + reg);
        }

        // FIXME: analyse source and detect if the referenced register really holds an integer (the only value suitable to use
        // as register reference)
        out.str(reg);
    } else {
        /*  Case is no longer basic - it seems that a register is being accessed by name.
         *  Names must be checked to see if the one used was declared.
         */
        if (reg[0] == '@') {
            out << '@';
            reg = str::sub(reg, 1);
        }
        try {
            out << names.at(reg);
        } catch (const std::out_of_range& e) {
            // first, check if the name is non-empty
            if (reg != "") {
                // Jinkies! This name was not declared.
                throw viua::cg::lex::InvalidSyntax(token, ("undeclared register name: " + str::strencode(reg)));
            } else {
                throw viua::cg::lex::InvalidSyntax(token, "not enough operands");
            }
        }
    }
    return out.str();
}


/*  This is a mapping of instructions to their assembly functions.
 *  Used in the assembly() function.
 *
 *  It is suitable for all instructions which use three, simple register-index operands.
 *
 *  BE WARNED!
 *  This mapping (and the assemble_three_intop_instruction() function) *greatly* reduce the amount of code repetition
 *  in the assembler but is kinda black voodoo magic...
 *
 *  NOTE TO FUTURE SELF:
 *  If you feel comfortable with taking pointers of member functions and calling such things - go on.
 *  Otherwise, it may be better to leave this alone until your have refreshed your memory.
 *  Here is isocpp.org's FAQ about pointers to members (2015-01-17): https://isocpp.org/wiki/faq/pointers-to-members
 */
typedef Program& (Program::*ThreeIntopAssemblerFunction)(int_op, int_op, int_op);
const map<string, ThreeIntopAssemblerFunction> THREE_INTOP_ASM_FUNCTIONS = {
    { "iadd", &Program::opiadd },
    { "isub", &Program::opisub },
    { "imul", &Program::opimul },
    { "idiv", &Program::opidiv },
    { "ilt",  &Program::opilt },
    { "ilte", &Program::opilte },
    { "igt",  &Program::opigt },
    { "igte", &Program::opigte },
    { "ieq",  &Program::opieq },

    { "fadd", &Program::opfadd },
    { "fsub", &Program::opfsub },
    { "fmul", &Program::opfmul },
    { "fdiv", &Program::opfdiv },
    { "flt",  &Program::opflt },
    { "flte", &Program::opflte },
    { "fgt",  &Program::opfgt },
    { "fgte", &Program::opfgte },
    { "feq",  &Program::opfeq },

    { "and",  &Program::opand },
    { "or",   &Program::opor },

    { "enclose", &Program::openclose },
    { "enclosecopy", &Program::openclosecopy },
    { "enclosemove", &Program::openclosemove },

    { "insert", &Program::opinsert },
    { "remove", &Program::opremove },
};


static uint64_t assemble_instruction(Program& program, uint64_t& instruction, uint64_t i, const vector<Token>& tokens, map<string, int>& marks, map<string, int>& names) {
    /*  This is main assembly loop.
     *  It iterates over lines with instructions and
     *  uses bytecode generation API to fill the program with instructions and
     *  from them generate the bytecode.
     */
    if (DEBUG and SCREAM) {
        cout << send_control_seq(COLOR_FG_LIGHT_CYAN) << "message" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "assembling '";
        cout << send_control_seq(COLOR_FG_WHITE) << tokens.at(i).str() << send_control_seq(ATTR_RESET);
        cout << "' instruction\n";
    }

    if (tokens.at(i) == "nop") {
        program.opnop();
    } else if (tokens.at(i) == "izero") {
        program.opizero(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "istore") {
        program.opistore(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "iadd") {
        program.opiadd(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "isub") {
        program.opisub(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "imul") {
        program.opimul(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "idiv") {
        program.opidiv(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "ilt") {
        program.opilt(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "ilte") {
        program.opilte(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "igte") {
        program.opigte(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "igt") {
        program.opigt(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "ieq") {
        program.opieq(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "iinc") {
        program.opiinc(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "idec") {
        program.opidec(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "fstore") {
        program.opfstore(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), static_cast<float>(stod(tokens.at(i+2).str())));
    } else if (tokens.at(i) == "fadd") {
        program.opfadd(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "fsub") {
        program.opfsub(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "fmul") {
        program.opfmul(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "fdiv") {
        program.opfdiv(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "flt") {
        program.opflt(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "flte") {
        program.opflte(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "fgt") {
        program.opfgt(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "fgte") {
        program.opfgte(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "feq") {
        program.opfeq(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "bstore") {
        program.opbstore(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getbyte(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "itof") {
        program.opitof(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "ftoi") {
        program.opftoi(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "stoi") {
        program.opstoi(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "stof") {
        program.opstof(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "strstore") {
        program.opstrstore(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "vec") {
        program.opvec(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "vinsert") {
        Token vec = tokens.at(i+1), src = tokens.at(i+2), pos = tokens.at(i+3);
        if (pos == "\n") { pos = Token(src.line(), src.character(), "0"); }
        program.opvinsert(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(src, names)), assembler::operands::getint(resolveregister(pos, names)));
    } else if (tokens.at(i) == "vpush") {
        program.opvpush(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "vpop") {
        Token vec = tokens.at(i+1), dst = tokens.at(i+2), pos = tokens.at(i+3);
        program.opvpop(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(dst, names)), assembler::operands::getint(resolveregister(pos, names)));
    } else if (tokens.at(i) == "vat") {
        Token vec = tokens.at(i+1), dst = tokens.at(i+2), pos = tokens.at(i+3);
        if (pos == "\n") { pos = Token(dst.line(), dst.character(), "-1"); }
        program.opvat(assembler::operands::getint(resolveregister(vec, names)), assembler::operands::getint(resolveregister(dst, names)), assembler::operands::getint(resolveregister(pos, names)));
    } else if (tokens.at(i) == "vlen") {
        program.opvlen(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "not") {
        program.opnot(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "and") {
        program.opand(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "or") {
        program.opor(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "move") {
        program.opmove(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "copy") {
        program.opcopy(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "ptr") {
        program.opptr(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "swap") {
        program.opswap(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "delete") {
        program.opdelete(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "isnull") {
        program.opisnull(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "ress") {
        program.opress(tokens.at(i+1));
    } else if (tokens.at(i) == "tmpri") {
        program.optmpri(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "tmpro") {
        program.optmpro(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "print") {
        program.opprint(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "echo") {
        program.opecho(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "enclose") {
        program.openclose(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "enclosecopy") {
        program.openclosecopy(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "enclosemove") {
        program.openclosemove(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "closure") {
        program.opclosure(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "function") {
        program.opfunction(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "fcall") {
        program.opfcall(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "frame") {
        program.opframe(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "param") {
        program.opparam(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "pamv") {
        program.oppamv(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "arg") {
        program.oparg(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "argc") {
        program.opargc(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "call") {
        /** Full form of call instruction has two operands: function name and return value register index.
         *  If call is given only one operand - it means it is the instruction index and returned value is discarded.
         *  To explicitly state that return value should be discarderd 0 can be supplied as second operand.
         */
        /** Why is the function supplied as a *string* and not direct instruction pointer?
         *  That would be faster - c'mon couldn't assembler just calculate offsets and insert them?
         *
         *  Nope.
         *
         *  Yes, it *would* be faster if calls were just precalculated jumps.
         *  However, by them being strings we get plenty of flexibility, good-quality stack traces, and
         *  a place to put plenty of debugging info.
         *  All that at a cost of just one map lookup; the overhead is minimal and gains are big.
         *  What's not to love?
         *
         *  Of course, you, my dear reader, are free to take this code (it's GPL after all!) and
         *  modify it to suit your particular needs - in that case that would be calculating call jumps
         *  at compile time and exchanging CALL instructions with JUMP instructions.
         *
         *  Good luck with debugging your code, then.
         */
        Token fn_name = tokens.at(i+2), reg = tokens.at(i+1);

        // if second operand is a newline, fill it with zero
        // which means that return value will be discarded
        if (fn_name == "\n") {
            fn_name = reg;
            reg = Token(fn_name.line(), fn_name.character(), "0");
        }

        program.opcall(assembler::operands::getint(resolveregister(reg, names)), fn_name.str());
    } else if (tokens.at(i) == "tailcall") {
        program.optailcall(tokens.at(i+1));
    } else if (tokens.at(i) == "process") {
        program.opprocess(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "self") {
        program.opself(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "join") {
        Token a_chnk = tokens.at(i+1), b_chnk = tokens.at(i+2), timeout_chnk = tokens.at(i+3);
        int_op timeout{false, 0};
        if (timeout_chnk != "infinity") {
            // remove the 'ms' part from timeout
            timeout = assembler::operands::getint(timeout_chnk.str().substr(0, timeout_chnk.str().size()-2));
            ++get<1>(timeout);
        }
        program.opjoin(assembler::operands::getint(resolveregister(a_chnk, names)), assembler::operands::getint(resolveregister(b_chnk, names)), timeout);
    } else if (tokens.at(i) == "send") {
        program.opsend(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)));
    } else if (tokens.at(i) == "receive") {
        Token regno_chnk = tokens.at(i+1), timeout_chnk = tokens.at(i+2);
        int_op to{false, 0};
        if (timeout_chnk != "infinity") {
            // remove the 'ms' part from timeout
            to = assembler::operands::getint(timeout_chnk.str().substr(0, timeout_chnk.str().size()-2));
            ++get<1>(to);
        }
        program.opreceive(assembler::operands::getint(resolveregister(regno_chnk, names)), to);
    } else if (tokens.at(i) == "watchdog") {
        program.opwatchdog(tokens.at(i+1));
    } else if (tokens.at(i) == "branch") {
        /*  If branch is given three operands, it means its full, three-operands form is being used.
         *  Otherwise, it is short, two-operands form instruction and assembler should fill third operand accordingly.
         *
         *  In case of short-form `branch` instruction:
         *
         *      * first operand is index of the register to check,
         *      * second operand is the address to which to jump if register is true,
         *      * third operand is assumed to be the *next instruction*, i.e. instruction after the branch instruction,
         *
         *  In full (with three operands) form of `branch` instruction:
         *
         *      * third operands is the address to which to jump if register is false,
         */
        Token condition = tokens.at(i+1), if_true = tokens.at(i+2), if_false = tokens.at(i+3);

        uint64_t addrt_target, addrf_target;
        enum JUMPTYPE addrt_jump_type, addrf_jump_type;
        tie(addrt_target, addrt_jump_type) = resolvejump(tokens.at(i+2), marks, instruction);
        if (if_false != "\n") {
            tie(addrf_target, addrf_jump_type) = resolvejump(tokens.at(i+3), marks, instruction);
        } else {
            addrf_jump_type = JMP_RELATIVE;
            addrf_target = instruction+1;
        }

        program.opbranch(assembler::operands::getint(resolveregister(condition, names)), addrt_target, addrt_jump_type, addrf_target, addrf_jump_type);
    } else if (tokens.at(i) == "jump") {
        /*  Jump instruction can be written in two forms:
         *
         *      * `jump <index>`
         *      * `jump :<marker>`
         *
         *  Assembler must distinguish between these two forms, and so it does.
         *  Here, we use a function from string support lib to determine
         *  if the jump is numeric, and thus an index, or
         *  a string - in which case we consider it a marker jump.
         *
         *  If it is a marker jump, assembler will look the marker up in a map and
         *  if it is not found throw an exception about unrecognised marker being used.
         */
        uint64_t jump_target;
        enum JUMPTYPE jump_type;
        tie(jump_target, jump_type) = resolvejump(tokens.at(i+1), marks, instruction);

        program.opjump(jump_target, jump_type);
    } else if (tokens.at(i) == "try") {
        program.optry();
    } else if (tokens.at(i) == "catch") {
        string type_chnk, catcher_chnk;
        program.opcatch(tokens.at(i+1), tokens.at(i+2));
    } else if (tokens.at(i) == "pull") {
        program.oppull(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "enter") {
        program.openter(tokens.at(i+1));
    } else if (tokens.at(i) == "throw") {
        program.opthrow(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "leave") {
        program.opleave();
    } else if (tokens.at(i) == "import") {
        program.opimport(tokens.at(i+1));
    } else if (tokens.at(i) == "link") {
        program.oplink(tokens.at(i+1));
    } else if (tokens.at(i) == "class") {
        program.opclass(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "derive") {
        program.opderive(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "attach") {
        program.opattach(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2), tokens.at(i+3));
    } else if (tokens.at(i) == "register") {
        program.opregister(assembler::operands::getint(resolveregister(tokens.at(i+1), names)));
    } else if (tokens.at(i) == "new") {
        program.opnew(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "msg") {
        program.opmsg(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), tokens.at(i+2));
    } else if (tokens.at(i) == "insert") {
        program.opinsert(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "remove") {
        program.opremove(assembler::operands::getint(resolveregister(tokens.at(i+1), names)), assembler::operands::getint(resolveregister(tokens.at(i+2), names)), assembler::operands::getint(resolveregister(tokens.at(i+3), names)));
    } else if (tokens.at(i) == "return") {
        program.opreturn();
    } else if (tokens.at(i) == "halt") {
        program.ophalt();
    } else if (tokens.at(i).str().substr(0, 1) == ".") {
        // do nothing, it's an assembler directive
    } else {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("unimplemented instruction: " + str::enquote(str::strencode(tokens.at(i)))));
    }

    if (tokens.at(i).str().substr(0, 1) != ".") {
        ++instruction;
        /* cout << "increased instruction count to " << instruction << ": " << tokens.at(i).str() << endl; */
    } else {
        /* cout << "not increasing instruction count: " << tokens.at(i).str() << endl; */
    }

    while (tokens.at(++i) != "\n");
    ++i;  // skip the newline
    return i;
}
static Program& compile(Program& program, const vector<Token>& tokens, map<string, int>& marks, map<string, int>& names) {
    /** Compile instructions into bytecode using bytecode generation API.
     *
     */
    uint64_t instruction = 0;
    for (decltype(tokens.size()) i = 0; i < tokens.size();) {
        i = assemble_instruction(program, instruction, i, tokens, marks, names);
    }

    return program;
}


static void assemble(Program& program, const vector<Token>& tokens) {
    /** Assemble instructions in lines into a program.
     *  This function first garthers required information about markers, named registers and functions.
     *  Then, it passes all gathered data into compilation function.
     *
     *  :params:
     *
     *  program         - Program object which will be used for assembling
     *  lines           - lines with instructions
     */
    map<string, int> marks = assembler::ce::getmarks(tokens);
    map<string, int> names = assembler::ce::getnames(tokens);
    compile(program, tokens, marks, names);
}


static map<string, uint64_t> mapInvocableAddresses(uint64_t& starting_instruction, const invocables_t& blocks) {
    map<string, uint64_t> addresses;
    for (string name : blocks.names) {
        addresses[name] = starting_instruction;
        try {
            starting_instruction += viua::cg::tools::calculate_bytecode_size(blocks.tokens.at(name));
        } catch (const std::out_of_range& e) {
            throw ("could not find block '" + name + "'");
        }
    }
    return addresses;
}

vector<string> expandSource(const vector<string>& lines, map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
    vector<string> stripped_lines;

    for (unsigned i = 0; i < lines.size(); ++i) {
        stripped_lines.emplace_back(str::lstrip(lines[i]));
    }

    vector<string> asm_lines;
    for (unsigned i = 0; i < stripped_lines.size(); ++i) {
        if (stripped_lines[i] == "") {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.emplace_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".signature")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.emplace_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".bsignature")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.emplace_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".function")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.emplace_back(lines[i]);
        } else if (str::startswith(stripped_lines[i], ".end")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.emplace_back(lines[i]);
        } else if (stripped_lines[i][0] == ';' or str::startswith(stripped_lines[i], "--")) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.emplace_back(lines[i]);
        } else if (not str::contains(stripped_lines[i], '(')) {
            expanded_lines_to_source_lines[asm_lines.size()] = i;
            asm_lines.emplace_back(lines[i]);
        } else {
            vector<vector<string>> decoded_lines = decode_line(stripped_lines[i]);
            auto indent = (lines[i].size() - stripped_lines[i].size());
            for (decltype(decoded_lines)::size_type j = 0; j < decoded_lines.size(); ++j) {
                expanded_lines_to_source_lines[asm_lines.size()] = i;
                asm_lines.emplace_back(str::strmul<char>(' ', indent) + str::join<char>(decoded_lines[j], ' '));
            }
        }
    }

    return asm_lines;
}

static uint64_t writeCodeBlocksSection(ofstream& out, const invocables_t& blocks, const vector<string>& linked_block_names, uint64_t block_bodies_size_so_far = 0) {
    uint64_t block_ids_section_size = 0;
    for (string name : blocks.names) { block_ids_section_size += name.size(); }
    // we need to insert address after every block
    block_ids_section_size += sizeof(uint64_t) * blocks.names.size();
    // for null characters after block names
    block_ids_section_size += blocks.names.size();

    /////////////////////////////////////////////
    // WRITE OUT BLOCK IDS SECTION
    // THIS ALSO INCLUDES IDS OF LINKED BLOCKS
    bwrite(out, block_ids_section_size);
    for (string name : blocks.names) {
        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_LIGHT_CYAN) << "message" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "writing block '";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name << send_control_seq(ATTR_RESET);
            cout << "' to block address table";
        }
        if (find(linked_block_names.begin(), linked_block_names.end(), name) != linked_block_names.end()) {
            if (DEBUG) {
                cout << ": delayed" << endl;
            }
            continue;
        }
        if (DEBUG) {
            cout << endl;
        }

        strwrite(out, name);
        // mapped address must come after name
        // FIXME: use uncasted uint64_t
        bwrite(out, block_bodies_size_so_far);
        // block_bodies_size_so_far size must be incremented by the actual size of block's bytecode size
        // to give correct offset for next block
        try {
            block_bodies_size_so_far += viua::cg::tools::calculate_bytecode_size(blocks.tokens.at(name));
        } catch (const std::out_of_range& e) {
            throw ("could not find block '" + name + "' during address table write");
        }
    }

    return block_bodies_size_so_far;
}

static string get_main_function(const vector<Token>& tokens, const vector<string>& available_functions) {
    string main_function = "";
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".main:") {
            main_function = tokens.at(i+1);
            break;
        }
    }
    if (main_function == "") {
        for (auto f : available_functions) {
            if (f == "main/0" or f == "main/1" or f == "main/2") {
                main_function = f;
                break;
            }
        }
    }
    return main_function;
}

static void check_main_function(const string& main_function, const vector<Token>& main_function_tokens) {
        // Why three newlines?
        //
        // Here's why:
        //
        // - first newline is after the final 'return' instruction
        // - second newline is after the last-but-one instruction which should set the return register
        // - third newline is the marker after which we look for the instruction that will set the return register
        //
        // Example:
        //
        //   1st newline
        //         |
        //         |  2nd newline
        //         |   |
        //      nop    |
        //      izero 0
        //      return
        //            |
        //          3rd newline
        //
        // If these three newlines are found then the main function is considered "full".
        // Anything less, and things get suspicious.
        // If there are two newlines - maybe the function just returns something.
        // If there is only one newline - the main function is invalid, because there is no way
        // to correctly set the return register, and return from the function with one instruction.
        //
        const int expected_newlines = 3;

        int found_newlines = 0;
        auto i = main_function_tokens.size()-1;
        while (i and found_newlines < expected_newlines) {
            if (main_function_tokens.at(i--) == "\n") {
                ++found_newlines;
            }
        }
        if (found_newlines >= expected_newlines) {
            // if found newlines number at least equals the expected number we
            // have to adjust token counter to skip past last required newline and the token before it
            i += 2;
        }
        auto last_instruction = main_function_tokens.at(i);
        if (not (last_instruction == "copy" or last_instruction == "move" or last_instruction == "swap" or last_instruction == "izero" or last_instruction == "istore")) {
            throw viua::cg::lex::InvalidSyntax(last_instruction, ("main function does not return a value: " + main_function));
        }
        if (main_function_tokens.at(i+1) != "0") {
            throw viua::cg::lex::InvalidSyntax(last_instruction, ("main function does not return a value: " + main_function));
        }
}

static uint64_t generate_entry_function(uint64_t bytes, map<string, uint64_t> function_addresses, invocables_t& functions, const string& main_function, uint64_t starting_instruction) {
    if (DEBUG) {
        cout << send_control_seq(COLOR_FG_LIGHT_CYAN) << "message" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "generating ";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << ENTRY_FUNCTION_NAME << send_control_seq(ATTR_RESET);
        cout << " function" << endl;
    }

    vector<string> entry_function_lines;
    vector<Token> entry_function_tokens;
    functions.names.emplace_back(ENTRY_FUNCTION_NAME);
    function_addresses[ENTRY_FUNCTION_NAME] = starting_instruction;

    // entry function sets global stuff (FIXME: not really)
    entry_function_lines.emplace_back("ress local");
    entry_function_tokens.emplace_back(0, 0, "ress");
    entry_function_tokens.emplace_back(0, 0, "local");
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += OP_SIZES.at("ress");

    // generate different instructions based on which main function variant
    // has been selected
    if (main_function == "main/0") {
        entry_function_lines.emplace_back("frame 0 16");
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "0");
        entry_function_tokens.emplace_back(0, 0, "16");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += OP_SIZES.at("frame");
    } else if (main_function == "main/2") {
        entry_function_lines.emplace_back("frame 2 16");
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "2");
        entry_function_tokens.emplace_back(0, 0, "16");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += OP_SIZES.at("frame");

        // pop first element on the list of aruments
        entry_function_lines.emplace_back("vpop 0 1 0");
        entry_function_tokens.emplace_back(0, 0, "vpop");
        entry_function_tokens.emplace_back(0, 0, "0");
        entry_function_tokens.emplace_back(0, 0, "1");
        entry_function_tokens.emplace_back(0, 0, "0");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += OP_SIZES.at("vpop");

        // for parameter for main/2 is the name of the program
        entry_function_lines.emplace_back("param 0 0");
        entry_function_tokens.emplace_back(0, 0, "param");
        entry_function_tokens.emplace_back(0, 0, "0");
        entry_function_tokens.emplace_back(0, 0, "0");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += OP_SIZES.at("param");

        // second parameter for main/2 is the vector with the rest
        // of the commandl ine parameters
        entry_function_lines.emplace_back("param 1 1");
        entry_function_tokens.emplace_back(0, 0, "param");
        entry_function_tokens.emplace_back(0, 0, "1");
        entry_function_tokens.emplace_back(0, 0, "1");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += OP_SIZES.at("param");
    } else {
        // this is for default main function, i.e. `main/1` or
        // for custom main functions
        // FIXME: should custom main function be allowed?
        entry_function_lines.emplace_back("frame 1 16");
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "1");
        entry_function_tokens.emplace_back(0, 0, "16");
        entry_function_tokens.emplace_back(0, 0, "\n");

        entry_function_lines.emplace_back("param 0 1");
        entry_function_tokens.emplace_back(0, 0, "param");
        entry_function_tokens.emplace_back(0, 0, "0");
        entry_function_tokens.emplace_back(0, 0, "1");
        entry_function_tokens.emplace_back(0, 0, "\n");

        bytes += OP_SIZES.at("frame");
        bytes += OP_SIZES.at("param");
    }

    // name of the main function must not be hardcoded because there is '.main:' assembler
    // directive which can set an arbitrary function as main
    // we also save return value in 1 register since 0 means "drop return value"
    entry_function_lines.emplace_back("call 1 " + main_function);
    entry_function_tokens.emplace_back(0, 0, "call");
    entry_function_tokens.emplace_back(0, 0, "1");
    entry_function_tokens.emplace_back(0, 0, main_function);
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += OP_SIZES.at("call");
    bytes += main_function.size()+1;

    // then, register 1 is moved to register 0 so it counts as a return code
    entry_function_lines.emplace_back("move 0 1");
    entry_function_tokens.emplace_back(0, 0, "move");
    entry_function_tokens.emplace_back(0, 0, "0");
    entry_function_tokens.emplace_back(0, 0, "1");
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += OP_SIZES.at("move");

    entry_function_lines.emplace_back("halt");
    entry_function_tokens.emplace_back(0, 0, "halt");
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += OP_SIZES.at("halt");

    functions.bodies[ENTRY_FUNCTION_NAME] = entry_function_lines;
    functions.tokens[ENTRY_FUNCTION_NAME] = entry_function_tokens;

    return bytes;
}

int generate(vector<Token>& tokens, invocables_t& functions, invocables_t& blocks, const string& filename, string& compilename, const vector<string>& commandline_given_links, const compilationflags_t& flags) {
    //////////////////////////////
    // SETUP INITIAL BYTECODE SIZE
    uint64_t bytes = 0;


    /////////////////////////
    // GET MAIN FUNCTION NAME
    string main_function = get_main_function(tokens, functions.names);
    if (((VERBOSE and main_function != "main/1" and main_function != "") or DEBUG) and not flags.as_lib) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "main function set to: ";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << main_function << send_control_seq(ATTR_RESET);
        cout << endl;
    }


    /////////////////////////////////////////
    // CHECK IF MAIN FUNCTION RETURNS A VALUE
    // FIXME: this is just a crude check - it does not acctually checks if these instructions set 0 register
    // this must be better implemented or we will receive "function did not set return register" exceptions at runtime
    bool main_is_defined = (find(functions.names.begin(), functions.names.end(), main_function) != functions.names.end());
    if (not flags.as_lib and main_is_defined) {
        check_main_function(main_function, functions.tokens.at(main_function));
    }
    if (not main_is_defined and (DEBUG or VERBOSE) and not flags.as_lib) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "main function (";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << main_function << send_control_seq(ATTR_RESET);
        cout << ") is not defined, deferring main function check to post-link phase" << endl;
    }


    /////////////////////////////////
    // MAP FUNCTIONS TO ADDRESSES AND
    // MAP BLOCKS TO ADDRESSES AND
    // SET STARTING INSTRUCTION
    uint64_t starting_instruction = 0;  // the bytecode offset to first executable instruction
    map<string, uint64_t> function_addresses;
    map<string, uint64_t> block_addresses;
    try {
        block_addresses = mapInvocableAddresses(starting_instruction, blocks);
        function_addresses = mapInvocableAddresses(starting_instruction, functions);
        bytes = viua::cg::tools::calculate_bytecode_size(tokens);
    } catch (const string& e) {
        throw ("bytecode size calculation failed: " + e);
    }


    /////////////////////////////////////////////////////////
    // GATHER LINKS, GET THEIR SIZES AND ADJUST BYTECODE SIZE
    vector<string> links = assembler::ce::getlinks(tokens);
    vector<tuple<string, uint64_t, byte*> > linked_libs_bytecode;
    vector<string> linked_function_names;
    vector<string> linked_block_names;
    map<string, vector<uint64_t> > linked_libs_jumptables;

    // map of symbol names to name of the module the symbol came from
    map<string, string> symbol_sources;
    for (auto f : functions.names) {
        symbol_sources[f] = filename;
    }

    for (string lnk : commandline_given_links) {
        if (find(links.begin(), links.end(), lnk) == links.end()) {
            links.emplace_back(lnk);
        } else {
            throw ("requested to link module '" + lnk + "' more than once");
        }
    }

    // gather all linked function names
    for (string lnk : links) {
        Loader loader(lnk);
        loader.load();

        vector<string> fn_names = loader.getFunctions();
        for (string fn : fn_names) {
            if (function_addresses.count(fn)) {
                throw ("duplicate symbol '" + fn + "' found when linking '" + lnk + "' (previously found in '" + symbol_sources.at(fn) + "')");
            }
        }

        map<string, uint64_t> fn_addresses = loader.getFunctionAddresses();
        for (string fn : fn_names) {
            function_addresses[fn] = 0; // for now we just build a list of all available functions
            symbol_sources[fn] = lnk;
            linked_function_names.emplace_back(fn);
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "prelinking function ";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << fn << send_control_seq(ATTR_RESET);
                cout << " from module ";
                cout << send_control_seq(COLOR_FG_WHITE) << lnk << send_control_seq(ATTR_RESET);
                cout << endl;
            }
        }
    }


    //////////////////////////////////////////////////////////////
    // EXTEND FUNCTION NAMES VECTOR WITH NAMES OF LINKED FUNCTIONS
    auto local_function_names = functions.names;
    for (string name : linked_function_names) { functions.names.emplace_back(name); }


    if (not flags.as_lib) {
        // check if our initial guess for main function is correct and
        // detect some main-function-related errors
        vector<string> main_function_found;
        for (auto f : functions.names) {
            if (f == "main/0" or f == "main/1" or f == "main/2") {
                main_function_found.emplace_back(f);
            }
        }
        if (main_function_found.size() > 1) {
            for (auto f : main_function_found) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_CYAN) << "note" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << f << send_control_seq(ATTR_RESET);
                cout << " function found in module ";
                cout << send_control_seq(COLOR_FG_WHITE) << symbol_sources.at(f) << send_control_seq(ATTR_RESET);
                cout << endl;
            }
            throw "more than one candidate for main function";
        } else if (main_function_found.size() == 0) {
            throw "main function is not defined";
        }
        main_function = main_function_found[0];
    }


    //////////////////////////
    // GENERATE ENTRY FUNCTION
    if (not flags.as_lib) {
        bytes = generate_entry_function(bytes, function_addresses, functions, main_function, starting_instruction);
    }


    uint64_t current_link_offset = bytes;
    for (string lnk : links) {
        if (DEBUG or VERBOSE) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_LIGHT_CYAN) << "message" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "[loader] linking with: '";
            cout << send_control_seq(COLOR_FG_WHITE) << lnk << send_control_seq(ATTR_RESET);
            cout << "'" << endl;
        }

        Loader loader(lnk);
        loader.load();

        vector<string> fn_names = loader.getFunctions();

        vector<uint64_t> lib_jumps = loader.getJumps();
        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "[loader] entries in jump table: " << lib_jumps.size() << endl;
            for (unsigned i = 0; i < lib_jumps.size(); ++i) {
                cout << "  jump at byte: " << lib_jumps[i] << endl;
            }
        }

        linked_libs_jumptables[lnk] = lib_jumps;

        map<string, uint64_t> fn_addresses = loader.getFunctionAddresses();
        for (string fn : fn_names) {
            function_addresses[fn] = fn_addresses.at(fn) + current_link_offset;
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "\"" << send_control_seq(COLOR_FG_LIGHT_GREEN) << fn << send_control_seq(ATTR_RESET) << "\": ";
                cout << "entry point at byte: " << current_link_offset << '+' << fn_addresses.at(fn);
                cout << endl;
            }
        }

        linked_libs_bytecode.emplace_back(lnk, loader.getBytecodeSize(), loader.getBytecode());
        bytes += loader.getBytecodeSize();
    }


    /////////////////////////////////////////////////////////////////////////
    // AFTER HAVING OBTAINED LINKED NAMES, IT IS POSSIBLE TO VERIFY CALLS AND
    // CALLABLE (FUNCTIONS, CLOSURES, ETC.) CREATIONS
    assembler::verify::functionCallsAreDefined(tokens, functions.names, functions.signatures);
    assembler::verify::callableCreations(tokens, functions.names, functions.signatures);


    /////////////////////////////
    // REPORT TOTAL BYTECODE SIZE
    if ((VERBOSE or DEBUG) and linked_function_names.size() != 0) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "total required bytes: " << bytes << " bytes" << endl;
    }
    if (DEBUG) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "required bytes: " << (bytes-(bytes-current_link_offset)) << " local, ";
        cout << (bytes-current_link_offset) << " linked";
        cout << endl;
    }


    ///////////////////////////
    // REPORT FIRST INSTRUCTION
    if ((VERBOSE or DEBUG) and not flags.as_lib) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "first instruction pointer: " << starting_instruction << endl;
    }


    ////////////////////
    // CREATE JUMP TABLE
    vector<uint64_t> jump_table;


    /////////////////////////////////////////////////////////
    // GENERATE BYTECODE OF LOCAL FUNCTIONS AND BLOCKS
    //
    // BYTECODE IS GENERATED HERE BUT NOT YET WRITTEN TO FILE
    // THIS MUST BE GENERATED HERE TO OBTAIN FILL JUMP TABLE
    map<string, tuple<uint64_t, byte*> > functions_bytecode;
    map<string, tuple<uint64_t, byte*> > block_bodies_bytecode;
    uint64_t functions_section_size = 0;
    uint64_t block_bodies_section_size = 0;

    vector<tuple<uint64_t, uint64_t> > jump_positions;

    for (string name : blocks.names) {
        // do not generate bytecode for blocks that were linked
        if (find(linked_block_names.begin(), linked_block_names.end(), name) != linked_block_names.end()) { continue; }

        if (VERBOSE or DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "generating bytecode for block \"";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name << send_control_seq(ATTR_RESET);
            cout << '"';
        }
        uint64_t fun_bytes = 0;
        try {
            fun_bytes = viua::cg::tools::calculate_bytecode_size(blocks.tokens.at(name));
            if (VERBOSE or DEBUG) {
                cout << " (" << fun_bytes << " bytes at byte " << block_bodies_section_size << ')' << endl;
            }
        } catch (const string& e) {
            throw ("failed block size count (during pre-assembling): " + e);
        } catch (const std::out_of_range& e) {
            throw ("in block '" + name + "': " + e.what());
        }

        Program func(fun_bytes);
        func.setdebug(DEBUG).setscream(SCREAM);
        try {
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "assembling block '";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name << send_control_seq(ATTR_RESET);
                cout << "'\n";
            }
            assemble(func, blocks.tokens.at(name));
        } catch (const string& e) {
            throw ("in block '" + name + "': " + e);
        } catch (const char*& e) {
            throw ("in block '" + name + "': " + e);
        } catch (const std::out_of_range& e) {
            throw ("in block '" + name + "': " + e.what());
        }

        vector<uint64_t> jumps = func.jumps();
        vector<uint64_t> jumps_absolute = func.jumpsAbsolute();

        vector<tuple<uint64_t, uint64_t> > local_jumps;
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            local_jumps.emplace_back(jmp, block_bodies_section_size);
        }
        func.calculateJumps(local_jumps);

        byte* btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet write it to the file to conform to bytecode format)
        block_bodies_bytecode[name] = tuple<uint64_t, byte*>(func.size(), btcode);

        // extend jump table with jumps from current block
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "pushed relative jump to jump table: " << jmp << '+' << block_bodies_section_size << endl;
            }
            jump_table.emplace_back(jmp+block_bodies_section_size);
        }

        for (unsigned i = 0; i < jumps_absolute.size(); ++i) {
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "pushed absolute jump to jump table: " << jumps_absolute[i] << "+0" << endl;
            }
            jump_positions.emplace_back(jumps_absolute[i]+block_bodies_section_size, 0);
        }

        block_bodies_section_size += func.size();
    }

    // functions section size, must be offset by the size of block section
    functions_section_size = block_bodies_section_size;

    for (string name : functions.names) {
        // do not generate bytecode for functions that were linked
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) { continue; }

        if (VERBOSE or DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "generating bytecode for function \"";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name << send_control_seq(ATTR_RESET);
            cout << '"';
        }
        uint64_t fun_bytes = 0;
        try {
            fun_bytes = viua::cg::tools::calculate_bytecode_size(functions.tokens.at(name));
            if (VERBOSE or DEBUG) {
                cout << " (" << fun_bytes << " bytes at byte " << functions_section_size << ')' << endl;
            }
        } catch (const string& e) {
            throw ("failed function size count (during pre-assembling): " + e);
        } catch (const std::out_of_range& e) {
            throw e.what();
        }

        Program func(fun_bytes);
        func.setdebug(DEBUG).setscream(SCREAM);
        try {
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "assembling function '";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name << send_control_seq(ATTR_RESET);
                cout << "'\n";
            }
            assemble(func, functions.tokens.at(name));
        } catch (const string& e) {
            string msg = ("in function '"
                          + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                          + send_control_seq(ATTR_RESET)
                          + "': " + e
                          );
            throw msg;
        } catch (const char*& e) {
            string msg = ("in function '"
                          + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                          + send_control_seq(ATTR_RESET)
                          + "': " + e
                          );
            throw msg;
        } catch (const std::out_of_range& e) {
            string msg = ("in function '"
                          + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                          + send_control_seq(ATTR_RESET)
                          + "': " + e.what()
                          );
            throw msg;
        }

        vector<uint64_t> jumps = func.jumps();
        vector<uint64_t> jumps_absolute = func.jumpsAbsolute();

        vector<tuple<uint64_t, uint64_t> > local_jumps;
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            local_jumps.emplace_back(jmp, functions_section_size);
        }
        func.calculateJumps(local_jumps);

        byte* btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet write it to the file to conform to bytecode format)
        functions_bytecode[name] = tuple<uint64_t, byte*>{func.size(), btcode};

        // extend jump table with jumps from current function
        for (unsigned i = 0; i < jumps.size(); ++i) {
            uint64_t jmp = jumps[i];
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "pushed relative jump to jump table: " << jmp << '+' << functions_section_size << endl;
            }
            jump_table.emplace_back(jmp+functions_section_size);
        }

        for (unsigned i = 0; i < jumps_absolute.size(); ++i) {
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "pushed absolute jump to jump table: " << jumps_absolute[i] << "+0" << endl;
            }
            jump_positions.emplace_back(jumps_absolute[i]+functions_section_size, 0);
        }

        functions_section_size += func.size();
    }


    ////////////////////////////////////////
    // CREATE OFSTREAM TO WRITE BYTECODE OUT
    ofstream out(compilename, ios::out | ios::binary);

    out.write(VIUA_MAGIC_NUMBER, sizeof(char)*5);
    if (flags.as_lib) {
        out.write(&VIUA_LINKABLE, sizeof(ViuaBinaryType));
    } else {
        out.write(&VIUA_EXECUTABLE, sizeof(ViuaBinaryType));
    }


    /////////////////////////////////////////////////////////////
    // WRITE META-INFORMATION MAP
    auto meta_information_map = gatherMetaInformation(tokens);
    uint64_t meta_information_map_size = 0;
    for (auto each : meta_information_map) {
        meta_information_map_size += (each.first.size() + each.second.size() + 2);
    }

    bwrite(out, meta_information_map_size);
    if (DEBUG) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "writing meta information\n";
    }
    for (auto each : meta_information_map) {
        if (DEBUG) {
            cout << "  " << str::enquote(each.first) << ": " << str::enquote(each.second) << endl;
        }
        strwrite(out, each.first);
        strwrite(out, each.second);
    }


    //////////////////////////
    // IF ASSEMBLING A LIBRARY
    // WRITE OUT JUMP TABLE
    if (flags.as_lib) {
        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "jump table has " << jump_table.size() << " entries" << endl;
        }
        uint64_t total_jumps = jump_table.size();
        bwrite(out, total_jumps);

        uint64_t jmp;
        for (unsigned i = 0; i < total_jumps; ++i) {
            jmp = jump_table[i];
            bwrite(out, jmp);
        }
    }


    /////////////////////////////////////////////////////////////
    // WRITE EXTERNAL FUNCTION SIGNATURES
    uint64_t signatures_section_size = 0;
    for (const auto each : functions.signatures) {
        signatures_section_size += (each.size() + 1); // +1 for null byte after each signature
    }
    bwrite(out, signatures_section_size);
    for (const auto each : functions.signatures) {
        strwrite(out, each);
    }


    /////////////////////////////////////////////////////////////
    // WRITE EXTERNAL BLOCK SIGNATURES
    signatures_section_size = 0;
    for (const auto each : blocks.signatures) {
        signatures_section_size += (each.size() + 1); // +1 for null byte after each signature
    }
    bwrite(out, signatures_section_size);
    for (const auto each : blocks.signatures) {
        strwrite(out, each);
    }


    /////////////////////////////////////////////////////////////
    // WRITE BLOCK AND FUNCTION ENTRY POINT ADDRESSES TO BYTECODE
    uint64_t functions_size_so_far = writeCodeBlocksSection(out, blocks, linked_block_names);
    functions_size_so_far = writeCodeBlocksSection(out, functions, linked_function_names, functions_size_so_far);
    for (string name : linked_function_names) {
        strwrite(out, name);
        // mapped address must come after name
        uint64_t address = function_addresses[name];
        bwrite(out, address);
    }


    //////////////////////
    // WRITE BYTECODE SIZE
    bwrite(out, bytes);

    byte* program_bytecode = new byte[bytes];
    uint64_t program_bytecode_used = 0;

    ////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL BLOCKS TO BYTECODE BUFFER
    for (string name : blocks.names) {
        // linked blocks are to be inserted later
        if (find(linked_block_names.begin(), linked_block_names.end(), name) != linked_block_names.end()) { continue; }

        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "pushing bytecode of local block '";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name << send_control_seq(ATTR_RESET);
            cout << "' to final byte array" << endl;
        }
        uint64_t fun_size = 0;
        byte* fun_bytecode = nullptr;
        tie(fun_size, fun_bytecode) = block_bodies_bytecode[name];

        for (uint64_t i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used+i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }


    ///////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL FUNCTIONS TO BYTECODE BUFFER
    for (string name : functions.names) {
        // linked functions are to be inserted later
        if (find(linked_function_names.begin(), linked_function_names.end(), name) != linked_function_names.end()) { continue; }

        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "pushing bytecode of local function '";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name << send_control_seq(ATTR_RESET);
            cout << "' to final byte array" << endl;
        }
        uint64_t fun_size = 0;
        byte* fun_bytecode = nullptr;
        tie(fun_size, fun_bytecode) = functions_bytecode[name];

        for (uint64_t i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used+i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }

    // free memory allocated for bytecode of local functions
    for (pair<string, tuple<uint64_t, byte*>> fun : functions_bytecode) {
        delete[] get<1>(fun.second);
    }

    Program calculator(bytes);
    calculator.setdebug(DEBUG).setscream(SCREAM);
    if (DEBUG) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "calculating absolute jumps..." << endl;
    }
    calculator.fill(program_bytecode).calculateJumps(jump_positions);


    ////////////////////////////////////
    // WRITE STATICALLY LINKED LIBRARIES
    uint64_t bytes_offset = current_link_offset;
    for (tuple<string, uint64_t, byte*> lnk : linked_libs_bytecode) {
        string lib_name;
        byte* linked_bytecode;
        uint64_t linked_size;
        tie(lib_name, linked_size, linked_bytecode) = lnk;

        if (VERBOSE or DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "linked module \"";
            cout << send_control_seq(COLOR_FG_WHITE) << lib_name << send_control_seq(ATTR_RESET);
            cout << "\" written at offset " << bytes_offset << endl;
        }

        vector<uint64_t> linked_jumptable;
        try {
            linked_jumptable = linked_libs_jumptables[lib_name];
        } catch (const std::out_of_range& e) {
            throw ("[linker] could not find jumptable for '" + lib_name + "' (maybe not loaded?)");
        }

        uint64_t jmp, jmp_target;
        for (unsigned i = 0; i < linked_jumptable.size(); ++i) {
            jmp = linked_jumptable[i];
            // we know what we're doing here
            jmp_target = *reinterpret_cast<uint64_t*>(linked_bytecode+jmp);
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug" << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "adjusting jump: at position " << jmp << ", " << jmp_target << '+' << bytes_offset << " -> " << (jmp_target+bytes_offset) << endl;
            }
            *reinterpret_cast<uint64_t*>(linked_bytecode+jmp) += bytes_offset;
        }

        for (decltype(linked_size) i = 0; i < linked_size; ++i) {
            program_bytecode[program_bytecode_used+i] = linked_bytecode[i];
        }
        program_bytecode_used += linked_size;
    }

    out.write(reinterpret_cast<const char*>(program_bytecode), static_cast<std::streamsize>(bytes));
    out.close();

    return 0;
}
