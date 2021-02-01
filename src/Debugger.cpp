#include "Debugger.h"

#include <IconsFontAwesome4.h>
#include <imgui.h>

#include <inttypes.h>

void hc::Debugger::init(Config* config) {
    _config = config;
}

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
            setDebugger(_debuggerIf);
        }
    }
}

void hc::Debugger::onDraw() {
    if (_debuggerIf == nullptr) {
        return;
    }

    ImGui::Text("%s", _debuggerIf->v1.system->v1.description);

    static auto const getter = [](void* data, int idx, char const** text) -> bool {
        auto const debuggerIf = static_cast<hc_DebuggerIf const*>(data);
        *text = debuggerIf->v1.system->v1.cpus[idx]->v1.description;
        return true;
    };

    ImGui::Combo("CPU", &_selectedCpu, getter, _debuggerIf, _debuggerIf->v1.system->v1.num_cpus);

    if (_selectedCpu >= 0 && static_cast<unsigned>(_selectedCpu) < _debuggerIf->v1.system->v1.num_cpus) {
        ImVec2 const available = ImGui::GetContentRegionAvail();
        ImVec2 const spacing = ImGui::GetStyle().ItemSpacing;
        float const width = (available.x - 32.0f - spacing.x * 2) / 2.0f;

        hc_Cpu const* cpu = _debuggerIf->v1.system->v1.cpus[_selectedCpu];

        for (unsigned i = 0; i < cpu->v1.num_registers; i++) {
            hc_Register const* reg = cpu->v1.registers[i];

            ImGui::PushItemWidth(32.0f);
            ImGui::LabelText("", "%s", reg->v1.name);
            ImGui::PopItemWidth();
            ImGui::SameLine();

            if (reg->v1.bits == NULL) {
                char label[32];
                char format[32];
                char buffer[64];

                snprintf(label, sizeof(label), "##%uhex", i);
                snprintf(format, sizeof(format), "0x%%0%d" PRIx64, reg->v1.width_bytes * 2);
                snprintf(buffer, sizeof(buffer), format, reg->v1.get(_debuggerIf, i));
                ImGuiInputTextFlags const flagsHex = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal;

                ImGui::PushItemWidth(width);

                if (ImGui::InputText(label, buffer, sizeof(buffer), flagsHex)) {
                    uint64_t value = 0;

                    if (sscanf(buffer, "0x%" SCNx64, &value) == 1) {
                        reg->v1.set(_debuggerIf, i, value);
                    }
                }

                ImGui::PopItemWidth();
                ImGui::SameLine();

                snprintf(label, sizeof(label), "##%udec", i);
                snprintf(buffer, sizeof(buffer), "%" PRIu64, reg->v1.get(_debuggerIf, i));
                ImGuiInputTextFlags const flagsDec = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal;

                ImGui::PushItemWidth(width);

                if (ImGui::InputText(label, buffer, sizeof(buffer), flagsDec)) {
                    uint64_t value = 0;

                    if (sscanf(buffer, "%" SCNu64, &value) == 1) {
                        reg->v1.set(_debuggerIf, i, value);
                    }
                }

                ImGui::PopItemWidth();
            }
            else {
                uint64_t const value = reg->v1.get(_debuggerIf, i);
                uint64_t newValue = 0;
                int f = 0;

                for (uint64_t bit = UINT64_C(1) << (reg->v1.width_bytes * 8 - 1); bit != 0; bit >>= 1, f++) {
                    bool checked = (value & bit) != 0;
                    ImGui::Checkbox(reg->v1.bits[f], &checked);
                    ImGui::SameLine();

                    if (checked) {
                        newValue |= bit;
                    }
                }

                reg->v1.set(_debuggerIf, i, newValue);
                ImGui::NewLine();
            }
        }
    }
}

void hc::Debugger::onGameUnloaded() {
    _debuggerIf = nullptr;
    _selectedCpu = 0;
}
