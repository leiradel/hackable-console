#include "LuaRepl.h"
#include "LuaUtil.h"

#include "LuaRepl.lua.h"

#include <IconsFontAwesome4.h>

extern "C" {
    #include <lauxlib.h>
}

hc::LuaRepl::LuaRepl(Desktop* desktop, Logger* logger)
    : View(desktop)
    , _logger(logger)
    , _term(
        [this](ImGuiAl::Terminal& self, char* command) { execute(command); },
        [this](ImGuiAl::Terminal& self, ImGuiInputTextCallbackData* data) { callback(data); }
    )
    , _execute(LUA_NOREF)
    , _history(LUA_NOREF)
{}

bool hc::LuaRepl::init() {
    return true;
}

char const* hc::LuaRepl::getTitle() {
    return ICON_FA_TERMINAL " Lua Repl";
}

void hc::LuaRepl::onDraw() {
    _term.draw();
}

int hc::LuaRepl::push(lua_State* L) {
    auto const self = static_cast<LuaRepl**>(lua_newuserdata(L, sizeof(LuaRepl*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::LuaRepl")) {
        static luaL_Reg const methods[] = {
            {"show", l_show},
            {"green", l_green},
            {"yellow", l_yellow},
            {"red", l_red},
            {nullptr, nullptr}
        };

        luaL_newlibtable(L, methods);
        lua_pushlightuserdata(L, this);
        luaL_setfuncs(L, methods, 1);

        int const res = luaL_loadbufferx(L, LuaRepl_lua, sizeof(LuaRepl_lua), "LuaRepl.lua", "t");

        if (res != LUA_OK) {
            _desktop->error("%s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        else {
            protectedCall(L, 0, 1, _logger);
            lua_pushvalue(L, -2);
            protectedCall(L, 1, 2, _logger);

            _history = luaL_ref(L, LUA_REGISTRYINDEX);
            _execute = luaL_ref(L, LUA_REGISTRYINDEX);
            _L = L;
        }

        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

void hc::LuaRepl::execute(char* const command) {
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

void hc::LuaRepl::callback(ImGuiInputTextCallbackData* data) {
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
