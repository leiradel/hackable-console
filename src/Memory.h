#pragma once

#include "Desktop.h"
#include "Scriptable.h"
#include "Handle.h"

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
        struct Block {
            void* data;
            size_t offset;
            size_t size;
        };

        struct Region {
            std::string name;
            size_t base;
            size_t size;
            bool readOnly;
            std::vector<Block> blocks;

            uint8_t peek(size_t address) const;
            void poke(size_t address, uint8_t value);
        };

        Memory(Desktop* desktop) : View(desktop), _selected(0), _viewCount(0) {}
        virtual ~Memory() {}

        void init();
        Region* translate(Handle<Region> const handle);

        static Memory* check(lua_State* const L, int const index);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;

        // hc::Scriptable
        virtual int push(lua_State* const L) override;

    protected:
        static int l_addRegion(lua_State* const L);

        HandleAllocator<Region> _handles;
        std::vector<Handle<Region>> _regions;
        int _selected;
        unsigned _viewCount;
    };

    class MemoryWatch : public View {
    public:
        MemoryWatch(Desktop* desktop, char const* title, Memory* memory, Handle<Memory::Region> handle);
        virtual ~MemoryWatch() {}

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
        Handle<Memory::Region> _handle;

        MemoryEditor _editor;

        ImGuiAl::BufferedSparkline<SparklineCount> _sparkline;
        size_t _lastPreviewAddress;
        int _lastEndianess;
        ImGuiDataType _lastType;
    };
}
