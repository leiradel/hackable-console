#pragma once

#include "Plugin.h"
#include "Logger.h"
#include "LifeCycle.h"

#include <string>
#include <map>

namespace hc {
    class Control : public Plugin {
    public:
        Control();

        void init(Logger* const logger, LifeCycle* const fsm);

        void setSystemInfo(retro_system_info const* info);

        static Control* check(lua_State* const L, int const index);

        // hc::Plugin
        virtual Type getType() override { return Type::Control; }
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

        virtual int push(lua_State* const L) override;

    protected:
        // Control will also be responsible for exposing LifeCycle methods to Lua
        static int l_addConsole(lua_State* const L);
        static int l_loadCore(lua_State* const L);
        static int l_quit(lua_State* const L);
        static int l_unloadCore(lua_State* const L);
        static int l_loadGame(lua_State* const L);
        static int l_resumeGame(lua_State* const L);
        static int l_resetGame(lua_State* const L);
        static int l_step(lua_State* const L);
        static int l_unloadGame(lua_State* const L);
        static int l_pauseGame(lua_State* const L);

        struct Callback {
            lua_State* const L;
            int const ref;
        };

        LifeCycle* _fsm;
        Logger* _logger;

        std::map<std::string, Callback> _consoles;
        int _selected;
        std::string _extensions;
        std::string _lastGameFolder;
    };
}
