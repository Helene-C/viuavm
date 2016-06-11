#!/usr/bin/env bash

if [[ $(g++ --version | head -n 1 | grep -Po 'g\+\+ \(GCC\) 6' | cat) == 'g++ (GCC) 6' ]]; then
    CXX_STANDARD='c++14'
else
    CXX_STANDARD='c++11'
fi

LINES=$(wc -l Makefile | cut -d' ' -f1)
LINES=$(dc -e "$LINES 1 - p")

sed -i "s:CXX_STANDARD=.*:CXX_STANDARD=$CXX_STANDARD:g" Makefile
