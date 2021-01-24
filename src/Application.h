#pragma once

#include "Desktop.h"
#include "Scriptable.h"

#include "Logger.h"
#include "Config.h"
#include "Audio.h"
#include "Video.h"
#include "Led.h"
#include "Input.h"
#include "Perf.h"

#include "LifeCycle.h"

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
    class Application : public Desktop, public Scriptable {
    public:
        Application();

        bool init(std::string const& title, int const width, int const height);
        void destroy();
        void draw();
        void run();

        // LifeCycle
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

        // hc::View
        virtual char const* getTitle() override;
        virtual void onCoreLoaded() override;
        virtual void onGameLoaded() override;
        virtual void onGameStarted() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;

        // hc::Scriptable
        virtual int push(lua_State* const L) override;

    protected:
        static void sdlPrint(void* userdata, int category, SDL_LogPriority priority, char const* message);
        static void lifeCycleVprintf(void* ud, char const* fmt, va_list args);
        static void audioCallback(void* const udata, Uint8* const stream, int const len);

        SDL_Window* _window;
        SDL_GLContext _glContext;
        SDL_AudioSpec _audioSpec;
        SDL_AudioDeviceID _audioDev;

        LifeCycle _fsm;

        Control* _control;
        Memory* _memory;

        Timer _runningTime;
        uint64_t _nextFrameTime;
        uint64_t _coreUsPerFrame;
        bool _syncExact;

        retro_perf_counter _runPerf;

        Fifo _fifo;
        lua_State* _L;
    };
}
