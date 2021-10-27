#pragma once

#include "Cpu.h"

#include <stdint.h>

namespace hc {
    class Z80 : public Cpu {
    public:
        Z80(Desktop* desktop, hc_Cpu const* cpu, void* userdata);
        ~Z80() {}

        // hc::Cpu
        virtual uint64_t instructionLength(uint64_t address, Memory const* memory) override;
        virtual void disasm(uint64_t address, Memory const* memory, char* buffer, size_t size, char* tooltip, size_t ttsz) override;

        // hc::View
        virtual void onFrame() override;
        virtual void onDraw() override;

    protected:
        static uint64_t disasm(uint64_t address, Memory const* memory, char* buffer, size_t size);

        uint32_t _hasChanged;
        uint16_t _previousValue[HC_Z80_NUM_REGISTERS];
    };
}
