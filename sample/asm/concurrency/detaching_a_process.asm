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

.function: running_detached/0
    izero (.name: %iota counter) local
    integer (.name: %iota limit) local 4
    string (.name: %iota report_text_format) local "Hello World! (from long-running detached process) #{0}"

    .mark: loop
    .name: %iota format_parameters
    .name: %iota n
    .name: %iota condition
    gte %condition local %counter local %limit local
    if %condition local after_loop

    vector %format_parameters local (copy %n local %counter local) local %1

    frame ^[(param %iota %report_text_format local) (param %iota %format_parameters local)]
    print (msg %iota local format/) local

    iinc %counter local

    jump loop
    .mark: after_loop

    return
.end

.function: main/1
    frame %0
    process void running_detached/0

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    print (string %3 "main/1 exited")

    izero %0 local
    return
.end
