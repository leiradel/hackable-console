#include "Led.h"

#include <IconsFontAwesome4.h>

#define TAG "[LED] "

hc::Led::Led() : _logger(nullptr) {}

void hc::Led::init(hc::Logger* logger) {
    _logger = logger;
}

char const* hc::Led::getName() {
    return "hc::Led built-in led plugin";
}

char const* hc::Led::getVersion() {
    return "0.0.0";
}

char const* hc::Led::getLicense() {
    return "MIT";
}

char const* hc::Led::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Led::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Led::onStarted() {}

void hc::Led::onConsoleLoaded() {}

void hc::Led::onGameLoaded() {}

void hc::Led::onGamePaused() {}

void hc::Led::onGameResumed() {}

void hc::Led::onGameReset() {
    size_t const count = _states.size();

    for (size_t i = 0; i < count; i++) {
        _states[i] = 0;
    }
}

void hc::Led::onFrame() {}

void hc::Led::onDraw() {
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

void hc::Led::onGameUnloaded() {
    _states.clear();
}

void hc::Led::onConsoleUnloaded() {}

void hc::Led::onQuit() {}

void hc::Led::setState(int led, int state) {
    _logger->info(TAG "Set LED %d to %s", led, state ? "on" : "off");
    _states.resize(led + 1);
    _states[led] = state;
}
