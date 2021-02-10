#pragma once

#include "Desktop.h"
#include "Handle.h"

#include <imgui.h>
#include <imgui_memory_editor.h>
#include <imguial_sparkline.h>

extern "C" {
    #include <hcdebug.h>
}

#include <string>
#include <vector>

extern "C" {
    #include <lauxlib.h>
}

namespace hc {
    class Memory {
    public:
        virtual ~Memory() {}

        virtual char const* name() const = 0;
        virtual uint64_t base() const = 0;
        virtual uint64_t size() const = 0;
        virtual bool readonly() const = 0;
        virtual uint8_t peek(uint64_t address) const = 0;
        virtual void poke(uint64_t address, uint8_t value) = 0;

        unsigned requiredDigits();
    };

    class MemorySelector : public View {
    public:

        MemorySelector(Desktop* desktop) : View(desktop), _selected(0), _viewCount(0) {}
        virtual ~MemorySelector() {}

        void init();
        void add(Handle<Memory*> memory);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;

    protected:
        std::vector<Handle<Memory*>> _regions;
        int _selected;

        unsigned _viewCount;
    };

    class MemoryWatch : public View {
    public:
        MemoryWatch(Desktop* desktop, char const* title, Handle<Memory*> handle);
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
        Handle<Memory*> _handle;
        MemoryEditor _editor;

        ImGuiAl::BufferedSparkline<SparklineCount> _sparkline;
        size_t _lastPreviewAddress;
        int _lastEndianess;
        ImGuiDataType _lastType;
    };
}
