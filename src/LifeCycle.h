#pragma once

// Generated with FSM compiler, https://github.com/leiradel/luamods/ddlt

#line 1 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

    namespace hc {
        class Application;
    }

    typedef hc::Application Application;
    typedef char const* const_cstr;


class LifeCycle {
public:
    enum class State {
        ConsoleLoaded,
        GamePaused,
        GameRunning,
        Quit,
        Start,
    };

    LifeCycle(Application& ctx): ctx(ctx), __state(State::Start) {}

    State currentState() const { return __state; }

#ifdef DEBUG_FSM
    const char* stateName(State state) const;
    void printf(const char* fmt, ...);
#endif

    bool loadConsole(const_cstr core);
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
};
