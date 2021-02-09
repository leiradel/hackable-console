#include "Disasm.h"
#include "Memory.h"
#include "Debugger.h"
#include "cpus/Z80.h"

#include <imgui.h>

#include <math.h>
#include <inttypes.h>
#include <vector>
#include <string>

static unsigned maximumDigits(hc::Memory* const memory) {
    unsigned count = 0;

    for (uint64_t maximum = memory->base() + memory->size() - 1; maximum > 0; maximum >>= 4) {
        count++;
    }

    return count;
}

static void renderFrame(ImVec2 const min, ImVec2 const max, ImU32 const color) {
    ImDrawList* const draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(min, max, color, false);
}

char const* hc::Disasm::getTitle() {
    return "Disasm";
}

void hc::Disasm::onDraw() {
    Memory* const memory = hc::handle::translate(_memory);
    Register* const reg = hc::handle::translate(_register);

    if (memory == nullptr || reg == nullptr) {
        return;
    }

    unsigned const addressDigitCount = maximumDigits(memory);

    char format[16];
    snprintf(format, sizeof(format), "%%0%u" PRIx64 ":  %%s", addressDigitCount);

    float const lineHeight = ImGui::GetTextLineHeightWithSpacing();

    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar
                                 | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("##scrolling", ImVec2(0.0f, 0.0f), false, flags);

    ImVec2 const regionMax = ImGui::GetContentRegionMax();
    size_t const numItems = static_cast<size_t>(ceil(regionMax.y / lineHeight));

    std::vector<uint64_t> addresses;
    addresses.reserve(numItems + numItems / 2);

    uint64_t const address = reg->get();
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
        z80::FlagState flags[8];
        z80::info(addr, memory, &length, &cycles, flags);

        addr += length;
    }

    size_t const firstLine = addrLine >= numItems / 2 ? addrLine - numItems / 2 : 0;
    addr = addresses[firstLine];

    for (size_t i = 0; i < numItems; i++) {
        char buffer[64];
        z80::disasm(addr, memory, buffer, sizeof(buffer));

        if (addr == address) {
            ImVec2 const pos = ImGui::GetCursorScreenPos();
            renderFrame(ImVec2(pos.x, pos.y), ImVec2(pos.x + regionMax.x, pos.y + lineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
        }

        ImGui::Text(format, addr, buffer);

        uint8_t length = 0;
        uint8_t cycles = 0;
        z80::FlagState flags[8];
        z80::info(addr, memory, &length, &cycles, flags);

        addr += length;
    }

    ImGui::EndChild();
}
