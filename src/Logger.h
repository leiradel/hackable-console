#pragma once

#include "Plugin.h"

#include <Components.h>
#include <imguial_term.h>

extern "C" {
    #include <lua.h>
}

#include <SDL.h>

namespace hc {
    class Logger: public Plugin, public lrcpp::Logger {
    public:
        Logger();
        virtual ~Logger() {}

        bool init();

        int push(lua_State* L);
        static Logger* check(lua_State* L, int index);

        // hc::Plugin
        virtual char const* getName() override;
        virtual char const* getVersion() override;
        virtual char const* getLicense() override;
        virtual char const* getCopyright() override;
        virtual char const* getUrl() override;

        virtual void onStarted() override;
        virtual void onConsoleLoaded() override;
        virtual void onGameLoaded() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onGameReset() override;
        virtual void onFrame() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        // lrcpp::Logger
        virtual void vprintf(retro_log_level level, char const* format, va_list args) override;

    protected:
        static int l_debug(lua_State* L);
        static int l_info(lua_State* L);
        static int l_warn(lua_State* L);
        static int l_error(lua_State* L);

        ImGuiAl::BufferedLog<1024 * 1024> _logger;
        SDL_mutex* _mutex;
    };
}
