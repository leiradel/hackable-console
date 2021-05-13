#pragma once

#include "Desktop.h"
#include "Scriptable.h"

#include <imguial_term.h>

extern "C" {
    #include <lua.h>
}

namespace hc {
    class LuaRepl : public View, public Scriptable {
    public:
        LuaRepl(Desktop* desktop, Logger* logger);
        virtual ~LuaRepl() {}

        bool init();

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;

        // hc::Scriptable
        virtual int push(lua_State* L) override;

    protected:
        enum {
            CommandSize = 1024
        };

        typedef ImGuiAl::BufferedTerminal<1024 * 1024, CommandSize> Terminal;

        void execute(char* const command);
        void callback(ImGuiInputTextCallbackData* data);

        static int l_show(lua_State* L);
        static int l_green(lua_State* L);
        static int l_yellow(lua_State* L);
        static int l_red(lua_State* L);

        Logger* _logger;
        lua_State* _L;
        Terminal _term;
        int _execute;
        int _history;
    };
}
