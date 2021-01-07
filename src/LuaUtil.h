#pragma once

#include "Logger.h"

extern "C" {
    #include "lua.h"
}

namespace hc {
    int GetField(lua_State* const L, int const tableIndex, char const* const fieldName, Logger* const logger = nullptr);
    bool ProtectedCall(lua_State* const L, int const nargs, int const nresults, Logger* const logger = nullptr);
}
