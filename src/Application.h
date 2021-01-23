#pragma once

#include "LifeCycle.h"
#include "Desktop.h"

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

#include <SDL.h>
#include <SDL_opengl.h>

extern "C" {
    #include <lua.h>
}

#include <stdarg.h>

namespace hc {
    class Application {
    public:
        Application();

        bool init(std::string const& title, int const width, int const height);
        void destroy();
        void draw();
        void run();

        bool loadCore(char const* path);
        bool loadGame(char const* path);
        bool pauseGame();
        bool quit();
        bool resetGame();
        bool resumeGame();
        bool startGame();
        bool step();
        bool unloadCore();
        bool unloadGame();

        void onStarted();
        void onCoreLoaded();
        void onGameLoaded();
        void onGameStarted();
        void onGamePaused();
        void onGameResumed();
        void onGameReset();
        void onFrame();
        void onStep();
        void onGameUnloaded();
        void onConsoleUnloaded();
        void onQuit();

    protected:
        static void vprintf(void* ud, char const* fmt, va_list args);
        static void audioCallback(void* const udata, Uint8* const stream, int const len);

        SDL_Window* _window;
        SDL_GLContext _glContext;
        SDL_AudioSpec _audioSpec;
        SDL_AudioDeviceID _audioDev;

        LifeCycle _fsm;

        Audio* _audio;
        Config* _config;
        Led* _led;
        Logger* _logger;
        Video* _video;
        Input* _input;
        Perf* _perf;

        Control* _control;
        Memory* _memory;

        Desktop _desktop;
        Timer _runningTime;
        uint64_t _nextFrameTime;
        uint64_t _coreUsPerFrame;
        bool _syncExact;

        retro_perf_counter _runPerf;

        Fifo _fifo;
        lua_State* _L;
    };
}
