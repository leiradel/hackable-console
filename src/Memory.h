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
    };

    class CoreMemory : public Memory {
    public:
        CoreMemory(char const* name, bool readonly);
        virtual ~CoreMemory() {}

        bool addBlock(void* data, uint64_t offset, uint64_t base, uint64_t size);

        // Memory
        virtual char const* name() const override { return _name.c_str(); }
        virtual uint64_t base() const override { return _base; }
        virtual uint64_t size() const override { return _size; }
        virtual bool readonly() const override { return _readonly; }
        virtual uint8_t peek(uint64_t address) const override;
        virtual void poke(uint64_t address, uint8_t value) override;

    protected:
        struct Block {
            Block(void* data, uint64_t offset, uint64_t size) : data(data), offset(offset), size(size) {}
            void* data;
            uint64_t offset;
            uint64_t size;
        };

        std::string _name;
        uint64_t _base;
        uint64_t _size;
        bool _readonly;
        std::vector<Block> _blocks;
    };

    class DebugMemory : public Memory {
    public:
        DebugMemory(hc_Memory const* memory, void* userdata) : _memory(memory), _userdata(userdata) {}
        virtual ~DebugMemory() {}

        // Memory
        virtual char const* name() const override { return _memory->v1.description; }
        virtual uint64_t base() const override { return _memory->v1.base_address; }
        virtual uint64_t size() const override { return _memory->v1.size; }
        virtual bool readonly() const override { return _memory->v1.poke == nullptr; }
        virtual uint8_t peek(uint64_t address) const override { return _memory->v1.peek(_userdata, address); }
        virtual void poke(uint64_t address, uint8_t value) override { _memory->v1.poke(_userdata, address, value); }

    protected:
        hc_Memory const* const _memory;
        void* const _userdata;
    };

    class MemorySelector : public View {
    public:

        MemorySelector(Desktop* desktop) : View(desktop), _selected(0), _viewCount(0) {}
        virtual ~MemorySelector() {}

        void init();
        void add(Memory* memory);
        Memory* translate(Handle<Memory*> const handle);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;

    protected:
        HandleAllocator<Memory*> _handles;

        std::vector<Memory*> _regions;
        int _selected;

        unsigned _viewCount;
    };

    class MemoryWatch : public View {
    public:
        MemoryWatch(Desktop* desktop, char const* title, MemorySelector* memorySelector, Handle<Memory*> handle);
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

        MemorySelector* _memorySelector;
        Handle<Memory*> _handle;

        MemoryEditor _editor;

        ImGuiAl::BufferedSparkline<SparklineCount> _sparkline;
        size_t _lastPreviewAddress;
        int _lastEndianess;
        ImGuiDataType _lastType;
    };
}
