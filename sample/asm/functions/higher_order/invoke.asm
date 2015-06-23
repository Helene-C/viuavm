.def: sum4 1
    ; this function takes four integers as parameters and
    ; adds them, and returns the sum
    arg 0 1
    arg 1 2
    arg 2 3
    arg 3 4

    iadd 1 2 0
    iadd 0 3 0
    iadd 0 4 0

    end
.end

.def: invoke 1
    ; this function takes two parameters:
    ;    1) a function object
    ;    2) a vector of parameters for function given as first parameter
    ;
    ; it then creates a frame with required number of parameter slots (as
    ; specified by length of the vector), and calls given function with this
    ; frame
    arg 0 1
    arg 1 2

    ; take length of the vector
    .name: 4 vector_length
    vlen 2 vector_length
    frame @vector_length

    ; zero loop counter
    .name: 3 loop_counter
    izero loop_counter
    .mark: while_begin

    ; simple condition:
    ; while (loop_counter < vector_length) {
    .name: 5 loop_condition
    igte loop_counter vector_length loop_condition
    branch loop_condition while_end while_body

    .mark: while_body

    ; store item located inside parameter vector at index denoted by loop_counter in
    ; a register
    .name: 7 slot
    vat 2 slot @loop_counter

    ; add parameter
    param @loop_counter slot

    ; clear parameter slot
    empty slot

    ; loop_counter++
    iinc loop_counter

    jump while_begin

    .mark: while_end

    ; finally, after the frame is ready
    ; call the function
    fcall 1 8
    move 8 0
    end
.end

.def: main 1
    ; create the vector
    vec 1

    istore 2 20
    vpush 1 2

    istore 3 16
    vpush 1 3

    istore 4 8
    vpush 1 4

    istore 5 -2
    vpush 1 5

    ; call sum/4() function
    frame 4
    param 0 2
    param 1 3
    param 2 4
    param 3 5
    call sum4 6
    print 6

    function sum4 7

    istore 9 2
    frame @9

    izero 9
    param @9 7
    iinc 9
    param @9 1

    call invoke 8

    print 8

    izero 0
    end
.end
