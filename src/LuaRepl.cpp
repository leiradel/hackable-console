#include "LuaRepl.h"
#include "LuaUtil.h"

#include <IconsFontAwesome4.h>

extern "C" {
    #include <lauxlib.h>
}

static char const* const repl = R"lua(
return function(term)
    local term_print = term.print
    local string_format = string.format
    local table_concat = table.concat
    local table_remove = table.remove
    local global_load = load
    local global_xpcall = xpcall
    local debug_traceback = debug.traceback
    local global_tostring = tostring
    local global_tonumber = tonumber

    -- Patch term.print to apply tostring to all arguments
    term.print = function(self, ...)
        local args = {...}

        for i = 1, #args do
            args[i] = global_tostring(args[i])
        end

        term_print(self, table_concat(args, ''))
        return self
    end

    -- Change the global print to use term.print
    print = function(...)
        local args = {...}

        for i = 1, #args do
            args[i] = global_tostring(args[i])
        end

        term:green():print(table_concat(args, '\t'))
    end

    -- Return a function that receives and runs commands typed in Lua
    local buffer = ''
    local history = {}
    local limit = 100

    return function(line)
        if line == 'history' then
            for i = 1, #history do
                term:print(string_format('%3d %s', i, history[i]))
            end

            return
        elseif line:sub(1, 1) == '!' then
            local i = #line == 1 and #history or global_tonumber(line:sub(2, -1))

            if i >= 1 and i <= #history then
                local line = history[i]
                table_remove(history, i)
                return line
            end

            return
        end

        buffer = buffer .. (#buffer == 0 and '' or '; ') .. line
        local chunk, err = global_load('return ' .. buffer .. ';', '=stdin', 't')

        if chunk then
            local res = {global_xpcall(chunk, debug_traceback)}

            if res[1] then
                table_remove(res, 1)
                term:green():print('> ', line):print(table.unpack(res))
                buffer = ''
                history[#history + 1] = line

                while #history > limit do
                    table_remove(history, 1)
                end

                return
            end
        end

        local chunk, err = global_load(buffer, '=stdin', 't')

        if not chunk then
            if err:sub(-5, -1) ~= '<eof>' then
                term:red():print(err)
                buffer = ''
                return
            end

            term:yellow():print('>> ', line)
            return
        end

        term:green():print('> ', line)
        history[#history + 1] = buffer

        while #history > limit do
            table_remove(history, 1)
        end

        buffer = ''
        local ok, err = global_xpcall(chunk, debug_traceback)

        if not ok then
            term:red():print(err)
        end
    end
end
)lua";

hc::LuaRepl::LuaRepl(Desktop* desktop)
    : View(desktop)
    , _L(nullptr)
    , _term([this](ImGuiAl::Terminal& self, char* const command) { Execute(command); })
    , _repl(LUA_NOREF)
{}

bool hc::LuaRepl::init(lua_State* L, Logger* logger) {
    _L = L;
    _logger = logger;

    const int res = luaL_loadbufferx(_L, repl, strlen(repl), "repl.lua", "t");

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
