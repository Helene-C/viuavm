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

.signature: Pointer::expired/1

.function: isExpired/1
    frame ^[(param %0 (arg %1 %0))]
    call %2 Pointer::expired/1
    echo (string %3 "expired: ")
    move %0 (print %2)
    return
.end

.function: main/1
    integer %1 42
    ptr %2 %1

    frame ^[(param %0 %2)]
    call void isExpired/1

    delete %1

    frame ^[(param %0 %2)]
    call void isExpired/1

    izero %0 local
    return
.end
