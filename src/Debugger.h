#pragma once

#include "Desktop.h"
#include "Config.h"
#include "Memory.h"

extern "C" {
    #include "hcdebug.h"
}

namespace hc {
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

    class Register {
    public:
        Register(hc_Register const* reg, void* userdata) : _register(reg), _userdata(userdata) {}

        uint64_t get() const { return _register->v1.get(_userdata); }
        void set(uint64_t value) const { _register->v1.set(_userdata, value); }

    protected:
        hc_Register const* const _register;
        void* const _userdata;
    };

    class Debugger : public View {
    public:
        Debugger(Desktop* desktop, Config* config, MemorySelector* memorySelector)
            : View(desktop)
            , _config(config)
            , _memorySelector(memorySelector)
            , _mainCpu(nullptr)
            , _debuggerIf(nullptr)
            , _selectedCpu(0)
        {}

        virtual ~Debugger() {}

        void init();

        virtual char const* getTitle() override;
        virtual void onGameLoaded() override;
        virtual void onGameUnloaded() override;
        virtual void onDraw() override;

    protected:
        Config* _config;
        MemorySelector* _memorySelector;

        hc_Cpu const* _mainCpu;

        hc_DebuggerIf* _debuggerIf;
        void* _userdata;
        int _selectedCpu;
    };
}
