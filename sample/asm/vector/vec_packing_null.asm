.function: main/0
    string %2 "answer to life"
    istore %3 42

    -- this would pack
    -- the vector inside itself
    vec %1 %2 %8

    print %1

    izero %0 local
    return
.end
