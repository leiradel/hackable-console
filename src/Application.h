#pragma once

#include "LifeCycle.h"
#include "Plugin.h"

#include "Logger.h"
#include "Config.h"
#include "Audio.h"
#include "Video.h"
#include "Led.h"
#include "Input.h"
#include "Perf.h"

#include "Control.h"
#include "Memory.h"
#include "Fifo.h"

#include "LuaBind.h"

#include <SDL.h>
#include <SDL_opengl.h>

extern "C" {
    #include <lua.h>
}

#include <stdarg.h>

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace hc {
    class Application {
    public:
        Application();

        bool init(std::string const& title, int const width, int const height);
        void destroy();
        void draw();
        void run();

        bool loadConsole(char const* name);
        bool loadGame(char const* path);
        bool pauseGame();
        bool quit();
        bool resetGame();
        bool resumeGame();
        bool step();
        bool unloadConsole();
        bool unloadGame();

        void onStarted();
        void onConsoleLoaded();
        void onGameLoaded();
        void onGamePaused();
        void onGameResumed();
        void onGameReset();
        void onFrame();
        void onGameUnloaded();
        void onConsoleUnloaded();
        void onQuit();

        int luaopen_hc(lua_State* const L);

    public:
        static void vprintf(void* ud, char const* fmt, va_list args);

        static void audioCallback(void* const udata, Uint8* const stream, int const len);

        static int l_addConsole(lua_State* const L);
        static int l_addMemoryRegion(lua_State* const L);

        SDL_Window* _window;
        SDL_GLContext _glContext;
        SDL_AudioSpec _audioSpec;
        SDL_AudioDeviceID _audioDev;

        LifeCycle _fsm;

        std::unordered_set<Plugin*> _plugins;

        Audio* _audio;
        Config* _config;
        Led* _led;
        Logger* _logger;
        Video* _video;
        Input* _input;
        Perf* _perf;

        Control* _control;
        Memory _memory;

        Fifo _fifo;
        std::unordered_map<std::string, int> _consoleRefs;
        std::string _currentConsole;
        lua_State* _L;
    };
}
