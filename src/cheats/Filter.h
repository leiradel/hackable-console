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
        Set* fsigned(Memory const& memory, Snapshot const& snapshot, Operator op, Endianess endianess, size_t valueSize);
        Set* fsigned(Snapshot const& snapshot, int64_t value, Operator op, Endianess endianess, size_t valueSize);
        Set* fsigned(Snapshot const& snapshot, Memory const& memory, Operator op, Endianess endianess, size_t valueSize);
        Set* fsigned(Snapshot const& snapshot1, Snapshot const& snapshot2, Operator op, Endianess endianess, size_t valueSize);

        Set* funsigned(Memory const& memory, uint64_t value, Operator op, Endianess endianess, size_t valueSize);
        Set* funsigned(Memory const& memory, Snapshot const& snapshot, Operator op, Endianess endianess, size_t valueSize);
        Set* funsigned(Snapshot const& snapshot, uint64_t value, Operator op, Endianess endianess, size_t valueSize);
        Set* funsigned(Snapshot const& snapshot, Memory const& memory, Operator op, Endianess endianess, size_t valueSize);
        Set* funsigned(Snapshot const& snapshot1, Snapshot const& snapshot2, Operator op, Endianess endianess, size_t valueSize);

        int l_Filter(lua_State* L);
    }
}
