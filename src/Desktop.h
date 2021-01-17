#pragma once

#include <Frontend.h>

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
        virtual void onStarted() = 0;
        virtual void onConsoleLoaded() = 0;
        virtual void onGameLoaded() = 0;
        virtual void onGamePaused() = 0;
        virtual void onGameResumed() = 0;
        virtual void onGameReset() = 0;
        virtual void onFrame() = 0;
        virtual void onDraw() = 0;
        virtual void onGameUnloaded() = 0;
        virtual void onConsoleUnloaded() = 0;
        virtual void onQuit() = 0;

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

        // hc::View
        virtual char const* getTitle() override;
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
    };
}
