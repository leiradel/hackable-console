#include "Cpu.h"
#include "cpus/Z80.h"
#include "cpus/M6502.h"

#include <IconsFontAwesome4.h>
#include <imgui.h>
#include <imguial_button.h>

#include <inttypes.h>
#include <atomic>

static void renderFrame(ImVec2 const min, ImVec2 const max, ImU32 const color) {
    ImDrawList* const draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, color, false);
}

hc::Cpu* hc::Cpu::create(Desktop* desktop, hc_Cpu const* cpu, void* userdata) {
    switch (cpu->v1.type) {
        case HC_CPU_Z80: return new Z80(desktop, cpu, userdata);
        case HC_CPU_6502: return new M6502(desktop, cpu, userdata);
    }

    return nullptr;
}

hc::Cpu::Cpu(Desktop* desktop, hc_Cpu const* cpu, void* userdata) : View(desktop), _cpu(cpu), _userdata(userdata), _valid(true) {
    _title = ICON_FA_MICROCHIP " ";
    _title += _cpu->v1.description;

    _memory = new DebugMemory(_cpu->v1.memory_region, _userdata);
}

void hc::Cpu::drawRegister(unsigned const reg, char const* const name, unsigned const width, bool const highlight) {
    ImVec2 const available = ImGui::GetContentRegionAvail();
    ImVec2 const spacing = ImGui::GetStyle().ItemSpacing;
    float const inputWidth = (available.x - 32.0f - spacing.x * 2) / 2.0f;
    float const lineHeight = ImGui::GetTextLineHeightWithSpacing();

    if (highlight) {
        ImVec2 const pos = ImGui::GetCursorScreenPos();
        renderFrame(ImVec2(pos.x, pos.y), ImVec2(pos.x + 32.0f, pos.y + lineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
    }

    ImGui::PushItemWidth(32.0f);
    ImGui::LabelText("", "%s", name);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    char label[32];
    char format[32];
    char buffer[64];

    snprintf(label, sizeof(label), "##%uhex", reg);
    snprintf(format, sizeof(format), "0x%%0%d" PRIx64, (width + 3) / 4);
    snprintf(buffer, sizeof(buffer), format, _cpu->v1.get_register(_userdata, reg));
    ImGuiInputTextFlags const flagsHex = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsHexadecimal;

    ImGui::PushItemWidth(inputWidth);

    if (ImGui::InputText(label, buffer, sizeof(buffer), flagsHex)) {
        uint64_t value = 0;

        if (sscanf(buffer, "0x%" SCNx64, &value) == 1) {
            _cpu->v1.set_register(_userdata, reg, value);
        }
    }

    ImGui::PopItemWidth();
    ImGui::SameLine();

    snprintf(label, sizeof(label), "##%udec", reg);
    snprintf(buffer, sizeof(buffer), "%" PRIu64, _cpu->v1.get_register(_userdata, reg));
    ImGuiInputTextFlags const flagsDec = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal;

    ImGui::PushItemWidth(inputWidth);

    if (ImGui::InputText(label, buffer, sizeof(buffer), flagsDec)) {
        uint64_t value = 0;

        if (sscanf(buffer, "%" SCNu64, &value) == 1) {
            _cpu->v1.set_register(_userdata, reg, value);
        }
    }

    ImGui::PopItemWidth();
}

void hc::Cpu::drawFlags(unsigned reg, char const* name, char const* const* flags, unsigned width, bool highlight) {
    ImGui::Dummy(ImVec2(32.0f, 0.0f));
    ImGui::SameLine();

    uint64_t const value = _cpu->v1.get_register(_userdata, reg);
    uint64_t newValue = 0;
    int f = 0;

    for (uint64_t bit = UINT64_C(1) << (width - 1); bit != 0; bit >>= 1, f++) {
        bool checked = (value & bit) != 0;
        ImGui::Checkbox(flags[f], &checked);
        ImGui::SameLine();

        if (checked) {
            newValue |= bit;
        }
    }

    _cpu->v1.set_register(_userdata, reg, value);
    ImGui::NewLine();
}

char const* hc::Cpu::getTitle() {
    return _title.c_str();
}

void hc::Cpu::onGameUnloaded() {
    _valid = false;
}
