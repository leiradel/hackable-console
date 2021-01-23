#pragma once

#include "Timer.h"

#include <lrcpp/Frontend.h>

#include <string.h>
#include <map>

extern "C" {
    #include <lua.h>
}

namespace hc {
    class Logger;
    class Desktop;

    class View {
    public:
        View(Desktop* desktop) : _desktop(desktop) {}
        virtual ~View() {}

        virtual char const* getTitle() = 0;
        virtual void onStarted() {}
        virtual void onCoreLoaded() {}
        virtual void onGameLoaded() {}
        virtual void onGameStarted() {}
        virtual void onGamePaused() {}
        virtual void onGameResumed() {}
        virtual void onGameReset() {}
        virtual void onFrame() {}
        virtual void onStep() {}
        virtual void onDraw() {}
        virtual void onGameUnloaded() {}
        virtual void onConsoleUnloaded() {}
        virtual void onQuit() {}

        virtual int push(lua_State* const L) = 0;

    protected:
        Desktop* _desktop;
    };

    class Desktop : public View {
    public:
        Desktop();
        virtual ~Desktop() {}

        void init(Logger* const logger);
        void add(View* const view, bool const top, bool const free, char const* const id);
        double drawFps();
        void resetDrawFps();
        double frameFps();
        void resetFrameFps();

        // hc::View
        virtual char const* getTitle() override;
        virtual void onStarted() override;
        virtual void onCoreLoaded() override;
        virtual void onGameLoaded() override;
        virtual void onGameStarted() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onGameReset() override;
        virtual void onFrame() override;
        virtual void onStep() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        virtual int push(lua_State* const L) override;

    protected:
        struct ViewProperties {
            View* view;
            bool top;
            bool free;
            char const* id;
            bool opened;

            bool operator<(ViewProperties const& other) const {
                return strcmp(id, other.id) < 0;
            }
        };

        Logger* _logger;
        
        std::map<std::string, ViewProperties> _views;

        uint64_t _drawCount;
        Timer _drawTimer;

        uint64_t _frameCount;
        Timer _frameTimer;
    };
}
