#pragma once

#include "Desktop.h"
#include "Config.h"
#include "Cpu.h"
#include "Memory.h"

extern "C" {
    #include "hcdebug.h"
}

#include <vector>

namespace hc {
    class Disasm : public View {
    public:
        Disasm(Desktop* desktop, Cpu* cpu, Memory* memory, unsigned reg);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameUnloaded() override;
        virtual void onDraw() override;

    protected:
        bool _valid;
        Cpu* _cpu;
        Memory* _memory;
        unsigned _register;
        std::string _title;
    };

    class Debugger : public View {
    public:
        Debugger(Desktop* desktop, Config* config, MemorySelector* memorySelector)
            : View(desktop)
            , _config(config)
            , _memorySelector(memorySelector)
            , _debuggerIf(nullptr)
            , _selectedCpu(0)
        {}

        virtual ~Debugger() {}

        void init();

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameLoaded() override;
        virtual void onGameUnloaded() override;
        virtual void onDraw() override;

    protected:
        Config* _config;
        MemorySelector* _memorySelector;

        hc_DebuggerIf* _debuggerIf;
        void* _userdata;

        std::vector<hc_Cpu const*> _cpus;
        int _selectedCpu;
    };
}
