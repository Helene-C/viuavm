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

#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/types/exception.h>
#include <viua/process.h>
#include <viua/kernel/kernel.h>
using namespace std;


viua::internals::types::byte* viua::process::Process::dispatch(viua::internals::types::byte* addr) {
    /** Dispatches instruction at a pointer to its handler.
     */
    switch (static_cast<OPCODE>(*addr)) {
        case IZERO:
            addr = opizero(addr+1);
            break;
        case ISTORE:
            addr = opistore(addr+1);
            break;
        case IINC:
            addr = opiinc(addr+1);
            break;
        case IDEC:
            addr = opidec(addr+1);
            break;
        case FSTORE:
            addr = opfstore(addr+1);
            break;
        case ITOF:
            addr = opitof(addr+1);
            break;
        case FTOI:
            addr = opftoi(addr+1);
            break;
        case STOI:
            addr = opstoi(addr+1);
            break;
        case STOF:
            addr = opstof(addr+1);
            break;
        case ADD:
            addr = opadd(addr+1);
            break;
        case SUB:
            addr = opsub(addr+1);
            break;
        case MUL:
            addr = opmul(addr+1);
            break;
        case DIV:
            addr = opdiv(addr+1);
            break;
        case LT:
            addr = oplt(addr+1);
            break;
        case LTE:
            addr = oplte(addr+1);
            break;
        case GT:
            addr = opgt(addr+1);
            break;
        case GTE:
            addr = opgte(addr+1);
            break;
        case EQ:
            addr = opeq(addr+1);
            break;
        case STRSTORE:
            addr = opstrstore(addr+1);
            break;
        case TEXT:
            addr = optext(addr+1);
            break;
        case TEXTEQ:
            addr = optexteq(addr+1);
            break;
        case TEXTAT:
            addr = optextat(addr+1);
            break;
        case VEC:
            addr = opvec(addr+1);
            break;
        case VINSERT:
            addr = opvinsert(addr+1);
            break;
        case VPUSH:
            addr = opvpush(addr+1);
            break;
        case VPOP:
            addr = opvpop(addr+1);
            break;
        case VAT:
            addr = opvat(addr+1);
            break;
        case VLEN:
            addr = opvlen(addr+1);
            break;
        case NOT:
            addr = opnot(addr+1);
            break;
        case AND:
            addr = opand(addr+1);
            break;
        case OR:
            addr = opor(addr+1);
            break;
        case MOVE:
            addr = opmove(addr+1);
            break;
        case COPY:
            addr = opcopy(addr+1);
            break;
        case PTR:
            addr = opptr(addr+1);
            break;
        case SWAP:
            addr = opswap(addr+1);
            break;
        case DELETE:
            addr = opdelete(addr+1);
            break;
        case ISNULL:
            addr = opisnull(addr+1);
            break;
        case RESS:
            addr = opress(addr+1);
            break;
        case PRINT:
              addr = opprint(addr+1);
            break;
        case ECHO:
              addr = opecho(addr+1);
            break;
        case CAPTURE:
              addr = opcapture(addr+1);
            break;
        case CAPTURECOPY:
            addr = opcapturecopy(addr+1);
            break;
        case CAPTUREMOVE:
            addr = opcapturemove(addr+1);
            break;
        case CLOSURE:
              addr = opclosure(addr+1);
            break;
        case FUNCTION:
              addr = opfunction(addr+1);
            break;
        case FRAME:
              addr = opframe(addr+1);
            break;
        case PARAM:
              addr = opparam(addr+1);
            break;
        case PAMV:
            addr = oppamv(addr+1);
            break;
        case ARG:
              addr = oparg(addr+1);
            break;
        case ARGC:
              addr = opargc(addr+1);
            break;
        case CALL:
              addr = opcall(addr+1);
            break;
        case TAILCALL:
              addr = optailcall(addr+1);
            break;
        case PROCESS:
            addr = opprocess(addr+1);
            break;
        case SELF:
            addr = opself(addr+1);
            break;
        case JOIN:
            addr = opjoin(addr+1);
            break;
        case SEND:
            addr = opsend(addr+1);
            break;
        case RECEIVE:
            addr = opreceive(addr+1);
            break;
        case WATCHDOG:
            addr = opwatchdog(addr+1);
            break;
        case RETURN:
            addr = opreturn(addr);
            break;
        case JUMP:
              addr = opjump(addr+1);
            break;
        case IF:
              addr = opif(addr+1);
            break;
        case TRY:
            addr = optry(addr+1);
            break;
        case CATCH:
            addr = opcatch(addr+1);
            break;
        case DRAW:
              addr = opdraw(addr+1);
            break;
        case ENTER:
            addr = openter(addr+1);
            break;
        case THROW:
            addr = opthrow(addr+1);
            break;
        case LEAVE:
              addr = opleave(addr+1);
            break;
        case IMPORT:
            addr = opimport(addr+1);
            break;
        case LINK:
            addr = oplink(addr+1);
            break;
        case CLASS:
            addr = opclass(addr+1);
            break;
        case DERIVE:
            addr = opderive(addr+1);
            break;
        case ATTACH:
            addr = opattach(addr+1);
            break;
        case REGISTER:
            addr = opregister(addr+1);
            break;
        case NEW:
            addr = opnew(addr+1);
            break;
        case MSG:
            addr = opmsg(addr+1);
            break;
        case INSERT:
            addr = opinsert(addr+1);
            break;
        case REMOVE:
            addr = opremove(addr+1);
            break;
        case HALT:
            throw HaltException();
            break;
        case NOP:
            ++addr;
            break;
        default:
            ostringstream error;
            error << "unrecognised instruction (byte value " << int(*addr) << ")";
            if (OP_NAMES.count(static_cast<OPCODE>(*addr))) {
                error << ": " << OP_NAMES.at(static_cast<OPCODE>(*addr));
            }
            throw new viua::types::Exception(error.str());
    }
    return addr;
}
