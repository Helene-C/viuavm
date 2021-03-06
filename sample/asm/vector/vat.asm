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

.function: main/1
    vector (.name: %iota a_vector)
    print (vlen (.name: %iota length) %a_vector)

    vpush %a_vector (string %iota "Hello World!")
    print (vlen %length %a_vector)

    vat (.name: %iota something) %a_vector (integer %iota -1)
    print (vlen %length %a_vector)

    print *something

    izero %0 local
    return
.end
