#ifndef TATANKA_PROGRAM_H
#define TATANKA_PROGRAM_H

#include <string>
#include "bytecode.h"

typedef char byte;

class Program {
    byte* program;
    int bytes;

    int addr_no;
    byte* addr_ptr;


    void ensurebytes(int n);

    public:
    // instructions interface
    Program& istore     (int, int);
    Program& iadd       ();
    Program& isub       ();
    Program& imul       ();
    Program& idiv       ();
    Program& iinc       ();
    Program& idec       ();
    Program& ilt        ();
    Program& ilte       ();
    Program& igt        ();
    Program& igte       ();
    Program& ieq        ();
    Program& print      ();
    Program& branch     ();
    Program& branchif   ();
    Program& ret        ();
    Program& end        ();
    Program& pass       ();
    Program& halt       ();

    Program& setAddressPtr(int n = 0);

    // byte array manipulation
    void expand(int n = 0);

    // representations
    byte* bytecode();
    std::string assembler();


    int size();


    Program(int bts = 2): bytes(bts) {
        program = new byte[bytes];
        for (int i = 0; i < bytes; ++i) { program[i] = PASS; }

        addr_no = 0;
        addr_ptr = program+addr_no;
    }
    ~Program() {
        delete[] program;
    }
};


#endif
