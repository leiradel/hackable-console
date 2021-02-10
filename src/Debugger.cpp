#include "Debugger.h"
#include "cpus/Z80.h"

#include <IconsFontAwesome4.h>
#include <imgui.h>

#include <inttypes.h>
#include <math.h>

static void renderFrame(ImVec2 const min, ImVec2 const max, ImU32 const color) {
    ImDrawList* const draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, color, false);
}

hc::Cpu::Cpu(Desktop* desktop, hc_Cpu const* cpu, void* userdata) : View(desktop), _cpu(cpu), _userdata(userdata), _valid(true) {
    _title = ICON_FA_MICROCHIP " ";
    _title += _cpu->v1.description;

    _programCounter = _stackPointer = nullptr;

    for (unsigned i = 0; i < _cpu->v1.num_registers; i++) {
        hc_Register const* const reg = _cpu->v1.registers[i];

        if ((reg->v1.flags & HC_PROGRAM_COUNTER) != 0) {
            _programCounter = new Register(reg, _userdata);
        }
        else if ((reg->v1.flags & HC_STACK_POINTER) != 0) {
            _stackPointer = new Register(reg, _userdata);
        }
        else if ((reg->v1.flags & HC_MEMORY_POINTER) != 0) {
            _memoryPointers.emplace_back(new Register(reg, _userdata));
        }
    }

    _mainMemory = nullptr;

    for (unsigned i = 0; i < _cpu->v1.num_memory_regions; i++) {
        hc_Memory const* const memory = _cpu->v1.memory_regions[i];

        if ((memory->v1.flags & HC_CPU_ADDRESSABLE) != 0) {
            _mainMemory = new DebugMemory(memory, _userdata);
        }
    }
}

char const* hc::Cpu::getTitle() {
    return _title.c_str();
}

void hc::Cpu::onGameUnloaded() {
    _valid = false;
}

void hc::Cpu::onDraw() {
    if (!_valid) {
        return;
    }

    ImVec2 const available = ImGui::GetContentRegionAvail();
    ImVec2 const spacing = ImGui::GetStyle().ItemSpacing;
    float const width = (available.x - 32.0f - spacing.x * 2) / 2.0f;

    for (unsigned i = 0; i < _cpu->v1.num_registers; i++) {
        hc_Register const* const reg = _cpu->v1.registers[i];
        unsigned const width_bytes = 1U << (reg->v1.flags & HC_SIZE_MASK);
        bool const readonly = reg->v1.set == NULL;
        ImGuiInputTextFlags const readonlyFlag = readonly ? ImGuiInputTextFlags_ReadOnly : 0;

        ImGui::PushItemWidth(32.0f);
        ImGui::LabelText("", "%s", reg->v1.name);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        char label[32];
        char format[32];
        char buffer[64];

        snprintf(label, sizeof(label), "##%uhex", i);
        snprintf(format, sizeof(format), "0x%%0%d" PRIx64, width_bytes * 2);
        snprintf(buffer, sizeof(buffer), format, reg->v1.get(_userdata));
        ImGuiInputTextFlags const flagsHex = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal
                                           | readonlyFlag;

        ImGui::PushItemWidth(width);

        if (ImGui::InputText(label, buffer, sizeof(buffer), flagsHex)) {
            uint64_t value = 0;

            if (!readonly && sscanf(buffer, "0x%" SCNx64, &value) == 1) {
                reg->v1.set(_userdata, value);
            }
        }

        ImGui::PopItemWidth();
        ImGui::SameLine();

        snprintf(label, sizeof(label), "##%udec", i);
        snprintf(buffer, sizeof(buffer), "%" PRIu64, reg->v1.get(_userdata));
        ImGuiInputTextFlags const flagsDec = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal
                                           | readonlyFlag;

        ImGui::PushItemWidth(width);

        if (ImGui::InputText(label, buffer, sizeof(buffer), flagsDec)) {
            uint64_t value = 0;

            if (!readonly && sscanf(buffer, "%" SCNu64, &value) == 1) {
                reg->v1.set(_userdata, value);
            }
        }

        ImGui::PopItemWidth();

        if (reg->v1.bits != NULL) {
            ImGui::Dummy(ImVec2(32.0f, 0.0f));
            ImGui::SameLine();

            uint64_t const value = reg->v1.get(_userdata);
            uint64_t newValue = 0;
            int f = 0;

            for (uint64_t bit = UINT64_C(1) << (width_bytes * 8 - 1); bit != 0 && reg->v1.bits[f] != NULL; bit >>= 1, f++) {
                bool checked = (value & bit) != 0;
                ImGui::Checkbox(reg->v1.bits[f], &checked);
                ImGui::SameLine();

                if (checked) {
                    newValue |= bit;
                }
            }

            if (!readonly) {
                reg->v1.set(_userdata, newValue);
            }

            ImGui::NewLine();
        }
    }

    if (ImGui::Button(ICON_FA_EYE " Disassembly")) {
        _desktop->addView(new Disasm(_desktop, this, _mainMemory, _programCounter), true, true);
    }
}

hc::Disasm::Disasm(Desktop* desktop, Cpu* cpu, Memory* memory, Register* reg)
    : View(desktop)
    , _valid(true)
    , _memory(memory)
    , _register(reg)
{
    _title = ICON_FA_MICROCHIP " ";
    _title += cpu->name();
    _title += " disassembly";
}

char const* hc::Disasm::getTitle() {
    return _title.c_str();
}

void hc::Disasm::onGameUnloaded() {
    _valid = false;
}

void hc::Disasm::onDraw() {
    if (!_valid) {
        return;
    }

    char format[32];
    snprintf(format, sizeof(format), "%%0%u" PRIx64 ":  %%-11s  %%s", _memory->requiredDigits());

    float const lineHeight = ImGui::GetTextLineHeightWithSpacing();

    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar
                                 | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("##scrolling", ImVec2(0.0f, 0.0f), false, flags);

    ImVec2 const regionMax = ImGui::GetContentRegionMax();
    size_t const numItems = static_cast<size_t>(ceil(regionMax.y / lineHeight));

    std::vector<uint64_t> addresses;
    addresses.reserve(numItems + numItems / 2);

    uint64_t const address = _register->get();
    uint64_t addr = address >= numItems * 4 ? address - numItems * 4 : 0;
    size_t addrLine = 0;

    for (size_t i = 0;; i++) {
        addresses.emplace_back(addr);

        if (addr >= address) {
            addrLine = addresses.size();
            break;
        }

        uint8_t length = 0;
        uint8_t cycles = 0;
        char flags[9];
        z80::info(addr, _memory, &length, &cycles, flags);

        addr += length;
    }

    size_t const firstLine = addrLine >= numItems / 2 ? addrLine - numItems / 2 : 0;
    addr = addresses[firstLine];

    for (size_t i = 0; i < numItems; i++) {
        char buffer[64];
        z80::disasm(addr, _memory, buffer, sizeof(buffer));

        if (addr == address) {
            ImVec2 const pos = ImGui::GetCursorScreenPos();
            renderFrame(ImVec2(pos.x, pos.y), ImVec2(pos.x + regionMax.x, pos.y + lineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
        }

        uint8_t length = 0;
        uint8_t cycles = 0;
        char flags[8];
        z80::info(addr, _memory, &length, &cycles, flags);

        char opcodes[12];

        switch (length) {
            case 1: snprintf(opcodes, sizeof(opcodes), "%02x", _memory->peek(addr)); break;
            case 2: snprintf(opcodes, sizeof(opcodes), "%02x %02x", _memory->peek(addr), _memory->peek(addr + 1)); break;

            case 3:
                snprintf(
                    opcodes, sizeof(opcodes),
                    "%02x %02x %02x", _memory->peek(addr), _memory->peek(addr + 1), _memory->peek(addr + 2)
                );

                break;

            case 4:
                snprintf(opcodes, sizeof(opcodes),
                    "%02x %02x %02x %02x",
                    _memory->peek(addr), _memory->peek(addr + 1), _memory->peek(addr + 2), _memory->peek(addr + 3)
                );

                break;
        }

        ImGui::Text(format, addr, opcodes, buffer);

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();

            switch (cycles) {
                case z80::CyclesDjnz: ImGui::Text("13/8 cycles"); break;
                case z80::CyclesCondJr: ImGui::Text("12/7 cycles"); break;
                case z80::CyclesCondRet: ImGui::Text("11/5 cycles"); break;
                case z80::CyclesCondCall: ImGui::Text("17/10 cycles"); break;
                case z80::CyclesBlockTransfer: ImGui::Text("21/16 cycles"); break;
                default: ImGui::Text("%u cycles", cycles); break;
            }

            ImGui::Text(
                "S=%c Z=%c Y=%c H=%c X=%c P/V=%c N=%c C=%c",
                flags[0], flags[1], flags[2], flags[3], flags[4], flags[5], flags[6], flags[7]
            );

            ImGui::EndTooltip();
        }

        addr += length;
    }

    ImGui::EndChild();
}

void hc::Debugger::init() {}

char const* hc::Debugger::getTitle() {
    return ICON_FA_BUG " Debugger";
}

void hc::Debugger::onGameLoaded() {
    static hc_DebuggerIf const templ = {
        1,
        {NULL}
    };

    _debuggerIf = (hc_DebuggerIf*)malloc(sizeof(*_debuggerIf));

    if (_debuggerIf != nullptr) {
        memcpy(static_cast<void*>(_debuggerIf), &templ, sizeof(*_debuggerIf));
        hc_Set setDebugger = (hc_Set)_config->getExtension("hc_set_debuggger");

        if (setDebugger != nullptr) {
            _userdata = setDebugger(_debuggerIf);

            for (unsigned i = 0; i < _debuggerIf->v1.system->v1.num_memory_regions; i++) {
                DebugMemory* memory = new DebugMemory(_debuggerIf->v1.system->v1.memory_regions[i], _userdata);
                _memorySelector->add(hc::handle::allocate(memory));
            }

            for (unsigned i = 0; i < _debuggerIf->v1.system->v1.num_cpus; i++) {
                hc_Cpu const* const cpu = _debuggerIf->v1.system->v1.cpus[i];

                for (unsigned j = 0; j < cpu->v1.num_memory_regions; j++) {
                    hc_Memory const* mem = cpu->v1.memory_regions[i];
                    DebugMemory* memory = new DebugMemory(mem, _userdata);
                    Handle<Memory*> const handle = hc::handle::allocate(memory);
                    _memorySelector->add(handle);
                }
            }
        }
        else {
            free(_debuggerIf);
            _debuggerIf = nullptr;
        }
    }
}

void hc::Debugger::onDraw() {
    if (_debuggerIf == nullptr) {
        return;
    }

    ImGui::Text("%s, interface version %u", _debuggerIf->v1.system->v1.description, _debuggerIf->version);

    static auto const getter = [](void* const data, int idx, char const** const text) -> bool {
        auto const debuggerIf = static_cast<hc_DebuggerIf const*>(data);
        *text = debuggerIf->v1.system->v1.cpus[idx]->v1.description;
        return true;
    };

    int const count = static_cast<int>(_debuggerIf->v1.system->v1.num_cpus);

    ImGui::Combo("##Cpus", &_selectedCpu, getter, _debuggerIf, count);
    ImGui::SameLine();

    ImVec2 const rest = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);

    if (ImGui::Button(ICON_FA_EYE " View", rest)) {
        _desktop->addView(new Cpu(_desktop, _debuggerIf->v1.system->v1.cpus[_selectedCpu], _userdata), true, true);
    }
}

void hc::Debugger::onGameUnloaded() {
    _debuggerIf = nullptr;
    _selectedCpu = 0;
}
