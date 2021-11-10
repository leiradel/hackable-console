#pragma once

#include "Desktop.h"
#include "components/Config.h"
#include "Cpu.h"
#include "Memory.h"

extern "C" {
    #include "hcdebug.h"
}

#include <vector>
#include <mutex>
#include <condition_variable>

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
            , _paused(false)
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
        void executionBreakpoint(hc_ExecutionBreakpoint const* event);
        void interruptBreakpoint(hc_InterruptBreakpoint const* event);
        void memoryWatchpoint(hc_MemoryWatchpoint const* event);
        void registerWatchpoint(hc_RegisterWatchpoint const* event);
        void ioWatchpoint(hc_IoWatchpoint const* event);
        void genericBreakpoint(hc_Breakpoint const* event);

        static void handleEvent(void* frontend_user_data, hc_Event const* event);

        Config* _config;
        MemorySelector* _memorySelector;

        hc_DebuggerIf* _debuggerIf;

        std::vector<hc_Cpu const*> _cpus;
        int _selectedCpu;

        std::mutex _mutex;
        std::condition_variable _gate;
        volatile bool _paused;
    };
}
