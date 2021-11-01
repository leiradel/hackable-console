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
        DebugMemory(hc_Memory const* memory) : _memory(memory) {}
        virtual ~DebugMemory() {}

        // Memory
        virtual char const* id() const override { return _memory->v1.id; }
        virtual char const* name() const override { return _memory->v1.description; }
        unsigned alignment() const { return _memory->v1.alignment; }
        virtual uint64_t base() const override { return _memory->v1.base_address; }
        virtual uint64_t size() const override { return _memory->v1.size; }
        virtual uint8_t peek(uint64_t address) const override { return _memory->v1.peek(address); }
        virtual void poke(uint64_t address, uint8_t value) override { _memory->v1.poke(address, value); }

        virtual bool readonly() const override { return false; }

    protected:
        hc_Memory const* const _memory;
    };

    class Cpu : public View {
    public:
        ~Cpu() {}
        
        static Cpu* create(Desktop* desktop, hc_Cpu const* cpu);

        char const* name() const { return _cpu->v1.description; }
        unsigned type() const { return _cpu->v1.type; }
        bool isMain() const { return _cpu->v1.is_main; }

        Memory* mainMemory() const { return _memory; }

        uint64_t getRegister(unsigned reg) const { return _cpu->v1.get_register(reg); }

        void drawRegister(unsigned reg, char const* name, unsigned width, bool highlight);
        void drawFlags(unsigned reg, char const* name, char const* const* flags, unsigned width, bool highlight);

        virtual uint64_t instructionLength(uint64_t address, Memory const* memory) = 0;
        virtual void disasm(uint64_t address, Memory const* memory, char* buffer, size_t size, char* tooltip, size_t ttsz) = 0;

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameUnloaded() override;

    protected:
        Cpu(Desktop* desktop, hc_Cpu const* cpu);

        hc_Cpu const* const _cpu;
        bool _valid;
        std::string _title;
        Memory* _memory;
    };
}
