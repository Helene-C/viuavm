#ifndef VIUA_CPU_TRYFRAME_H
#define VIUA_CPU_TRYFRAME_H

#pragma once

#include <string>
#include <map>
#include "../bytecode/bytetypedef.h"
#include "frame.h"
#include "catcher.h"

class TryFrame {
    public:
        byte* return_address;
        Frame* associated_frame;

        std::string block_name;

        std::map<std::string, Catcher*> catchers;

        inline byte* ret_address() { return return_address; }

        TryFrame(byte* ra, Frame* af): return_address(ra), associated_frame(af) {}
};


#endif
