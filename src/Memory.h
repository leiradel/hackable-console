#pragma once

#include "Desktop.h"
#include "Scriptable.h"

#include <imgui.h>
#include <imgui_memory_editor.h>
#include <imguial_sparkline.h>

#include <string>
#include <vector>

extern "C" {
    #include <lauxlib.h>
}

namespace hc {
    class Memory : public View, public Scriptable {
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

        Memory(Desktop* desktop) : View(desktop), _selected(0), _viewCount(0) {}
        virtual ~Memory() {}

        void init();
        Region* lock(Handle const handle);

        static Memory* check(lua_State* const L, int const index);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;

        // hc::Scriptable
        virtual int push(lua_State* const L) override;

    protected:
        static int l_addRegion(lua_State* const L);

        std::vector<Region> _regions;
        int _selected;

        unsigned _viewCount;
    };

    class MemoryWatch : public View {
    public:
        MemoryWatch(Desktop* desktop) : View(desktop), _memory(nullptr), _handle(0) {}
        virtual ~MemoryWatch() {}

        void init(char const* title, Memory* const memory, Memory::Handle const handle);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onFrame() override;
        virtual void onDraw() override;

    protected:
        enum {
            SparklineCount = 512
        };

        std::string _title;

        Memory* _memory;
        Memory::Handle _handle;
        MemoryEditor _editor;

        ImGuiAl::BufferedSparkline<SparklineCount> _sparkline;
        size_t _lastPreviewAddress;
        int _lastEndianess;
        ImGuiDataType _lastType;
    };
}
