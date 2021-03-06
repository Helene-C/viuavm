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

; This file test support for 'add' instruction and
; support for negative numbers in float.
; The nagative-numbers thingy is just nice-to-have and not the
; true porpose of this script, though.

.function: main/1
    float %1 4.0
    float %2 -3.5
    add %3 %1 %2
    float %4 0
    add %3 %3 %4
    print %3
    izero %0 local
    return
.end
