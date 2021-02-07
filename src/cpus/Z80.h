#pragma once

#include "Cpu.h"
#include "Memory.h"

namespace hc {
    namespace z80 {
        enum Cycles : uint8_t {
            CyclesDjnz = 64,         // 13 when it jumps, 8 when it doesn't
            CyclesCondJr = 65,       // 12 when it jumps, 7 when it doesn't
            CyclesCondRet = 66,      // 11 when it returns, 5 when it doesn't
            CyclesCondCall = 67,     // 17 when it jumps, 10 when it doesn't
            CyclesBlockTransfer = 68 // 21 when it repeats, 16 when it doesn't
        };

        void info(uint64_t address, Memory const* memory, uint8_t* length, uint8_t* cycles, FlagState flags[8]);
        void disasm(uint64_t address, Memory const* memory, char* buffer, size_t size);
    }
}
