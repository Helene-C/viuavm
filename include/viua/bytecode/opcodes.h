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

#ifndef VIUA_BYTECODE_OPCODES_H
#define VIUA_BYTECODE_OPCODES_H

#pragma once

#include <viua/bytecode/bytetypedef.h>

enum OPCODE : viua::internals::types::byte {
    NOP = 0,    // do nothing

    // integer instructions
    IZERO,
    ISTORE,
    IINC,
    IDEC,

    // float instructions
    FSTORE,

    // numeric conversion instructions
    ITOF,   // convert integer to float
    FTOI,   // convert float to integer
    STOI,   // convert string to integer
    STOF,   // convert string to float

    ADD,
    SUB,
    MUL,
    DIV,
    LT,
    LTE,
    GT,
    GTE,
    EQ,

    // string instructions
    STRSTORE,
    STREQ,

    /*
     *  Store a text value in a register.
     *
     *  text {target-register} "<text>"
     */
    TEXT,

    /*
     *  Compare two text values for equality.
     *
     *  texteq {result-register} {lhs-register} {rhs-register}
     */
    TEXTEQ,

    /*
     *  Return copy of the character at a given index in text.
     *
     *  textat {result-register} {string-register} {index-register}
     */
    TEXTAT,

    /*
     *  Return a copy of a part of the given text between given indexes.
     *
     *  textsub {result-register} {string-register} {begin-index-register} {end-index:register|void}
     */
    TEXTSUB,

    /*
     *  Return length of a given text value (in characters).
     *
     *  textlength {result-register} {string-register}
     */
    TEXTLENGTH,

    /*
     *  Return length of common prefix of two text values.
     *
     *  textcommonprefix {result-register} {lhs-string-register} {rhs-string-register}
     */
    TEXTCOMMONPREFIX,

    /*
     *  Return length of common suffix of two text values.
     *
     *  textcommonsuffix {result-register} {lhs-string-register} {rhs-string-register}
     */
    TEXTCOMMONSUFFIX,

    /*
     *  Return a view of a text value between two indexes.
     *  It is a non-copying TEXTSUB.
     *  Values returned by TEXTVIEW are treated as regular text values by TEXT* instructions, so
     *  for example TEXTCONCAT of two views will result in a copy of viewed text.
     *
     *  textview {result-register} {string-register} {begin-index-register} {end-index:register|void}
     */
    TEXTVIEW,

    /*
     *  Concatenate two text values. Creates a copy of each text value.
     *
     *  textconcat {result-register} {lhs-string-register} {rhs-string-register}
     */
    TEXTCONCAT,

    VEC,
    VINSERT,
    VPUSH,
    VPOP,
    VAT,
    VLEN,

    // booleans
    BOOL,   // store Boolean false object in given register (empty) or
            // convert an object to Boolean value
    NOT,
    AND,
    OR,

    // register manipulation
    MOVE,   // move an object from one register to another
    COPY,   // copy an object from one register to another
    PTR,    // create a pointer to an object
    SWAP,   // swap two objects between registers
    DELETE, // delete an object from a register, freeing the memory
    ISNULL, // checks if register is null (empty)
    RESS,   // REgister Set Switch - switches register set

    PRINT,
    ECHO,

    CAPTURE,
    CAPTURECOPY,
    CAPTUREMOVE,
    CLOSURE,

    FUNCTION,

    // Opcodes related to functions.
    FRAME,  // create new frame (required before param and pamv) for future function call
    PARAM,  // copy object from a register to parameter register (pass-by-value),
    PAMV,   // move object from a register to parameter register (pass-by-move),
    CALL,   // call given function with parameters set in parameter register,
    TAILCALL,   // perform a tail call to a function
    ARG,    // move an object from argument register to a normal register (inside a function call),
    ARGC,   // store number of supplied parameters in a register

    PROCESS, // spawn a process (call a function and run it in a different process)
    SELF,   // store a PID of the running process in a register
    JOIN, // join a process
    SEND,   // send a message to a process
    RECEIVE, // receive passed message, block until one arrives
    WATCHDOG,  // spawn watchdog process

    JUMP,
    IF,

    THROW,      // throw an object
    CATCH,      // register a catcher block for given type
    DRAW,       // move caught object into a register (it becomes local object for current frame)
                // the "draw" name has been chosen because the sequence of events:
                //
                //  1) an object is thrown
                //  2) that object is caught
                //  3) the catcher draws the object into a register
                //
                // nicely describes the situation these instructions model.

    TRY,        // create a frame for try block
    ENTER,      // enter a block, if an exception is thrown and no catcher claims it, it is propagated up
                // ENTER instructions do not require any CATCH to precede them
    LEAVE,      // leave a block and resume execution after last enter instruction

    IMPORT,     // dynamically link foreign library
    LINK,       // dynamically link native library

    CLASS,      // create a prototype for new class
    DERIVE,     // derive a prototype from an existing class
    ATTACH,     // attach a method to the prototype
    REGISTER,   // register a prototype in VM's typesystem

    NEW,        // construct new instance of a class in a register
    MSG,        // send a message to an object (used for dynamic dispatch, for static use plain "CALL")
    INSERT,     // insert an object as a value of an attribute of another object
    REMOVE,     // remove an attribute from an object

    RETURN,
    HALT,
};

#endif
