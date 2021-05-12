#pragma once

extern "C" {
    #include <lua.h>
}

#include <stdint.h>

namespace hc {
    class Memory;
    class Snapshot;
    class Set;

    namespace filter {
        // The endianess to use during a filter operation.
        enum class Endianess {
            Little,
            Big
        };

        // The operator to use during a filter operation.
        enum class Operator {
            LessThan,
            LessEqual,
            GreaterThan,
            GreaterEqual,
            Equal,
            NotEqual
        };

        Set* fsigned(Memory const& memory, int64_t value, Operator op, Endianess endianess, size_t valueSize);
        Set* fsigned(Memory const& memory1, Memory const& memory2, Operator op, Endianess endianess, size_t valueSize);

        Set* funsigned(Memory const& memory, uint64_t value, Operator op, Endianess endianess, size_t valueSize);
        Set* funsigned(Memory const& memory1, Memory const& memory2, Operator op, Endianess endianess, size_t valueSize);

        int l_Filter(lua_State* L);
    }
}
