#pragma once

#include "Desktop.h"
#include "Config.h"
#include "Memory.h"

extern "C" {
    #include "hcdebug.h"
}

namespace hc {
    class Register {
    public:
        Register(hc_Register const* reg, void* userdata) : _register(reg), _userdata(userdata), _previousValue(get()) {}

        char const* name() const { return _register->v1.name; }
        unsigned size() const { return 1U << (_register->v1.flags & HC_SIZE_MASK); }
        bool programCounter() const { return (_register->v1.flags & HC_PROGRAM_COUNTER) != 0; }
        bool stackPointer() const { return (_register->v1.flags & HC_STACK_POINTER) != 0; }
        bool memoryPointer() const { return (_register->v1.flags & HC_MEMORY_POINTER) != 0; }
        uint64_t get() const { return _register->v1.get(_userdata); }
        void set(uint64_t value) const { if (!readonly()) _register->v1.set(_userdata, value); }
        char const* const* bits() { return _register->v1.bits; }

        bool readonly() const { return _register->v1.set == nullptr; }

        bool changed();
        void clearChanged() { _hasChanged = false; }

    protected:
        hc_Register const* const _register;
        void* const _userdata;
        uint64_t _previousValue;
        bool _hasChanged;
    };

    class DebugMemory : public Memory {
    public:
        DebugMemory(hc_Memory const* memory, void* userdata) : _memory(memory), _userdata(userdata) {}
        virtual ~DebugMemory() {}

        // Memory
        virtual char const* name() const override { return _memory->v1.description; }
        unsigned alignment() const { return 1 << (_memory->v1.flags & HC_ALIGNMENT_MASK); }
        bool cpuAddressable() const { return (_memory->v1.flags & HC_CPU_ADDRESSABLE) != 0; }
        virtual uint64_t base() const override { return _memory->v1.base_address; }
        virtual uint64_t size() const override { return _memory->v1.size; }
        virtual uint8_t peek(uint64_t address) const override { return _memory->v1.peek(_userdata, address); }

        virtual void poke(uint64_t address, uint8_t value) override {
            if (!readonly()) _memory->v1.poke(_userdata, address, value);
        }

        virtual bool readonly() const override { return _memory->v1.poke == nullptr; }

    protected:
        hc_Memory const* const _memory;
        void* const _userdata;
    };

    class Cpu : public View {
    public:
        Cpu(Desktop* desktop, hc_Cpu const* cpu, void* userdata);

        hc_CpuType type() const { return _cpu->v1.type; }
        char const* name() const { return _cpu->v1.description; }
        bool main() const { return (_cpu->v1.flags & HC_CPU_MAIN) != 0; }

        void pause() const { if (canPause()) _cpu->v1.pause(_userdata); }
        void resume() const { if (canResume()) _cpu->v1.resume(_userdata); }
        void stepInto() const { if (canStepInto()) _cpu->v1.step_into(_userdata); }
        void stepOver() const { if (canStepOver()) _cpu->v1.step_over(_userdata); }
        void stepOut() const { if (canStepOut()) _cpu->v1.step_out(_userdata); }

        bool canPause() const { return _cpu->v1.pause != nullptr; }
        bool canResume() const { return _cpu->v1.resume != nullptr; }
        bool canStepInto() const { return _cpu->v1.step_into != nullptr; }
        bool canStepOver() const { return _cpu->v1.step_over != nullptr; }
        bool canStepOut() const { return _cpu->v1.step_out != nullptr; }

        Memory* mainMemory() const { return _mainMemory; }
        Register* programCounter() { return _programCounter < 0 ? nullptr : &_registers[_programCounter]; }
        Register* stackPointer() { return _stackPointer < 0 ? nullptr : &_registers[_stackPointer]; }

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameUnloaded() override;
        virtual void onFrame() override;
        virtual void onDraw() override;

    protected:
        hc_Cpu const* const _cpu;
        void* const _userdata;
        bool _valid;
        std::string _title;

        Memory* _mainMemory;
        std::vector<Register> _registers;

        int _programCounter;
        int _stackPointer;
        std::vector<int> _memoryPointers;
    };

    class Disasm : public View {
    public:
        Disasm(Desktop* desktop, Cpu* cpu, Memory* memory, Register* reg);
        Disasm(Desktop* desktop, Cpu* cpu, Memory* memory, uint64_t address);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameUnloaded() override;
        virtual void onDraw() override;

    protected:
        bool _valid;
        Memory* _memory;
        Register* _register;
        uint64_t _address;
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
        int _selectedCpu;
    };
}
