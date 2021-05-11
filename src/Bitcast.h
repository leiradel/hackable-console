#pragma once

namespace hc {
    template<typename T, typename U>
    static T bitcast(const U u) {
        static_assert(sizeof(T) == sizeof(U), "cannot bitcast from a different size");
        union { T t; U u; } x;
        x.u = u;
        return x.t;
    }
}
