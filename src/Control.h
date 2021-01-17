#pragma once

#include "Desktop.h"
#include "Logger.h"
#include "LifeCycle.h"

#include <string>
#include <vector>

namespace hc {
    class Control : public View {
    public:
        Control();

        void init(Logger* const logger, LifeCycle* const fsm);

        void setSystemInfo(retro_system_info const* info);

        static Control* check(lua_State* const L, int const index);

        // hc::View
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
        virtual void onDraw(bool* opened) override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        virtual int push(lua_State* const L) override;

    protected:
        // Control will also be responsible for exposing LifeCycle and Frontend
        // methods to Lua
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

        static int l_apiVersion(lua_State* const L);
        static int l_getSystemInfo(lua_State* const L);
        static int l_getSystemAvInfo(lua_State* const L);
        static int l_serializeSize(lua_State* const L);
        static int l_serialize(lua_State* const L);
        static int l_unserialize(lua_State* const L);
        static int l_cheatReset(lua_State* const L);
        static int l_cheatSet(lua_State* const L);
        static int l_getRegion(lua_State* const L);
        static int l_getMemoryData(lua_State* const L);
        static int l_getMemorySize(lua_State* const L);

        struct Console {
            std::string name;
            lua_State* L;
            int ref;
        };

        LifeCycle* _fsm;
        Logger* _logger;

        std::vector<Console> _consoles;
        int _selected;
        int _opened;
        std::string _extensions;
        std::string _lastGameFolder;
    };
}
