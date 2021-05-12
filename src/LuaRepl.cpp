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
    , _term(
        [this](ImGuiAl::Terminal& self, char* command) { Execute(command); },
        [this](ImGuiAl::Terminal& self, ImGuiInputTextCallbackData* data) { Callback(data); }
    )
    , _execute(LUA_NOREF)
    , _history(LUA_NOREF)
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

    lua_pushlightuserdata(_L, this);
    lua_pushcclosure(_L, l_show, 1);

    lua_pushlightuserdata(_L, this);
    lua_pushcclosure(_L, l_green, 1);

    lua_pushlightuserdata(_L, this);
    lua_pushcclosure(_L, l_yellow, 1);

    lua_pushlightuserdata(_L, this);
    lua_pushcclosure(_L, l_red, 1);

    lua_call(_L, 4, 2);
    _history = luaL_ref(_L, LUA_REGISTRYINDEX);
    _execute = luaL_ref(_L, LUA_REGISTRYINDEX);
    return true;
}

char const* hc::LuaRepl::getTitle() {
    return ICON_FA_TERMINAL " Lua Repl";
}

void hc::LuaRepl::onDraw() {
    _term.draw();
}

void hc::LuaRepl::Execute(char* const command) {
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _execute);
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

void hc::LuaRepl::Callback(ImGuiInputTextCallbackData* data) {
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _history);
    lua_pushboolean(_L, data->EventKey == ImGuiKey_UpArrow);

    protectedCall(_L, 1, 1, _logger);

    if (lua_isstring(_L, -1)) {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, lua_tostring(_L, -1));
    }

    lua_pop(_L, 1);
}

int hc::LuaRepl::l_show(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.printf("%s", luaL_checkstring(L, 1));
    self->_term.scrollToBottom();
    return 0;
}

int hc::LuaRepl::l_green(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.setForegroundColor(ImGuiAl::Crt::CGA::BrightGreen);
    return 0;
}

int hc::LuaRepl::l_yellow(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.setForegroundColor(ImGuiAl::Crt::CGA::Yellow);
    return 0;
}

int hc::LuaRepl::l_red(lua_State* L) {
    auto const self = static_cast<LuaRepl*>(lua_touserdata(L, lua_upvalueindex(1)));
    self->_term.setForegroundColor(ImGuiAl::Crt::CGA::BrightRed);
    return 0;
}
