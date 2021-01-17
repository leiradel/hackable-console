#pragma once

#include "Desktop.h"
#include "Logger.h"

#include <imgui_memory_editor.h>
#include <imguial_sparkline.h>

#include <string>
#include <vector>

extern "C" {
    #include <lauxlib.h>
}

namespace hc {
    class Memory : public View {
    public:
        struct Region {
            Region(std::string&& name, void* data, size_t offset, size_t size, size_t base, bool readOnly);

            std::string name;
            void* data;
            size_t offset;
            size_t size;
            size_t base;
            bool readOnly;
        };

        typedef size_t Handle;

        Memory(Desktop* desktop) : View(desktop), _logger(nullptr), _selected(0), _viewCount(0) {}
        virtual ~Memory() {}

        void init(Logger* const logger);
        Region* lock(Handle const handle);

        static Memory* check(lua_State* const L, int const index);

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
        static int l_addRegion(lua_State* const L);

        Logger* _logger;

        std::vector<Region> _regions;
        int _selected;

        unsigned _viewCount;
    };

    class MemoryWatch : public View {
    public:
        MemoryWatch(Desktop* desktop) : View(desktop), _logger(nullptr), _memory(nullptr), _handle(0) {}
        virtual ~MemoryWatch() {}

        void init(Logger* const logger, char const* title, Memory* const memory, Memory::Handle const handle);

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
        enum {
            SparklineCount = 512
        };

        Logger* _logger;

        std::string _title;

        Memory* _memory;
        Memory::Handle _handle;
        MemoryEditor _editor;

        ImGuiAl::BufferedSparkline<SparklineCount> _sparkline;
        int _lastEndianess;
        ImGuiDataType _lastType;
    };

#if 0
    class Sparkline : public View {
    public:
        Sparkline(Desktop* desktop) : View(desktop), _memory(nullptr), _handle(0) {}
        virtual ~Sparkline() {}

        void init(char const* title, Memory* const memory, Memory::Handle const handle, uint64_t const address);

        void add(uint8_t const value) {
            _sparkline.add(value);
        }

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
        std::string _title;
        Memory* _memory;
        Memory::Handle _handle;
        uint64_t _address;
        ImGuiAl::Sparkline<uint8_t, 600> _sparkline;
    };
#endif
}
