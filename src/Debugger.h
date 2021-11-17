#pragma once

#include "Desktop.h"
#include "components/Config.h"
#include "Cpu.h"
#include "Memory.h"

extern "C" {
    #include "hcdebug.h"
}

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace hc {
    class Disasm : public View {
    public:
        Disasm(Desktop* desktop, Cpu* cpu);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameUnloaded() override;
        virtual void onDraw() override;

    protected:
        bool _valid;
        Cpu* _cpu;
        std::string _title;
    };

    class Debugger : public View {
    public:
        Debugger(Desktop* desktop, Config* config, MemorySelector* memorySelector);
        virtual ~Debugger() {}

        void init();

        bool paused() const;
        void step();

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
        std::atomic<bool> _paused;
        std::atomic<bool> _step;
    };
}
