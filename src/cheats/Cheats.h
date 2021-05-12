#pragma once

extern "C" {
    #include <lua.h>
}

namespace hc {
    namespace cheats {
        int push(lua_State* const L);
        void onFrame();
    }
}
