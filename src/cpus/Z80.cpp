#include "Z80.h"
#include "Debugger.h"

#define CHIPS_IMPL
#include <z80dasm.h>

#include <IconsFontAwesome4.h>
#include <imgui.h>
#include <imguial_button.h>

hc::Z80::Z80(Desktop* desktop, hc_DebuggerIf const* debuggerIf, hc_Cpu const* cpu)
    : Cpu(desktop, debuggerIf, cpu)
    , _hasChanged(0) {

    for (unsigned i = 0; i < HC_Z80_NUM_REGISTERS; i++) {
        _previousValue[i] = _debuggerIf->v1.get_register(_cpu, i);
    }
}

uint64_t hc::Z80::instructionLength(uint64_t address, Memory const* memory) {
    char buffer[32];
    return disasm(address, memory, buffer, sizeof(buffer));
}

void hc::Z80::disasm(uint64_t address, Memory const* memory, char* buffer, size_t size, char* tooltip, size_t ttsz) {
    (void)ttsz;

    disasm(address, memory, buffer, size);
    *tooltip = 0;
}

void hc::Z80::onFrame() {
    _hasChanged = 0;
}

void hc::Z80::onDraw() {
    if (!_valid) {
        return;
    }

    for (unsigned i = 0; i < HC_Z80_NUM_REGISTERS; i++) {
        static char const* const names[HC_Z80_NUM_REGISTERS] = {
            "A", "F", "BC", "DE", "HL", "IX", "IY", "AF2", "BC2", "DE2", "HL2", "I", "R", "SP", "PC", "IFF", "IM", "WZ"
        };

        static uint8_t const width[HC_Z80_NUM_REGISTERS] = {
            8, 8, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 8, 16, 16, 2, 8, 16
        };

        uint32_t const regBit = UINT32_C(1) << i;

        if ((_hasChanged & regBit) == 0) {
            uint64_t const value = _debuggerIf->v1.get_register(_cpu, i);
            _hasChanged |= ((value == _previousValue[i]) - 1) & regBit;
            _previousValue[i] = value;
        }

        bool const highlight = (regBit & _hasChanged) != 0;

        if (i == HC_Z80_F) {
            static char const* const flags[] = {"S", "Z", "Y", "H", "X", "PV", "N", "C"};
            drawFlags(i, names[i], flags, width[i], highlight);
        }
        else if (i == HC_Z80_IFF) {
            static char const* const flags[] = {"IFF1", "IFF2"};
            drawFlags(i, names[i], flags, width[i], highlight);
        }
        else {
            drawRegister(i, names[i], width[i], highlight);
        }
    }

    if (ImGui::Button(ICON_FA_CODE " Disassembly")) {
        _desktop->addView(new Disasm(_desktop, this, mainMemory(), HC_Z80_PC), false, true);
    }
}

uint64_t hc::Z80::disasm(uint64_t address, Memory const* memory, char* buffer, size_t size) {
    struct Userdata {
        Memory const* memory;
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
    z80dasm_op(static_cast<uint16_t>(address), inCallback, outCallback, &ud);
    ud.buffer[ud.position] = 0;

    return ud.address - address;
}
