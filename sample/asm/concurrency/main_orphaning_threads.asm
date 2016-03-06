.function: print_lazy
    ; many nops to make the process run for a long time long
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    print (arg 1 0)
    return
.end

.function: print_eager
    print (arg 1 0)
    return
.end

.function: main
    frame ^[(param 0 (strstore 1 "Hello multithreaded World! (1)"))]
    process 3 print_lazy

    frame ^[(param 0 (strstore 2 "Hello multithreaded World! (2)"))]
    process 4 print_eager

    thjoin 0 4
    ; do not join the process to test __entry/0 orphaning detection
    ;thjoin 0 3

    izero 0
    return
.end
