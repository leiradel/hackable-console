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
        void tick();
        void memoryWatchpoint(unsigned id, hc_Memory const* memory, uint64_t address, unsigned event);
        void registerWatchpoint(unsigned id, hc_Cpu const* cpu, unsigned reg, uint64_t old_value);
        void executionBreakpoint(unsigned id, hc_Cpu const* cpu, uint64_t address);
        void ioWatchpoint(unsigned id, hc_Cpu const* cpu, uint64_t address, unsigned event, uint64_t value);
        void interruptBreakpoint(unsigned id, hc_Cpu const* cpu, unsigned type, uint64_t address);
        void genericBreakpoint(unsigned id, hc_Breakpoint const* break_point, uint64_t arg1, uint64_t arg2);

        static void tick(void* ud);
        static void memoryWatchpoint(void* ud, unsigned id, hc_Memory const* memory, uint64_t address, unsigned event);
        static void registerWatchpoint(void* ud, unsigned id, hc_Cpu const* cpu, unsigned reg, uint64_t old_value);
        static void executionBreakpoint(void* ud, unsigned id, hc_Cpu const* cpu, uint64_t address);
        static void ioWatchpoint(void* ud, unsigned id, hc_Cpu const* cpu, uint64_t address, unsigned event, uint64_t value);
        static void interruptBreakpoint(void* ud, unsigned id, hc_Cpu const* cpu, unsigned type, uint64_t address);
        static void genericBreakpoint(void* ud, unsigned id, hc_Breakpoint const* break_point, uint64_t arg1, uint64_t arg2);

        Config* _config;
        MemorySelector* _memorySelector;

        hc_DebuggerIf* _debuggerIf;

        std::vector<hc_Cpu const*> _cpus;
        int _selectedCpu;
    };
}
