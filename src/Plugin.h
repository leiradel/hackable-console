#pragma once

#include <Frontend.h>

#include <vector>

extern "C" {
    #include <lua.h>
}

namespace hc {
    class Logger;

    class Plugin {
    public:
        enum class Type {
            Audio,
            Config,
            Input,
            Led,
            Logger,
            Perf,
            Video,

            Control,
            Memory,

            Manager
        };

        virtual ~Plugin() {}

        char const* getTypeName();

        virtual Type getType() = 0;
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
        virtual void onDraw(bool* opened) = 0;
        virtual void onGameUnloaded() = 0;
        virtual void onConsoleUnloaded() = 0;
        virtual void onQuit() = 0;

        virtual int push(lua_State* const L) = 0;
    };

    class Plugins : public Plugin {
    public:
        Plugins();
        virtual ~Plugins() {}

        void init(Logger* const logger);
        void add(Plugin* const plugin);

        // hc::Plugin
        virtual Type getType() override { return Type::Manager; }
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
        virtual void onDraw(bool* opened) override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        virtual int push(lua_State* const L) override;

    protected:
        struct View {
            Plugin* plugin;
            bool opened;
        };

        Logger* _logger;
        std::vector<View> _plugins;        
    };
}
