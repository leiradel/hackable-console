#pragma once

#include "Desktop.h"

#include <lrcpp/Components.h>
#include <imguial_term.h>

#include <mutex>

extern "C" {
    #include <lua.h>
}

namespace hc {
    class Logger: public View, public lrcpp::Logger {
    public:
        Logger(Desktop* desktop);
        virtual ~Logger() {}

        bool init();

        static Logger* check(lua_State* const L, int const index);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;
        virtual void onConsoleUnloaded() override;

        // hc::Scriptable
        virtual int push(lua_State* const L) override;

        // lrcpp::Logger
        virtual void vprintf(retro_log_level level, char const* format, va_list args) override;

    protected:
        static int l_debug(lua_State* const L);
        static int l_info(lua_State* const L);
        static int l_warn(lua_State* const L);
        static int l_error(lua_State* const L);

        ImGuiAl::BufferedLog<1024 * 1024> _logger;
        std::mutex _mutex;
    };
}
