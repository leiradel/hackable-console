#pragma once

#include "Plugin.h"
#include "Logger.h"
#include "LifeCycle.h"

#include <string>
#include <set>

namespace hc {
    class Control : public Plugin {
    public:
        Control();

        void init(Logger* logger, LifeCycle* fsm);

        void setSystemInfo(retro_system_info const* info);
        void addConsole(char const* const name);

        // hc::Plugin
        virtual char const* getName() override;
        virtual char const* getVersion() override;
        virtual char const* getLicense() override;
        virtual char const* getCopyright() override;
        virtual char const* getUrl() override;

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

    protected:
        LifeCycle* _fsm;
        Logger* _logger;

        std::set<std::string> _consoles;
        int _selected;
        std::string _extensions;
        std::string _lastGameFolder;
    };
}
