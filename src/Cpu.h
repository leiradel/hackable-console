#pragma once

#include "Desktop.h"
#include "Memory.h"

extern "C" {
    #include "hcdebug.h"
}

#include <stdint.h>
#include <string>

namespace hc {
    class DebugMemory : public Memory {
    public:
        DebugMemory(hc_Memory const* memory, void* userdata) : _memory(memory), _userdata(userdata) {}
        virtual ~DebugMemory() {}

        // Memory
        virtual char const* name() const override { return _memory->v1.description; }
        unsigned alignment() const { return _memory->v1.alignment; }
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
        ~Cpu() {}
        
        static Cpu* create(Desktop* desktop, hc_Cpu const* cpu, void* userdata);

        char const* name() const { return _cpu->v1.description; }
        unsigned type() const { return _cpu->v1.type; }
        bool isMain() const { return _cpu->v1.is_main; }

        Memory* mainMemory() const { return _memory; }

        uint64_t getRegister(unsigned reg) const { return _cpu->v1.get_register(_userdata, reg); }

        void stepInto() const { if (canStepInto()) _cpu->v1.step_into(_userdata); }
        void stepOver() const { if (canStepOver()) _cpu->v1.step_over(_userdata); }
        void stepOut() const { if (canStepOut()) _cpu->v1.step_out(_userdata); }

        bool canStepInto() const { return _cpu->v1.step_into != nullptr; }
        bool canStepOver() const { return _cpu->v1.step_over != nullptr; }
        bool canStepOut() const { return _cpu->v1.step_out != nullptr; }

        unsigned setBreakpoint(uint64_t address);
        unsigned setIoWatchpoint(uint64_t address, uint64_t length, int read, int write);
        unsigned setIntBreakpoint(unsigned type);

        void drawRegister(unsigned reg, char const* name, unsigned width, bool highlight);
        void drawFlags(unsigned reg, char const* name, char const* const* flags, unsigned width, bool highlight);

        virtual uint64_t instructionLength(uint64_t address, Memory const* memory) = 0;
        virtual void disasm(uint64_t address, Memory const* memory, char* buffer, size_t size, char* tooltip, size_t ttsz) = 0;

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameUnloaded() override;

    protected:
        Cpu(Desktop* desktop, hc_Cpu const* cpu, void* userdata);

        hc_Cpu const* const _cpu;
        void* const _userdata;
        bool _valid;
        std::string _title;
        Memory* _memory;
    };
}
