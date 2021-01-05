#pragma once

#include <Components.h>
#include <imguial_term.h>

extern "C" {
    #include <lua.h>
}

#include <SDL.h>

namespace hc {
    class Logger: public lrcpp::Logger {
    public:
        Logger() {}
        virtual ~Logger() {}

        bool init();
        void destroy();
        void reset();
        void draw();

        virtual void vprintf(retro_log_level level, char const* format, va_list args) override;

        int push(lua_State* L);
        static Logger* check(lua_State* L, int index);

    protected:
        static int l_debug(lua_State* L);
        static int l_info(lua_State* L);
        static int l_warn(lua_State* L);
        static int l_error(lua_State* L);

        ImGuiAl::BufferedLog<1024 * 1024> _logger;
        SDL_mutex* _mutex;
    };
}
