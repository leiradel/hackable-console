#pragma once

#include <stdint.h>
#include <stddef.h>

namespace hc {
    /*enum class FlagState : uint8_t {
        HC_FLAG_UNCHANGED,
        HC_FLAG_SET,
        HC_FLAG_RESET,
        HC_FLAG_CHANGED
    };*/

    typedef uint8_t (*Peek)(uint64_t address, void* userdata);

    unsigned disasm_z80(uint64_t const address, Peek const peek, void* const userdata, char* const buffer, size_t size);
}
