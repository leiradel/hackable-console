// Generated with FSM compiler, https://github.com/leiradel/luamods/ddlt

#include "LifeCycle.h"

#line 10 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

    #include "Application.h"


#ifdef DEBUG_FSM
const char* LifeCycle::stateName(State state) const {
    switch (state) {
        case State::ConsoleLoaded: return "ConsoleLoaded";
        case State::GamePaused: return "GamePaused";
        case State::GameRunning: return "GameRunning";
        case State::Quit: return "Quit";
        case State::Start: return "Start";
        default: break;
    }

    return NULL;
}

void LifeCycle::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ctx.printf(fmt, args);
    va_end(args);
}
#endif

bool LifeCycle::before() const {
    return true;
}

bool LifeCycle::before(State state) const {
    switch (state) {
        default: break;
    }

    return true;
}

void LifeCycle::after() const {
}

void LifeCycle::after(State state) const {
    switch (state) {
        default: break;
    }
}

bool LifeCycle::loadConsole(const_cstr core) {
    switch (__state) {
        case State::Start: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
                );
#endif

                return false;
            }

#line 18 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

            if (!ctx.loadConsole(core)) {
                return false;
            }
        
            __state = State::ConsoleLoaded;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::loadGame(const_cstr path) {
    switch (__state) {
        case State::ConsoleLoaded: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

#line 34 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

            if (!ctx.loadGame(path)) {
                return false;
            }
        
            __state = State::GamePaused;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::GamePaused)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::pauseGame() {
    switch (__state) {
        case State::GameRunning: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

            __state = State::GamePaused;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::GamePaused)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::quit() {
    switch (__state) {
        case State::ConsoleLoaded: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            bool __ok = unloadConsole() &&
                        quit();

            if (__ok) {
                after(__state);
                after();

            }
            else {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed to switch to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif
            }

            return __ok;
        }
        break;

        case State::GamePaused: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            bool __ok = unloadGame() &&
                        quit();

            if (__ok) {
                after(__state);
                after();

            }
            else {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed to switch to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif
            }

            return __ok;
        }
        break;

        case State::GameRunning: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            bool __ok = unloadGame() &&
                        quit();

            if (__ok) {
                after(__state);
                after();

            }
            else {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed to switch to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif
            }

            return __ok;
        }
        break;

        case State::Start: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Quit)
                );
#endif

                return false;
            }

            __state = State::Quit;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::Quit)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::resetGame() {
    switch (__state) {
        case State::GamePaused: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

            bool __ok = resumeGame() &&
                        resetGame() &&
                        pauseGame();

            if (__ok) {
                after(__state);
                after();

            }
            else {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed to switch to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif
            }

            return __ok;
        }
        break;

        case State::GameRunning: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GameRunning)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GameRunning)
                );
#endif

                return false;
            }

#line 64 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

            ctx.resetGame();
        
            __state = State::GameRunning;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::GameRunning)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::resumeGame() {
    switch (__state) {
        case State::GamePaused: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GameRunning)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GameRunning)
                );
#endif

                return false;
            }

            __state = State::GameRunning;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::GameRunning)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::step() {
    switch (__state) {
        case State::GamePaused: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::GamePaused)
                );
#endif

                return false;
            }

#line 48 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

            ctx.step();
        
            __state = State::GamePaused;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::GamePaused)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::unloadConsole() {
    switch (__state) {
        case State::ConsoleLoaded: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Start)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::Start)
                );
#endif

                return false;
            }

#line 30 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

            ctx.unloadConsole();
        
            __state = State::Start;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::Start)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

bool LifeCycle::unloadGame() {
    switch (__state) {
        case State::GamePaused: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
                );
#endif

                return false;
            }

#line 52 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

            if (!ctx.unloadGame()) {
                return false;
            }
        
            __state = State::ConsoleLoaded;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
            );
#endif
            return true;
        }
        break;

        case State::GameRunning: {
            if (!before()) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed global precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
                );
#endif

                return false;
            }

            if (!before(__state)) {
#ifdef DEBUG_FSM
                printf(
                    "FSM %s:%u Failed state precondition while switching to %s",
                    __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
                );
#endif

                return false;
            }

#line 68 "/home/leiradel/Develop/hackable-console/src/LifeCycle.fsm"

            if (!ctx.unloadGame()) {
                return false;
            }
        
            __state = State::ConsoleLoaded;
            after(__state);
            after();

#ifdef DEBUG_FSM
            printf(
                "FSM %s:%u Switched to %s",
                __FUNCTION__, __LINE__, stateName(State::ConsoleLoaded)
            );
#endif
            return true;
        }
        break;

        default: break;
    }

    return false;
}

