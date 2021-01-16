#pragma once

#include "Logger.h"

#include <Frontend.h>

extern "C" {
    #include "lua.h"
}

namespace hc {
    class LuaRewinder {
    public:
        LuaRewinder(lua_State* const L) : _L(L), _top(lua_gettop(L)) {}
        ~LuaRewinder() { lua_settop(_L, _top); }

    protected:
        lua_State* const _L;
        int const _top;
    };

    void registerSearcher(lua_State* const L);
    int getField(lua_State* const L, int const tableIndex, char const* const fieldName, Logger* const logger = nullptr);
    bool protectedCall(lua_State* const L, int const nargs, int const nresults, Logger* const logger = nullptr);

    bool protectedCallField(
        lua_State* const L,
        int const tableIndex,
        char const* const fieldName,
        int const nargs,
        int const nresults,
        Logger* const logger = nullptr
    );
}
