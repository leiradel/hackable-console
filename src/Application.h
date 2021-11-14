#pragma once

#include "Desktop.h"
#include "Scriptable.h"

#include "components/Logger.h"
#include "components/Config.h"
#include "components/Audio.h"
#include "components/Video.h"
#include "components/Led.h"
#include "components/Input.h"
#include "components/Perf.h"

#include "LifeCycle.h"

#include "Control.h"
#include "Memory.h"
#include "Devices.h"
#include "LuaRepl.h"
#include "Debugger.h"

#include "thread/Semaphore.h"

#include <SDL.h>
#include <SDL_opengl.h>

extern "C" {
    #include <lua.h>
}

#include <stdarg.h>
#include <atomic>
#include <thread>

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
        static void runFrame(Application* self);

        SDL_Window* _window;
        SDL_GLContext _glContext;

        LifeCycle _fsm;

        Logger _logger;
        Config _config;
        Video _video;
        Led _led;
        Audio _audio;
        Input _input;
        Perf _perf;
        
        Control _control;
        MemorySelector _memorySelector;
        Devices _devices;
        LuaRepl _repl;
        Debugger _debugger;

        Timer _runningTime;
        uint64_t _nextFrameTime;
        uint64_t _coreUsPerFrame;
        Semaphore _coreRun;
        Semaphore _appRun;
        std::atomic<bool> _exit;
        std::thread _coreThread;

        retro_perf_counter _runPerf;

        lua_State* _L;
    };
}
