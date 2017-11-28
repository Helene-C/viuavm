# Viua VM ![Latest Release](https://img.shields.io/github/tag/marekjm/viuavm.svg)

[![Build Status](https://travis-ci.org/marekjm/viuavm.svg)](https://travis-ci.org/marekjm/viuavm)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/7140.svg)](https://scan.coverity.com/projects/marekjm-viuavm)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/581/badge)](https://bestpractices.coreinfrastructure.org/projects/581)
[![Open issues](https://img.shields.io/github/issues/marekjm/viuavm.svg)](https://github.com/marekjm/viuavm/issues)

![License](https://img.shields.io/github/license/marekjm/viuavm.svg)


> A register-based, parallel virtual machine programmable in custom assembly lookalike language with
> strong emphasis on reliability, predictability, and concurrency.

#### Building the code

```
]$ ./configure      # (1) configure the project
]$ make -j 4        # (2) build it using 4 jobs
```

**Note**: the default build is run with `-O0` (without any optimisations).
You can compile Viua VM with higher optimisation leves, but keep in mind that the compilation might not
succeed on machines with 4GB or less RAM (with `-O3`).
This is tracked by [issue #184](https://github.com/marekjm/viuavm/issues/184) on GitHub, or
by issue `7c06177872c3a718510a54e6513820f8fe0fb99b` in the embedded issue repository.


#### Hello World in Viua VM

```
; Main function of the program, automatically invoked by the VM.
; It must be present in every executable file.
; Valid main functions are main/0, main/1 and main/2.
;
.function: main/0
    ; Store text value in register with index 1.
    ; The text instruction shares operand order with majority of
    ; other instructions:
    ;
    ;   <mnemonic> <target> <sources>...
    ;
    text %1 local "Hello World!"

    ; Print contents of a register to standard output.
    ; This is the most primitive form of output the VM supports, and
    ; should be used only in the simplest programs and
    ; for quick-and-dirty debugging.
    print %1 local

    ; The finishing sequence of every program running on Viua.
    ;
    ; Register 0 is used to store return value of a function, and
    ; the assembler will enforce that it is set to integer 0 for main
    ; function.
    izero %0 local
    return
.end
```

For more examples visit either [documentation](http://docs.viuavm.org/) or [Rosetta](http://rosettacode.org/wiki/Viua_VM_assembly) page for Viua VM.
Check [Weekly](http://weekly.viuavm.org/) blog for news and developments in Viua VM.


### Use cases

Viua is a runtime environment focused on reliability, predictability, and concurrency.
It is suitable for writing long-running software that runs in the background providing infrastructure services; servers, message queues, and
system and application daemons.
The VM is able to fully utilise all cores of the CPU it's running on (tested on a system with 8 hardware threads) so can
generate high CPU loads, but is relatively light on RAM and should not contain any memory leaks (all runtime tests are
run under Valgrind to ensure this).

The virtual machine is covered by more than 400 tests to provide safety, and guard against possible regressions.
It ships with an assembler and a static analyser, but does not provide any higher-level language compiler.


----


#### Design goals

- **predictable execution**: it is easier to reason about code when you know exactly how it will behave
- **predictable value lifetimes**: in Viua you do not have to guess when the memory will be released, when objects will be destroyed, or
  remember about the possibility of a gargabe collector kicking in and interrupting your program;
  Viua manages resources without a GC in a strictly scope-based (where "scope" means "virtual stack frame") manner
- **massive concurrency**: Viua architecture supports spawning massive amounts of independent, VM-based lightweight processes that can
  run in parallel (Viua is capable of providing true parallelism given sufficient hardware resources, i.e. at least two CPU cores)
- **parallel I/O and FFI schedulers**: I/O operations and FFI calls cannot block the VM as they are executed on dedicated schedulers, so block only the
  virtual process that called them without affecting other virtual processes
- **easy scatter/gather processing**: processes communicate using messages but machine supports a *join* instruction - which synchronises virtual
  processes execution, it blocks calling process until called process finishes and receives return value of called process
- **safe inter-process communication** via message-passing (with queueing)
- **soft-realtime capabilities**: join and receive operations with timeouts (throwing exceptions if nothing has been received) make Viua
  a VM suitable to host soft-realtime programs
- **fast debugging**: error handling is performed with exceptions (even across virtual processes), and unserviced exceptions cause the machine
  to generate precise and detailed stack traces; running programs are also debuggable with GDB
- **reliability**: programs running on Viua should be able to run until they are told to stop, not until they crash;
  machine helps writing reliable programs by providing a framework for detailed exception-based error communication and
  a way to register a per-process watchdog that handles processes killed by runaway exceptions (the exception is serviced by watchdog instead of
  killing the whole VM)


Some features also supported by the VM:

- separate compilation of Viua code modules
- static and dynamic linking of Viua-native libraries
- built-in, simple algorithm supporting multiple inheritance of user-defined types in Viua
- straightforward ways to use both dynamic and static method dispatch on objects
- first-class functions
- closures (with multiple way of capturing objects inside a closure)
- passing function parameters by value and move (non-copying pass)
- copy-free function returns
- inter-function tail calls
- support for pointers (with no arithmetic, and no reassignment - pointers may be only used for reading and mutating objects)

For enhanced reliability, Viua assembler provides a built-in static analyser that is able to detect most common errors related to
register manipulation at compile time.
It provides traced errors whenever possible, i.e. when SA detects an error and is able to trace execution path that would trigger it,
a sequence of instructions (with source code locations) leading to the detected error is presented to the user instead of a single offending
instruction.


Current limitations include:

- severly limited introspection,
- no way to express atoms (i.e. all names must be known at compile time, and there is no way to tell the machine "Here, take this string,
  convert it to atom and return corresponding function/class/etc.")
- calling Viua code from C++ is not tested
- debugging information encoded in compiled files is limited
- speed: Viua is not the fastest VM around
- VM cannot distinguish FFI calls that are CPU or I/O bound (**WIP**), so if the program performs many I/O operations it may saturate all
  FFI schedulers (which will effectively prevent the program from quickly executing more FFI calls; virtual processes are still running, and
  Viua functions are still freely callable, though)


##### Software state notice

Viua VM is an alpha-stage software.
Even though great care is taken not to introduce bugs during development, it is inevitable that some will make their way into the codebase.
Viua in its current state *is not* production ready; bytecode definition and format will be altered, opcodes may be removed and
added without deprecation warning, and various external and internal APIs may change without prior notice.
Suitable announcements will be made when the VM reaches beta, RC and release stages.


#### Influences

The way Viua works has mostly been influenced by
Python (objects as dictionaries, the way multiple inheritance works),
C++ (static and dynamic method dispatch), and
Erlang (message passing, indepenedent VM-based lightweight processes as units of concurrency).
Maybe also a little bit of Java (more like JVM actually) and Ruby.
Some parts of Viua assembly syntax may remind some people of Lisp.


----


## Programming in Viua

Viua can be programmed in an assembly-like language which must be compiled into bytecode.
A typical session is shown below (assuming current working directory is the local clone of Viua repository):

```
 ]$ vi some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.out some_file.asm
# static analysis or syntax errors...
 ]$ vi some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.out some_file.asm
 ]$ ./build/bin/vm/kernel some_file.out
# runtime exceptions...
 ]$ vi some_file.asm
 ]$ ./build/bin/vm/asm -o some_file.out some_file.asm
 ]$ ./build/bin/vm/kernel some_file.out
```

----

## Contributing

Please read [CONTRIBUTING.markdown](./CONTRIBUTING.markdown) for details on development setup, and
the process for submitting pull requests, bug reports, and feature suggestions.


----

## License

The code is licensed under GNU GPL v3.


----

### Contact information

Project website: [viuavm.org](http://viuavm.org/)

Maintainer: &lt;marekjm at ozro dot pw&gt;

Bugtracker: https://github.com/marekjm/viuavm/issues
