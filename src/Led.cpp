#include "Led.h"

#include <IconsFontAwesome4.h>

bool hc::Led::init(hc::Logger* logger) {
    _logger = logger;
    return true;
}

void hc::Led::destroy() {}

void hc::Led::reset() {
    _states.clear();
}

void hc::Led::draw() {
    size_t const count = _states.size();

    if (count == 0) {
        return;
    }

    static ImColor const on = ImColor(IM_COL32(255, 0, 0, 255));
    static ImColor const off = ImColor(IM_COL32(64, 64, 64, 255));

    if (ImGui::Begin(ICON_FA_LIGHTBULB_O " Leds")) {
        for (size_t i = 0; i < count; i++) {
            ImGui::PushStyleColor(ImGuiCol_Text, _states[i] ? on.Value : off.Value);
            ImGui::Text(ICON_FA_CIRCLE);
            ImGui::PopStyleColor(1);
            ImGui::SameLine();
        }
    }

    ImGui::End();
}

void hc::Led::setState(int led, int state) {
    _states.resize(led + 1);
    _states[led] = state;
}
