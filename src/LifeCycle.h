#pragma once

// Generated with FSM compiler, https://github.com/leiradel/luamods/ddlt

//#line 1 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

    namespace hc {
        class Application;
    }

    typedef hc::Application Application;
    typedef char const* const_cstr;


#include <stdarg.h>

class LifeCycle {
public:
    enum class State {
        ConsoleLoaded,
        GamePaused,
        GameRunning,
        Quit,
        Start,
    };

    typedef void (*VPrintf)(void* ud, const char* fmt, va_list args);

    LifeCycle(Application& ctx) : ctx(ctx), __state(State::Start), __vprintf(nullptr), __vprintfud(nullptr) {}
    LifeCycle(Application& ctx, VPrintf printer, void* printerud) : ctx(ctx), __state(State::Start), __vprintf(printer), __vprintfud(printerud) {}

    State currentState() const { return __state; }

#ifdef DEBUG_FSM
    const char* stateName(State state) const;
    void printf(const char* fmt, ...);
#endif

    bool loadConsole(const_cstr name);
    bool loadGame(const_cstr path);
    bool pauseGame();
    bool quit();
    bool resetGame();
    bool resumeGame();
    bool step();
    bool unloadConsole();
    bool unloadGame();

protected:
    bool before() const;
    bool before(State state) const;
    void after() const;
    void after(State state) const;

    Application& ctx;
    State __state;
    VPrintf __vprintf;
    void* __vprintfud;
};
