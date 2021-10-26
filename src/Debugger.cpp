#include "Debugger.h"
#include "cpus/Z80.h"

#include <IconsFontAwesome4.h>
#include <imgui.h>
#include <imguial_button.h>

#include <inttypes.h>
#include <math.h>

#include <atomic>

#define TAG "DGB "

static void renderFrame(ImVec2 const min, ImVec2 const max, ImU32 const color) {
    ImDrawList* const draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, color, false);
}

hc::Disasm::Disasm(Desktop* desktop, Cpu* cpu, Memory* memory, unsigned reg)
    : View(desktop)
    , _valid(true)
    , _cpu(cpu)
    , _memory(memory)
    , _register(reg)
{
    static std::atomic<unsigned> counter;

    _title = ICON_FA_CODE " ";
    _title += _cpu->name();
    _title += " Disassembly##";
    _title += counter++;
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

    ImGuiWindowFlags const flagsFollow = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar
                                       | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("##scrolling", ImVec2(0.0f, 0.0f), false, flagsFollow);

    ImVec2 const regionMax = ImGui::GetContentRegionMax();
    size_t const numItems = static_cast<size_t>(ceil(regionMax.y / lineHeight));

    std::vector<uint64_t> addresses;
    addresses.reserve(numItems + numItems / 2);

    uint64_t const address = _cpu->getRegister(_register);
    uint64_t addr = address >= numItems * 4 ? address - numItems * 4 : 0;
    size_t addrLine = 0;

    for (size_t i = 0;; i++) {
        addresses.emplace_back(addr);

        if (addr >= address) {
            addrLine = addresses.size();
            break;
        }

        addr += _cpu->instructionLength(addr, _memory);
    }

    size_t const firstLine = addrLine >= numItems / 2 ? addrLine - numItems / 2 : 0;
    addr = addresses[firstLine];

    for (size_t i = 0; i < numItems; i++) {
        if (addr == address) {
            ImVec2 const pos = ImGui::GetCursorScreenPos();
            renderFrame(ImVec2(pos.x, pos.y), ImVec2(pos.x + regionMax.x, pos.y + lineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
        }

        char opcodes[12];

        uint64_t const length = _cpu->instructionLength(addr, _memory);

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

        char buffer[64], tooltip[64];
        _cpu->disasm(addr, _memory, buffer, sizeof(buffer), tooltip, sizeof(tooltip));

        ImGui::Text(format, addr, opcodes, buffer);

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", tooltip);
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
        0,
        {NULL}
    };

    hc_Set setDebugger = (hc_Set)_config->getExtension("hc_set_debugger");

    if (setDebugger != nullptr) {
        _debuggerIf = (hc_DebuggerIf*)malloc(sizeof(*_debuggerIf));

        if (_debuggerIf != nullptr) {
            memcpy(static_cast<void*>(_debuggerIf), &templ, sizeof(*_debuggerIf));
            _userdata = setDebugger(_debuggerIf);

            for (unsigned i = 0; i < _debuggerIf->v1.system->v1.num_memory_regions; i++) {
                DebugMemory* memory = new DebugMemory(_debuggerIf->v1.system->v1.memory_regions[i], _userdata);
                _memorySelector->add(memory);
            }

            for (unsigned i = 0; i < _debuggerIf->v1.system->v1.num_cpus; i++) {
                hc_Cpu const* const cpu = _debuggerIf->v1.system->v1.cpus[i];

                if (HC_CPU_API_VERSION(_debuggerIf->v1.system->v1.cpus[i]->v1.type) <= HC_API_VERSION) {
                    _cpus.emplace_back(cpu);

                    DebugMemory* memory = new DebugMemory(cpu->v1.memory_region, _userdata);
                    _memorySelector->add(memory);
                }
                else {
                    _desktop->warn(TAG "Unsupported CPU \"%s\"", cpu->v1.description);
                }
            }
        }
    }
}

void hc::Debugger::onDraw() {
    if (_debuggerIf == nullptr) {
        return;
    }

    ImGui::Text("%s, interface version %u", _debuggerIf->v1.system->v1.description, _debuggerIf->core_api_version);

    static auto const getter = [](void* const data, int idx, char const** const text) -> bool {
        auto const cpus = static_cast<std::vector<hc_Cpu const*> const*>(data);
        *text = (*cpus)[idx]->v1.description;
        return true;
    };

    int const count = static_cast<int>(_debuggerIf->v1.system->v1.num_cpus);

    ImGui::Combo("##Cpus", &_selectedCpu, getter, &_cpus, count);
    ImGui::SameLine();

    ImVec2 const rest = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);

    if (ImGui::Button(ICON_FA_EYE " View", rest)) {
        Cpu* const cpu = Cpu::create(_desktop, _debuggerIf->v1.system->v1.cpus[_selectedCpu], _userdata);
        _desktop->addView(cpu, false, true);
    }
}

void hc::Debugger::onGameUnloaded() {
    _debuggerIf = nullptr;
    _cpus.clear();
    _selectedCpu = 0;
}
