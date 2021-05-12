#pragma once

#include "Desktop.h"

#include <imguial_term.h>

extern "C" {
    #include <lua.h>
}

namespace hc {
    class LuaRepl : public View {
    public:
        LuaRepl(Desktop* desktop);
        virtual ~LuaRepl() {}

        bool init(lua_State* L, Logger* logger);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;

    protected:
        enum {
            CommandSize = 1024
        };

        typedef ImGuiAl::BufferedTerminal<1024 * 1024, CommandSize> Terminal;

        void Execute(char* const command);

        static int l_print(lua_State* L);
        static int l_green(lua_State* L);
        static int l_yellow(lua_State* L);
        static int l_red(lua_State* L);

        lua_State* _L;
        Logger* _logger;
        Terminal _term;
        int _repl;
    };
}
