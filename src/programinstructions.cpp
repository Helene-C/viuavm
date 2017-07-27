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

#include <viua/bytecode/opcodes.h>
#include <viua/program.h>
#include <viua/support/pointer.h>
using namespace std;


Program& Program::opnop() {
    /*  Inserts nop instuction.
     */
    addr_ptr = cg::bytecode::opnop(addr_ptr);
    return (*this);
}

Program& Program::opizero(int_op regno) {
    /*  Inserts izero instuction.
     */
    addr_ptr = cg::bytecode::opizero(addr_ptr, regno);
    return (*this);
}

Program& Program::opistore(int_op regno, int_op i) {
    /*  Inserts istore instruction to bytecode.
     *
     *  :params:
     *
     *  regno:int - register number
     *  i:int     - value to store
     */
    addr_ptr = cg::bytecode::opistore(addr_ptr, regno, i);
    return (*this);
}

Program& Program::opiinc(int_op regno) {
    /*  Inserts iinc instuction.
     */
    addr_ptr = cg::bytecode::opiinc(addr_ptr, regno);
    return (*this);
}

Program& Program::opidec(int_op regno) {
    /*  Inserts idec instuction.
     */
    addr_ptr = cg::bytecode::opidec(addr_ptr, regno);
    return (*this);
}

Program& Program::opfstore(int_op regno, viua::internals::types::plain_float f) {
    /*  Inserts fstore instruction to bytecode.
     *
     *  :params:
     *
     *  regno - register number
     *  f     - value to store
     */
    addr_ptr = cg::bytecode::opfstore(addr_ptr, regno, f);
    return (*this);
}

Program& Program::opitof(int_op a, int_op b) {
    /*  Inserts itof instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opitof(addr_ptr, a, b);
    return (*this);
}

Program& Program::opftoi(int_op a, int_op b) {
    /*  Inserts ftoi instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opftoi(addr_ptr, a, b);
    return (*this);
}

Program& Program::opstoi(int_op a, int_op b) {
    /*  Inserts stoi instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opstoi(addr_ptr, a, b);
    return (*this);
}

Program& Program::opstof(int_op a, int_op b) {
    /*  Inserts stof instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opstof(addr_ptr, a, b);
    return (*this);
}

Program& Program::opadd(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opsub(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opsub(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opmul(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opmul(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opdiv(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opdiv(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::oplt(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::oplt(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::oplte(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::oplte(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opgt(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opgt(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opgte(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opgte(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opeq(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opeq(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opstrstore(int_op reg, string s) {
    /*  Inserts strstore instruction.
     */
    addr_ptr = cg::bytecode::opstrstore(addr_ptr, reg, s);
    return (*this);
}

Program& Program::optext(int_op reg, string s) {
    addr_ptr = cg::bytecode::optext(addr_ptr, reg, s);
    return (*this);
}

Program& Program::optext(int_op a, int_op b) {
    addr_ptr = cg::bytecode::optext(addr_ptr, a, b);
    return (*this);
}

Program& Program::optexteq(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::optexteq(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::optextat(int_op target, int_op source, int_op index) {
    addr_ptr = cg::bytecode::optextat(addr_ptr, target, source, index);
    return (*this);
}
Program& Program::optextsub(int_op target, int_op source, int_op begin_index, int_op end_index) {
    addr_ptr = cg::bytecode::optextsub(addr_ptr, target, source, begin_index, end_index);
    return (*this);
}
Program& Program::optextlength(int_op target, int_op source) {
    addr_ptr = cg::bytecode::optextlength(addr_ptr, target, source);
    return (*this);
}
Program& Program::optextcommonprefix(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::optextcommonprefix(addr_ptr, target, lhs, rhs);
    return (*this);
}
Program& Program::optextcommonsuffix(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::optextcommonsuffix(addr_ptr, target, lhs, rhs);
    return (*this);
}
Program& Program::optextconcat(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::optextconcat(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opvec(int_op index, int_op pack_start_index, int_op pack_length) {
    addr_ptr = cg::bytecode::opvec(addr_ptr, index, pack_start_index, pack_length);
    return (*this);
}

Program& Program::opvinsert(int_op vec, int_op src, int_op dst) {
    addr_ptr = cg::bytecode::opvinsert(addr_ptr, vec, src, dst);
    return (*this);
}

Program& Program::opvpush(int_op vec, int_op src) {
    addr_ptr = cg::bytecode::opvpush(addr_ptr, vec, src);
    return (*this);
}

Program& Program::opvpop(int_op vec, int_op dst, int_op pos) {
    addr_ptr = cg::bytecode::opvpop(addr_ptr, vec, dst, pos);
    return (*this);
}

Program& Program::opvat(int_op vec, int_op dst, int_op at) {
    addr_ptr = cg::bytecode::opvat(addr_ptr, vec, dst, at);
    return (*this);
}

Program& Program::opvlen(int_op vec, int_op reg) {
    addr_ptr = cg::bytecode::opvlen(addr_ptr, vec, reg);
    return (*this);
}

Program& Program::opnot(int_op target, int_op source) {
    addr_ptr = cg::bytecode::opnot(addr_ptr, target, source);
    return (*this);
}

Program& Program::opand(int_op regr, int_op rega, int_op regb) {
    addr_ptr = cg::bytecode::opand(addr_ptr, regr, rega, regb);
    return (*this);
}

Program& Program::opor(int_op regr, int_op rega, int_op regb) {
    addr_ptr = cg::bytecode::opor(addr_ptr, regr, rega, regb);
    return (*this);
}

Program& Program::opbits(int_op target, int_op count) {
    addr_ptr = cg::bytecode::opbits(addr_ptr, target, count);
    return (*this);
}

Program& Program::opbits(int_op target, const vector<uint8_t> bit_string) {
    addr_ptr = cg::bytecode::opbits(addr_ptr, target, bit_string);
    return (*this);
}

Program& Program::opbitand(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opbitand(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opbitor(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opbitor(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opbitnot(int_op target, int_op lhs) {
    addr_ptr = cg::bytecode::opbitnot(addr_ptr, target, lhs);
    return (*this);
}

Program& Program::opbitxor(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opbitxor(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opbitat(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opbitat(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opbitset(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opbitset(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opbitset(int_op target, int_op lhs, bool rhs) {
    addr_ptr = cg::bytecode::opbitset(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opshl(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opshl(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opshr(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opshr(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opashl(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opashl(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opashr(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opashr(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::oprol(int_op target, int_op count) {
    addr_ptr = cg::bytecode::oprol(addr_ptr, target, count);
    return (*this);
}

Program& Program::opror(int_op target, int_op count) {
    addr_ptr = cg::bytecode::opror(addr_ptr, target, count);
    return (*this);
}

Program& Program::opfixedincrement(int_op target) {
    addr_ptr = cg::bytecode::opfixedincrement(addr_ptr, target);
    return (*this);
}

Program& Program::opfixeddecrement(int_op target) {
    addr_ptr = cg::bytecode::opfixeddecrement(addr_ptr, target);
    return (*this);
}

Program& Program::opfixedadd(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opfixedadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opmove(int_op a, int_op b) {
    /*  Inserts move instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number (move from...)
     *  b - register number (move to...)
     */
    addr_ptr = cg::bytecode::opmove(addr_ptr, a, b);
    return (*this);
}

Program& Program::opcopy(int_op a, int_op b) {
    /*  Inserts copy instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number (copy from...)
     *  b - register number (copy to...)
     */
    addr_ptr = cg::bytecode::opcopy(addr_ptr, a, b);
    return (*this);
}

Program& Program::opptr(int_op a, int_op b) {
    addr_ptr = cg::bytecode::opptr(addr_ptr, a, b);
    return (*this);
}

Program& Program::opswap(int_op a, int_op b) {
    /*  Inserts swap instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::opswap(addr_ptr, a, b);
    return (*this);
}

Program& Program::opdelete(int_op reg) {
    /*  Inserts delete instuction.
     */
    addr_ptr = cg::bytecode::opdelete(addr_ptr, reg);
    return (*this);
}

Program& Program::opisnull(int_op a, int_op b) {
    /*  Inserts isnull instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::opisnull(addr_ptr, a, b);
    return (*this);
}

Program& Program::opress(string a) {
    /*  Inserts ress instruction to bytecode.
     *
     *  :params:
     *
     *  a - register set ID
     */
    addr_ptr = cg::bytecode::opress(addr_ptr, a);
    return (*this);
}

Program& Program::opprint(int_op reg) {
    /*  Inserts print instuction.
     */
    addr_ptr = cg::bytecode::opprint(addr_ptr, reg);
    return (*this);
}

Program& Program::opecho(int_op reg) {
    /*  Inserts echo instuction.
     */
    addr_ptr = cg::bytecode::opecho(addr_ptr, reg);
    return (*this);
}

Program& Program::opcapture(int_op target_closure, int_op target_register, int_op source_register) {
    /*  Inserts clbing instuction.
     */
    addr_ptr = cg::bytecode::opcapture(addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

Program& Program::opcapturecopy(int_op target_closure, int_op target_register, int_op source_register) {
    /*  Inserts opcapturecopy instuction.
     */
    addr_ptr = cg::bytecode::opcapturecopy(addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

Program& Program::opcapturemove(int_op target_closure, int_op target_register, int_op source_register) {
    /*  Inserts opcapturemove instuction.
     */
    addr_ptr = cg::bytecode::opcapturemove(addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

Program& Program::opclosure(int_op reg, const string& fn) {
    /*  Inserts closure instuction.
     */
    addr_ptr = cg::bytecode::opclosure(addr_ptr, reg, fn);
    return (*this);
}

Program& Program::opfunction(int_op reg, const string& fn) {
    /*  Inserts function instuction.
     */
    addr_ptr = cg::bytecode::opfunction(addr_ptr, reg, fn);
    return (*this);
}

Program& Program::opframe(int_op a, int_op b) {
    /*  Inserts frame instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opframe(addr_ptr, a, b);
    return (*this);
}

Program& Program::opparam(int_op a, int_op b) {
    /*  Inserts param instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::opparam(addr_ptr, a, b);
    return (*this);
}

Program& Program::oppamv(int_op a, int_op b) {
    /*  Inserts pamv instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::oppamv(addr_ptr, a, b);
    return (*this);
}

Program& Program::oparg(int_op a, int_op b) {
    /*  Inserts arg instruction to bytecode.
     *
     *  :params:
     *
     *  a - argument number
     *  b - register number
     */
    addr_ptr = cg::bytecode::oparg(addr_ptr, a, b);
    return (*this);
}

Program& Program::opargc(int_op a) {
    /*  Inserts argc instruction to bytecode.
     *
     *  :params:
     *
     *  a - target register
     */
    addr_ptr = cg::bytecode::opargc(addr_ptr, a);
    return (*this);
}

Program& Program::opcall(int_op reg, const string& fn_name) {
    /*  Inserts call instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::opcall(addr_ptr, reg, fn_name);
    return (*this);
}

Program& Program::opcall(int_op reg, int_op fn) {
    /*  Inserts call instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::opcall(addr_ptr, reg, fn);
    return (*this);
}

Program& Program::optailcall(const string& fn_name) {
    /*  Inserts tailcall instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::optailcall(addr_ptr, fn_name);
    return (*this);
}

Program& Program::optailcall(int_op fn) {
    addr_ptr = cg::bytecode::optailcall(addr_ptr, fn);
    return (*this);
}

Program& Program::opdefer(const string& fn_name) {
    addr_ptr = cg::bytecode::opdefer(addr_ptr, fn_name);
    return (*this);
}

Program& Program::opdefer(int_op fn) {
    addr_ptr = cg::bytecode::opdefer(addr_ptr, fn);
    return (*this);
}

Program& Program::opprocess(int_op ref, const string& fn_name) {
    addr_ptr = cg::bytecode::opprocess(addr_ptr, ref, fn_name);
    return (*this);
}

Program& Program::opprocess(int_op reg, int_op fn) {
    addr_ptr = cg::bytecode::opprocess(addr_ptr, reg, fn);
    return (*this);
}

Program& Program::opself(int_op target) {
    /*  Inserts self instuction.
     */
    addr_ptr = cg::bytecode::opself(addr_ptr, target);
    return (*this);
}

Program& Program::opjoin(int_op target, int_op source, timeout_op timeout) {
    addr_ptr = cg::bytecode::opjoin(addr_ptr, target, source, timeout);
    return (*this);
}

Program& Program::opsend(int_op target, int_op source) {
    addr_ptr = cg::bytecode::opsend(addr_ptr, target, source);
    return (*this);
}

Program& Program::opreceive(int_op ref, timeout_op timeout) {
    addr_ptr = cg::bytecode::opreceive(addr_ptr, ref, timeout);
    return (*this);
}

Program& Program::opwatchdog(const string& fn_name) {
    addr_ptr = cg::bytecode::opwatchdog(addr_ptr, fn_name);
    return (*this);
}

Program& Program::opjump(viua::internals::types::bytecode_size addr, enum JUMPTYPE is_absolute) {
    /*  Inserts jump instruction. Parameter is instruction index.
     *  Byte offset is calculated automatically.
     *
     *  :params:
     *
     *  addr:int    - index of the instruction to which to branch
     */
    if (is_absolute != JMP_TO_BYTE) {
        branches.push_back((addr_ptr + 1));
    }

    addr_ptr = cg::bytecode::opjump(addr_ptr, addr);
    return (*this);
}

Program& Program::opif(int_op regc, viua::internals::types::bytecode_size addr_truth,
                       enum JUMPTYPE absolute_truth, viua::internals::types::bytecode_size addr_false,
                       enum JUMPTYPE absolute_false) {
    /*  Inserts branch instruction.
     *  Byte offset is calculated automatically.
     */
    viua::internals::types::byte* jump_position_in_bytecode = addr_ptr;

    jump_position_in_bytecode += sizeof(viua::internals::types::byte);   // for opcode
    jump_position_in_bytecode += sizeof(viua::internals::types::byte);   // for operand-type marker
    jump_position_in_bytecode += sizeof(viua::internals::RegisterSets);  // for rs-type marker
    jump_position_in_bytecode += sizeof(viua::internals::types::register_index);

    if (absolute_truth != JMP_TO_BYTE) {
        branches.push_back(jump_position_in_bytecode);
    }
    jump_position_in_bytecode += sizeof(viua::internals::types::bytecode_size);

    if (absolute_false != JMP_TO_BYTE) {
        branches.push_back(jump_position_in_bytecode);
    }

    addr_ptr = cg::bytecode::opif(addr_ptr, regc, addr_truth, addr_false);
    return (*this);
}

Program& Program::optry() {
    /*  Inserts try instruction.
     */
    addr_ptr = cg::bytecode::optry(addr_ptr);
    return (*this);
}

Program& Program::opcatch(string type_name, string block_name) {
    /*  Inserts catch instruction.
     */
    addr_ptr = cg::bytecode::opcatch(addr_ptr, type_name, block_name);
    return (*this);
}

Program& Program::opdraw(int_op regno) {
    /*  Inserts throw instuction.
     */
    addr_ptr = cg::bytecode::opdraw(addr_ptr, regno);
    return (*this);
}

Program& Program::openter(string block_name) {
    /*  Inserts enter instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::openter(addr_ptr, block_name);
    return (*this);
}

Program& Program::opthrow(int_op regno) {
    /*  Inserts throw instuction.
     */
    addr_ptr = cg::bytecode::opthrow(addr_ptr, regno);
    return (*this);
}

Program& Program::opleave() {
    /*  Inserts leave instruction.
     */
    addr_ptr = cg::bytecode::opleave(addr_ptr);
    return (*this);
}

Program& Program::opimport(string module_name) {
    /*  Inserts import instruction.
     */
    addr_ptr = cg::bytecode::opimport(addr_ptr, module_name);
    return (*this);
}

Program& Program::opclass(int_op reg, const string& class_name) {
    /*  Inserts class instuction.
     */
    addr_ptr = cg::bytecode::opclass(addr_ptr, reg, class_name);
    return (*this);
}

Program& Program::opderive(int_op reg, const string& base_class_name) {
    /*  Inserts derive instuction.
     */
    addr_ptr = cg::bytecode::opderive(addr_ptr, reg, base_class_name);
    return (*this);
}

Program& Program::opattach(int_op reg, const string& function_name, const string& method_name) {
    /*  Inserts attach instuction.
     */
    addr_ptr = cg::bytecode::opattach(addr_ptr, reg, function_name, method_name);
    return (*this);
}

Program& Program::opregister(int_op reg) {
    /*  Inserts register instuction.
     */
    addr_ptr = cg::bytecode::opregister(addr_ptr, reg);
    return (*this);
}

Program& Program::opatom(int_op reg, string s) {
    addr_ptr = cg::bytecode::opatom(addr_ptr, reg, s);
    return (*this);
}

Program& Program::opatomeq(int_op target, int_op lhs, int_op rhs) {
    addr_ptr = cg::bytecode::opatomeq(addr_ptr, target, lhs, rhs);
    return (*this);
}

Program& Program::opstruct(int_op regno) {
    addr_ptr = cg::bytecode::opstruct(addr_ptr, regno);
    return (*this);
}

Program& Program::opstructinsert(int_op target, int_op key, int_op source) {
    addr_ptr = cg::bytecode::opstructinsert(addr_ptr, target, key, source);
    return (*this);
}

Program& Program::opstructremove(int_op target, int_op key, int_op source) {
    addr_ptr = cg::bytecode::opstructremove(addr_ptr, target, key, source);
    return (*this);
}

Program& Program::opstructkeys(int_op a, int_op b) {
    addr_ptr = cg::bytecode::opstructkeys(addr_ptr, a, b);
    return (*this);
}

Program& Program::opnew(int_op reg, const string& class_name) {
    addr_ptr = cg::bytecode::opnew(addr_ptr, reg, class_name);
    return (*this);
}

Program& Program::opmsg(int_op reg, const string& method_name) {
    /*  Inserts msg instuction.
     */
    addr_ptr = cg::bytecode::opmsg(addr_ptr, reg, method_name);
    return (*this);
}

Program& Program::opmsg(int_op reg, int_op method_name) {
    /*  Inserts msg instuction.
     */
    addr_ptr = cg::bytecode::opmsg(addr_ptr, reg, method_name);
    return (*this);
}

Program& Program::opinsert(int_op target, int_op key, int_op source) {
    addr_ptr = cg::bytecode::opinsert(addr_ptr, target, key, source);
    return (*this);
}

Program& Program::opremove(int_op target, int_op key, int_op source) {
    addr_ptr = cg::bytecode::opremove(addr_ptr, target, key, source);
    return (*this);
}

Program& Program::opreturn() {
    addr_ptr = cg::bytecode::opreturn(addr_ptr);
    return (*this);
}

Program& Program::ophalt() {
    /*  Inserts halt instruction.
     */
    addr_ptr = cg::bytecode::ophalt(addr_ptr);
    return (*this);
}
