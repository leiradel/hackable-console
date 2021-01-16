#pragma once

#include "Plugin.h"
#include "Logger.h"

#include <imgui_memory_editor.h>

#include <string>
#include <vector>

extern "C" {
    #include <lauxlib.h>
}

namespace hc {
    class Memory : public Plugin {
    public:
        Memory() : _logger(nullptr), _selected(0) {}

        void init(Logger* const logger);

        static Memory* check(lua_State* const L, int const index);

        // hc::Plugin
        virtual Type getType() override { return Type::Memory; }
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

        virtual int push(lua_State* const L) override;

    protected:
        static int l_addRegion(lua_State* const L);

        struct Region {
            Region(std::string&& name, void* data, size_t offset, size_t size, size_t base, bool readOnly);

            std::string name;
            void* data;
            size_t offset;
            size_t size;
            size_t base;
            bool readOnly;
        };

        struct View {
            View(Region const& region);

            Region region;
            MemoryEditor editor;
        };

        Logger* _logger;

        std::vector<Region> _regions;
        int _selected;

        std::vector<View> _views;
    };
}
