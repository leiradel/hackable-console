#include "Disasm.h"
#include "Cpu.h"
#include "Memory.h"
#include "Debugger.h"

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

    float const lineHeight = ImGui::GetTextLineHeight();

    ImGuiWindowFlags const flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollbar
                                 | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("##scrolling", ImVec2(0.0f, 0.0f), false, flags);

    ImVec2 const regionMax = ImGui::GetContentRegionMax();
    size_t const numItems = static_cast<size_t>(ceil(regionMax.y / lineHeight));

    struct Line {
        Line(uint64_t addr, char const* dis) : address(addr), disasm(dis) {}
        uint64_t address;
        std::string disasm;
    };

    std::vector<Line> lines;
    lines.reserve(numItems + numItems / 2);

    uint64_t const address = reg->get();
    uint64_t addr = address >= numItems * 4 ? address - numItems * 4 : address;
    size_t addrLine = 0;

    for (size_t i = 0;; i++) {
        auto const peek = [](uint64_t const address, void* const userdata) -> uint8_t {
            Memory* const memory = static_cast<Memory*>(userdata);
            return memory->peek(address);
        };

        char buffer[64];
        unsigned const next = disasm_z80(addr, peek, memory, buffer, sizeof(buffer));

        lines.emplace_back(addr, buffer);

        if (addr == address) {
            addrLine = i;
            addr = next;
            break;
        }

        addr = next;
    }

    for (size_t i = 0; i < numItems / 2; i++) {
        auto const peek = [](uint64_t const address, void* const userdata) -> uint8_t {
            Memory* const memory = static_cast<Memory*>(userdata);
            return memory->peek(address);
        };

        char buffer[64];
        unsigned const next = disasm_z80(addr, peek, memory, buffer, sizeof(buffer));

        lines.emplace_back(addr, buffer);
        addr = next;
    }

    size_t const firstLine = addrLine >= numItems / 2 ? addrLine - numItems / 2 : 0;

    for (size_t i = 0; i < numItems; i++) {
        Line const& line = lines[i + firstLine];

        if (line.address == address) {
            ImVec2 const pos = ImGui::GetCursorScreenPos();
            renderFrame(ImVec2(pos.x, pos.y), ImVec2(pos.x + regionMax.x, pos.y + lineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
        }

        ImGui::Text(format, line.address, line.disasm.c_str());
    }

    ImGui::EndChild();
}
