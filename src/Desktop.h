#pragma once

#include <Frontend.h>

#include <vector>

extern "C" {
    #include <lua.h>
}

namespace hc {
    class Logger;

    class View {
    public:
        virtual ~View() {}

        virtual char const* getTitle() = 0;
        virtual void onStarted() = 0;
        virtual void onConsoleLoaded() = 0;
        virtual void onGameLoaded() = 0;
        virtual void onGamePaused() = 0;
        virtual void onGameResumed() = 0;
        virtual void onGameReset() = 0;
        virtual void onFrame() = 0;
        virtual void onDraw(bool* opened) = 0;
        virtual void onGameUnloaded() = 0;
        virtual void onConsoleUnloaded() = 0;
        virtual void onQuit() = 0;

        virtual int push(lua_State* const L) = 0;

    protected:
        friend class Desktop;

        bool _opened;
    };

    class Desktop : public View {
    public:
        Desktop();
        virtual ~Desktop() {}

        void init(Logger* const logger);
        void add(View* const plugin, bool const destroy);

        // hc::View
        virtual char const* getTitle() override;
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
        struct Vieww {
            View* view;
            bool destroy;
        };

        Logger* _logger;
        std::vector<Vieww> _views;        
    };
}
