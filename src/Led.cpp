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

    if (ImGui::Begin(ICON_FA_LIGHTBULB_O " Leds")) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (size_t i = 0; i < count; i++) {
            ImVec2 const pos = ImGui::GetCursorScreenPos();
            ImVec2 const center(pos.x + 5.0f, pos.y + 5.0f);
            drawList->AddCircleFilled(center, 8.0f, _states[i] ? 0xff0000ff : 0xff000080);
            ImGui::Dummy(ImVec2(20.0f, 0.0f));
        }
    }

    ImGui::End();
}

void hc::Led::setState(int led, int state) {
    _states.resize(led + 1);
    _states[led] = state;
}
