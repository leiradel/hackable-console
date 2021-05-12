#include "LuaRepl.h"
#include "LuaUtil.h"

#include "LuaRepl.lua.h"

#include <IconsFontAwesome4.h>

extern "C" {
    #include <lauxlib.h>
}

hc::LuaRepl::LuaRepl(Desktop* desktop)
    : View(desktop)
    , _L(nullptr)
    , _term([this](ImGuiAl::Terminal& self, char* const command) { Execute(command); })
    , _repl(LUA_NOREF)
{}

bool hc::LuaRepl::init(lua_State* L, Logger* logger) {
    _L = L;
    _logger = logger;

    int const res = luaL_loadbufferx(_L, LuaRepl_lua, sizeof(LuaRepl_lua), "LuaRepl.lua", "t");

    if (res != LUA_OK) {
        _desktop->error("%s", lua_tostring(_L, -1));
        lua_pop(_L, 1);
        return false;
    }

    lua_call(_L, 0, 1);

    static luaL_Reg const statics[] = {
        {"print", l_print},
        {"green", l_green},
        {"yellow", l_yellow},
        {"red", l_red},
        {nullptr, nullptr}
    };

    luaL_newlibtable(_L, statics);
    lua_pushlightuserdata(_L, this);
    luaL_setfuncs(_L, statics, 1);

    lua_call(_L, 1, 1);
    _repl = luaL_ref(_L, LUA_REGISTRYINDEX);
    return true;
}

char const* hc::LuaRepl::getTitle() {
    return ICON_FA_TERMINAL " Lua Repl";
}

void hc::LuaRepl::onDraw() {
    _term.draw();
}

void hc::LuaRepl::Execute(char* const command) {
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _repl);
    lua_pushstring(_L, command);

    protectedCall(_L, 1, 1, _logger);

    if (lua_isstring(_L, -1)) {
        strncpy(command, lua_tostring(_L, -1), CommandSize);
        command[CommandSize - 1] = 0;
    }
    else {
        command[0] = 0;
    }

    lua_pop(_L, 1);
}

int hc::LuaRepl::l_print(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.printf("%s", luaL_checkstring(L, 2));
    self->_term.scrollToBottom();
    lua_pushvalue(L, 1);
    return 1;
}

int hc::LuaRepl::l_green(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.setForegroundColor(ImGuiAl::Crt::CGA::BrightGreen);
    lua_pushvalue(L, 1);
    return 1;
}

int hc::LuaRepl::l_yellow(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.setForegroundColor(ImGuiAl::Crt::CGA::Yellow);
    lua_pushvalue(L, 1);
    return 1;
}

int hc::LuaRepl::l_red(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.setForegroundColor(ImGuiAl::Crt::CGA::BrightRed);
    lua_pushvalue(L, 1);
    return 1;
}
