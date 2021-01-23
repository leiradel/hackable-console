#pragma once

extern "C" {
    #include <lua.h>
}

namespace hc {
    class Scriptable {
    public:
        virtual ~Scriptable() {}
        virtual int push(lua_State* const L) = 0;
    };
}
