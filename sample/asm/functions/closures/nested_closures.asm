;
;   Copyright (C) 2015, 2016, 2017 Marek Marecki
;
;   This file is part of Viua VM.
;
;   Viua VM is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   Viua VM is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

; this example is the Viua assembly version of the following JS:
;
;   function closure_maker(x) {
;       function closure_level_1(a) {
;           function closure_level_2(b) {
;               function closure_level_3(c) {
;                   return (x + a + b + c);
;               }
;               return closure_level_3;
;           }
;           return closure_level_2;
;       }
;       return closure_level_1;
;   }
;
;   closure_maker(1)(2)(3)(4) == 10
;

.closure: closure_level_3/1
    ; expects 1, 2 and 3 to be captured integers
    .name: 5 accumulator
    move %0 (add %accumulator (arg %4 %0) (add %accumulator %3 (add %accumulator %1 %2)))
    return
.end

.closure: closure_level_2/1
    closure %0 closure_level_3/1
    ; registers 1 and 2 are occupied by captured integers
    ; but they must be captured by the "closure_level_3"
    capture %0 %1 %1
    capture %0 %2 %2
    capture %0 %3 (arg %3 %0)
    return
.end

.closure: closure_level_1/1
    closure %0 closure_level_2/1
    ; register 1 is occupied by captured integer
    ; but it must be captured by the "closure_level_2"
    capture %0 %1 %1
    capture %0 %2 (arg %2 %0)
    return
.end

.function: closure_maker/1
    ; create the outermost closure
    closure %0 closure_level_1/1
    capture %0 %1 (arg %1 %0)
    return
.end

.function: main/1
    frame ^[(param %0 (istore %1 1))]
    call %2 closure_maker/1

    frame ^[(param %0 (istore %1 2))]
    call %3 %2

    frame ^[(param %0 (istore %1 3))]
    call %4 %3

    frame ^[(param %0 (istore %1 4))]
    print (call %5 %4)

    izero %0 local
    return
.end
