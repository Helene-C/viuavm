#include <iostream>
#include <vector>
#include "../bytecode/bytetypedef.h"
#include "../bytecode/opcodes.h"
#include "../bytecode/maps.h"
#include "../types/object.h"
#include "../types/integer.h"
#include "../types/byte.h"
#include "../support/pointer.h"
#include "cpu.h"
using namespace std;


CPU& CPU::load(byte* bc) {
    /*  Load bytecode into the CPU.
     *  CPU becomes owner of loaded bytecode - meaning it will consider itself responsible for proper
     *  destruction of it, so make sure you have a copy of the bytecode.
     *
     *  Any previously loaded bytecode is freed.
     *  To free bytecode without loading anything new it is possible to call .load(0).
     *
     *  :params:
     *
     *  bc:char*    - pointer to byte array containing bytecode with a program to run
     */
    if (bytecode) { delete[] bytecode; }
    bytecode = bc;
    return (*this);
}

CPU& CPU::bytes(uint16_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}

CPU& CPU::eoffset(uint16_t o) {
    /*  Set offset of first executable instruction.
     */
    executable_offset = o;
    return (*this);
}

CPU& CPU::mapfunction(const string& name, unsigned address) {
    /** Maps function name to bytecode address.
     */
    function_addresses[name] = address;
    return (*this);
}


Object* CPU::fetch(int index) {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to fetch
     */
    if (index >= uregisters_size) { throw "register access out of bounds: read"; }
    Object* optr = uregisters[index];
    if (optr == 0) {
        ostringstream oss;
        oss << "read from null register: " << index;
        throw oss.str().c_str();
    }
    return optr;
}


template<class T> inline void copyvalue(Object* a, Object* b) {
    /** This is a short inline, template function to copy value between two `Object` pointers of the same polymorphic type.
     *  It is used internally by CPU.
     */
    static_cast<T>(a)->value() = static_cast<T>(b)->value();
}

void CPU::updaterefs(Object* before, Object* now) {
    /** This method updates references to a given address present in registers.
     *  It swaps old address for the new one in every register that points to the old address and
     *  is marked as a reference.
     */
    for (int i = 0; i < uregisters_size; ++i) {
        if (uregisters[i] == before and ureferences[i]) {
            if (debug) {
                cout << "\nCPU: updating reference address in register " << i << hex << ": 0x" << (unsigned long)before << " -> 0x" << (unsigned long)now << dec << endl;
            }
            uregisters[i] = now;
        }
    }
}

bool CPU::hasrefs(int index) {
    /** This method checks if object at a given address exists as a reference in another register.
     */
    bool has = false;
    for (int i = 0; i < uregisters_size; ++i) {
        if (i == index) continue;
        if (uregisters[i] == uregisters[index]) {
            has = true;
            break;
        }
    }
    return has;
}

void CPU::place(int index, Object* obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `Object` previously stored in it is destroyed.
     *
     */
    if (index >= uregisters_size) { throw "register access out of bounds: write"; }
    if (uregisters[index] != 0 and !ureferences[index]) {
        // register is not empty and is not a reference - the object in it must be destroyed to avoid memory leaks
        delete uregisters[index];
    }
    if (ureferences[index]) {
        Object* referenced = fetch(index);

        // it is a reference, copy value of the object
        if (referenced->type() == "Integer") { copyvalue<Integer*>(referenced, obj); }
        else if (referenced->type() == "Byte") { copyvalue<Byte*>(referenced, obj); }

        // and delete the newly created object to avoid leaks
        delete obj;
    } else {
        Object* old_ref_ptr = (hasrefs(index) ? uregisters[index] : 0);
        uregisters[index] = obj;
        if (old_ref_ptr) { updaterefs(old_ref_ptr, obj); }
    }
}


int CPU::run() {
    /*  VM CPU implementation.
     *
     *  A giant switch-in-while which iterates over bytecode and executes encoded instructions.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }
    int return_code = 0;

    bool halt = false;

    byte* instr_ptr = bytecode+executable_offset; // instruction pointer
    byte* previous_instr_ptr;

    // initial frame for entry function call
    Frame *initial_frame = new Frame(0, 0, 16);
    uregisters = initial_frame->registers;
    ureferences = initial_frame->references;
    uregisters_size = initial_frame->registers_size;
    frames.push_back(initial_frame);
    for (auto fn_addr : function_addresses) {
        if (fn_addr.first == "__entry") {
            // do not destroy last frame if it's entry function as it holds global registers
            destroy_last_frame = false;
            // delete global registers
            // FIXME: CPU shoul wait until now with allocating memory for global registers to see
            // if it's really necessary - unneeded new/delete would be avoided this way
            delete[] registers;
            delete[] references;
            registers = uregisters;
            references = ureferences;
            reg_count = uregisters_size;
            break;
        }
    }

    while (true) {
        previous_instr_ptr = instr_ptr;
        if (debug) {
            cout << "CPU: bytecode ";
            cout << dec << ((long)instr_ptr - (long)bytecode);
            cout << " at 0x" << hex << (long)instr_ptr;
            cout << dec << ": ";
        }

        try {
            if (debug) { cout << OP_NAMES.at(OPCODE(*instr_ptr)); }
            switch (*instr_ptr) {
                case ISTORE:
                    instr_ptr = istore(instr_ptr+1);
                    break;
                case IADD:
                    instr_ptr = iadd(instr_ptr+1);
                    break;
                case ISUB:
                    instr_ptr = isub(instr_ptr+1);
                    break;
                case IMUL:
                    instr_ptr = imul(instr_ptr+1);
                    break;
                case IDIV:
                    instr_ptr = idiv(instr_ptr+1);
                    break;
                case IINC:
                    instr_ptr = iinc(instr_ptr+1);
                    break;
                case IDEC:
                    instr_ptr = idec(instr_ptr+1);
                    break;
                case ILT:
                    instr_ptr = ilt(instr_ptr+1);
                    break;
                case ILTE:
                    instr_ptr = ilte(instr_ptr+1);
                    break;
                case IGT:
                    instr_ptr = igt(instr_ptr+1);
                    break;
                case IGTE:
                    instr_ptr = igte(instr_ptr+1);
                    break;
                case IEQ:
                    instr_ptr = ieq(instr_ptr+1);
                    break;
                case FSTORE:
                    instr_ptr = fstore(instr_ptr+1);
                    break;
                case FADD:
                    instr_ptr = fadd(instr_ptr+1);
                    break;
                case FSUB:
                    instr_ptr = fsub(instr_ptr+1);
                    break;
                case FMUL:
                    instr_ptr = fmul(instr_ptr+1);
                    break;
                case FDIV:
                    instr_ptr = fdiv(instr_ptr+1);
                    break;
                case FLT:
                    instr_ptr = flt(instr_ptr+1);
                    break;
                case FLTE:
                    instr_ptr = flte(instr_ptr+1);
                    break;
                case FGT:
                    instr_ptr = fgt(instr_ptr+1);
                    break;
                case FGTE:
                    instr_ptr = fgte(instr_ptr+1);
                    break;
                case FEQ:
                    instr_ptr = feq(instr_ptr+1);
                    break;
                case BSTORE:
                    instr_ptr = bstore(instr_ptr+1);
                    break;
                case ITOF:
                    instr_ptr = itof(instr_ptr+1);
                    break;
                case FTOI:
                    instr_ptr = ftoi(instr_ptr+1);
                    break;
                case NOT:
                    instr_ptr = lognot(instr_ptr+1);
                    break;
                case AND:
                    instr_ptr = logand(instr_ptr+1);
                    break;
                case OR:
                    instr_ptr = logor(instr_ptr+1);
                    break;
                case MOVE:
                    instr_ptr = move(instr_ptr+1);
                    break;
                case COPY:
                    instr_ptr = copy(instr_ptr+1);
                    break;
                case REF:
                    instr_ptr = ref(instr_ptr+1);
                    break;
                case SWAP:
                    instr_ptr = swap(instr_ptr+1);
                    break;
                case DELETE:
                    instr_ptr = del(instr_ptr+1);
                    break;
                case PRINT:
                    instr_ptr = print(instr_ptr+1);
                    break;
                case ECHO:
                    instr_ptr = echo(instr_ptr+1);
                    break;
                case FRAME:
                    instr_ptr = frame(instr_ptr+1);
                    break;
                case PARAM:
                    instr_ptr = param(instr_ptr+1);
                    break;
                case PAREF:
                    instr_ptr = paref(instr_ptr+1);
                    break;
                case ARG:
                    instr_ptr = arg(instr_ptr+1);
                    break;
                case CALL:
                    instr_ptr = call(instr_ptr+1);
                    break;
                case END:
                    instr_ptr = end(instr_ptr);
                    break;
                case JUMP:
                    instr_ptr = jump(instr_ptr+1);
                    break;
                case BRANCH:
                    instr_ptr = branch(instr_ptr+1);
                    break;
                case RET:
                    instr_ptr = ret(instr_ptr+1);
                    break;
                case HALT:
                    halt = true;
                    break;
                case PASS:
                    ++instr_ptr;
                    break;
                case NOP:
                    ++instr_ptr;
                    break;
                default:
                    ostringstream error;
                    error << "unrecognised instruction (bytecode value: " << *((int*)bytecode) << ")";
                    throw error.str().c_str();
            }
            if (debug and not stepping) { cout << endl; }
        } catch (const char*& e) {
            return_code = 1;
            cout << (debug ? "\n" : "") <<  "exception: " << e << endl;
            break;
        } catch (const string& e) {
            return_code = 1;
            cout << (debug ? "\n" : "") <<  "exception: " << e << endl;
            break;
        }

        if (halt or frames.size() == 0) break;

        if (instr_ptr >= (bytecode+bytecode_size)) {
            cout << "CPU: aborting: bytecode address out of bounds" << endl;
            return_code = 1;
            break;
        }

        if (instr_ptr == previous_instr_ptr and OPCODE(*instr_ptr) != END) {
            cout << "CPU: aborting: instruction pointer did not change, possibly an endless loop" << endl;
            cout << "note: instruction index was " << (long)(instr_ptr-bytecode) << " and the opcode was '" << OP_NAMES.at(OPCODE(*instr_ptr)) << "'" << endl;
            if (OPCODE(*instr_ptr) == CALL) {
                cout << "note: this was caused by 'call' opcode immediately calling itself\n"
                     << "      such situation may arise when a function which calls itself in its first instruction";
                cout << endl;
            }
            return_code = 2;
            break;
        }

        if (stepping) {
            string ins;
            getline(cin, ins);
        }
    }

    if (return_code == 0 and uregisters[0]) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        return_code = static_cast<Integer*>(uregisters[0])->value();
    }

    return return_code;
}
