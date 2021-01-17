#pragma once

#include "Desktop.h"

#include <Components.h>
#include <imguial_term.h>

extern "C" {
    #include <lua.h>
}

#include <SDL.h>

namespace hc {
    class Logger: public View, public lrcpp::Logger {
    public:
        Logger();
        virtual ~Logger() {}

        bool init();

        static Logger* check(lua_State* const L, int const index);

        // hc::View
        virtual Type getType() override { return Type::Logger; }
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
        virtual void onDraw(bool* opened) override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        virtual int push(lua_State* const L) override;

        // lrcpp::Logger
        virtual void vprintf(retro_log_level level, char const* format, va_list args) override;

    protected:
        static int l_debug(lua_State* const L);
        static int l_info(lua_State* const L);
        static int l_warn(lua_State* const L);
        static int l_error(lua_State* const L);

        ImGuiAl::BufferedLog<1024 * 1024> _logger;
        SDL_mutex* _mutex;
    };
}
