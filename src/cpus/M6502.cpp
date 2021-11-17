#include "M6502.h"
#include "Debugger.h"

#define CHIPS_IMPL
#include <m6502dasm.h>

#include <IconsFontAwesome4.h>
#include <imgui.h>
#include <imguial_button.h>

hc::M6502::M6502(Desktop* desktop, Debugger* debugger, hc_Cpu const* cpu) : Cpu(desktop, debugger, cpu), _hasChanged(0) {
    for (unsigned i = 0; i < HC_6502_NUM_REGISTERS; i++) {
        _previousValue[i] = _cpu->v1.get_register(i);
    }
}

uint64_t hc::M6502::instructionLength(uint64_t address, Memory const* memory) {
    char buffer[32];
    return disasm(address, memory, buffer, sizeof(buffer));
}

void hc::M6502::disasm(uint64_t address, Memory const* memory, char* buffer, size_t size, char* tooltip, size_t ttsz) {
    (void)ttsz;

    disasm(address, memory, buffer, size);
    *tooltip = 0;
}

void hc::M6502::onFrame() {
    _hasChanged = 0;
}

void hc::M6502::onDraw() {
    if (!_valid) {
        return;
    }

    for (unsigned i = 0; i < HC_6502_NUM_REGISTERS; i++) {
        static char const* const names[HC_6502_NUM_REGISTERS] = {
            "A", "X", "Y", "S", "PC", "P"
        };

        static uint8_t const width[HC_6502_NUM_REGISTERS] = {
            8, 8, 8, 8, 16, 8
        };

        uint32_t const regBit = UINT32_C(1) << i;

        if ((_hasChanged & regBit) == 0) {
            uint64_t const value = _cpu->v1.get_register(i);
            _hasChanged |= ((value == _previousValue[i]) - 1) & regBit;
            _previousValue[i] = value;
        }

        bool const highlight = (regBit & _hasChanged) != 0;

        if (i == HC_6502_P) {
            static char const* const flags[] = {"N", "V", "X", "B", "D", "I", "Z", "C"};
            drawFlags(i, names[i], flags, width[i], highlight);
        }
        else {
            drawRegister(i, names[i], width[i], highlight);
        }
    }
}

uint64_t hc::M6502::disasm(uint64_t address, hc::Memory const* memory, char* buffer, size_t size) {
    struct Userdata {
        hc::Memory const* memory;
        uint64_t address;

        char* buffer;
        size_t position;
        size_t size;
    };

    static auto const inCallback = [](void* user_data) -> uint8_t {
        auto const ud = static_cast<Userdata*>(user_data);
        return ud->memory->peek(ud->address++);
    };

    static auto const outCallback = [](char c, void* user_data) -> void {
        auto const ud = static_cast<Userdata*>(user_data);

        if (ud->position < ud->size) {
            ud->buffer[ud->position++] = c;
        }
    };

    Userdata ud = {memory, address, buffer, 0, size - 1};
    m6502dasm_op(static_cast<uint16_t>(address), inCallback, outCallback, &ud);
    ud.buffer[ud.position] = 0;

    return ud.address - address;
}

uint64_t hc::M6502::programCounter() const {
    return _cpu->v1.get_register(HC_6502_PC);
}
