#pragma once

#include <Frontend.h>

namespace hc {
    class Plugin {
    public:
        virtual ~Plugin() {}

        virtual char const* getName() = 0;
        virtual char const* getVersion() = 0;
        virtual char const* getLicense() = 0;
        virtual char const* getCopyright() = 0;
        virtual char const* getUrl() = 0;

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
    };
}
