#include "Led.h"

#include <IconsFontAwesome4.h>

extern "C" {
    #include "lauxlib.h"
}

#define TAG "[LED] "

hc::Led::Led(Desktop* desktop) : View(desktop), _logger(nullptr) {}

void hc::Led::init(hc::Logger* const logger) {
    _logger = logger;
}

char const* hc::Led::getTitle() {
    return ICON_FA_LIGHTBULB_O " Leds";
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
    static ImColor const on = ImColor(IM_COL32(255, 0, 0, 255));
    static ImColor const off = ImColor(IM_COL32(64, 64, 64, 255));

    size_t const count = _states.size();

    for (size_t i = 0; i < count; i++) {
        ImGui::PushStyleColor(ImGuiCol_Text, _states[i] ? on.Value : off.Value);
        ImGui::Text(ICON_FA_CIRCLE);
        ImGui::PopStyleColor(1);
        ImGui::SameLine();
    }
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

int hc::Led::push(lua_State* const L) {
    auto const self = static_cast<Led**>(lua_newuserdata(L, sizeof(Led*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::Led")) {
        static luaL_Reg const methods[] = {
            {"setState", l_setState},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Led* hc::Led::check(lua_State* const L, int const index) {
    return *static_cast<Led**>(luaL_checkudata(L, index, "hc::Led"));
}

int hc::Led::l_setState(lua_State* const L) {
    auto const self = check(L, 1);
    lua_Integer const led = luaL_checkinteger(L, 2);
    lua_Integer const state = lua_toboolean(L, 3);

    if (led < 1 || led > 16) {
        // Be reasonable
        return luaL_error(L, "invalid led index %I", led);
    }

    self->setState(led, state);
    return 0;
}
