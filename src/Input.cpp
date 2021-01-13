#include "Input.h"

#include <IconsFontAwesome4.h>

#define NONE_ID -1
#define KEYBOARD_ID -2
#define TAG "[INP] "

hc::Input::Input() : _logger(nullptr), _frontend(nullptr) {}

void hc::Input::init(Logger* logger, lrcpp::Frontend* frontend) {
    _logger = logger;
    _frontend = frontend;

    // Add the none controller
    Pad none;
    none.id = NONE_ID;
    none.controller = NULL;
    none.controllerName = "None";
    none.joystick = NULL;
    none.joystickName = "None";
    none.lastDir[0] = none.lastDir[1] = none.lastDir[2] =
    none.lastDir[3] = none.lastDir[4] = none.lastDir[5] = -1;
    memset(none.state, 0, sizeof(none.state));
    memset(none.analogs, 0, sizeof(none.analogs));
    none.sensitivity = 0.5f;
    none.digital = false;

    _pads.emplace_back(none);
    _logger->info(TAG "Controller %s (%s) created", none.controllerName.c_str(), none.joystickName.c_str());

    // Add the keyboard controller
    Pad virtualPad;
    virtualPad.id = KEYBOARD_ID;
    virtualPad.controller = NULL;
    virtualPad.controllerName = "Virtual";
    virtualPad.joystick = NULL;
    virtualPad.joystickName = "Virtual";
    virtualPad.lastDir[0] = virtualPad.lastDir[1] = virtualPad.lastDir[2] =
    virtualPad.lastDir[3] = virtualPad.lastDir[4] = virtualPad.lastDir[5] = -1;
    memset(virtualPad.state, 0, sizeof(virtualPad.state));
    memset(virtualPad.analogs, 0, sizeof(virtualPad.analogs));
    virtualPad.sensitivity = 0.5f;
    virtualPad.digital = false;

    _pads.emplace_back(virtualPad);
    _logger->info(TAG "Controller %s (%s) created", virtualPad.controllerName.c_str(), virtualPad.joystickName.c_str());
}

void hc::Input::processEvent(SDL_Event const* event) {
    switch (event->type) {
        case SDL_CONTROLLERDEVICEADDED:
            addController(event);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            removeController(event);
            break;

        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERBUTTONDOWN:
            controllerButton(event);
            break;

        case SDL_CONTROLLERAXISMOTION:
            controllerAxis(event);
            break;

        case SDL_KEYUP:
        case SDL_KEYDOWN:
            keyboard(event);
            break;
    }
}

char const* hc::Input::getName() {
    return "hc::Input built-in input plugin";
}

char const* hc::Input::getVersion() {
    return "0.0.0";
}

char const* hc::Input::getLicense() {
    return "MIT";
}

char const* hc::Input::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Input::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Input::onStarted() {}

void hc::Input::onConsoleLoaded() {
    for (size_t port = 0; port < MaxPorts; port++) {
        // All ports start disconnected
        _ports[port].selectedType = 0;
        _ports[port].selectedDevice = 0;
        _ports[port].type = RETRO_DEVICE_NONE;
        _ports[port].padId = NONE_ID;

        /**
         * Add None and RetroPad controllers to the ports, RetroArch always has
         * these controllers.
         */
        _controllerTypes[port].insert(_controllerTypes[port].begin(), ControllerInfo("RetroPad", RETRO_DEVICE_JOYPAD));
        _controllerTypes[port].insert(_controllerTypes[port].begin(), ControllerInfo("None", RETRO_DEVICE_NONE));
    }

    // TODO auto-assign controllers to ports here? Do it in the Lua console script?
}

void hc::Input::onGameLoaded() {}
void hc::Input::onGamePaused() {}
void hc::Input::onGameResumed() {}
void hc::Input::onGameReset() {}

void hc::Input::onFrame() {
    for (unsigned i = 0; i < RETROK_LAST; i++) {
        if (_keyState[i] > 0) {
            _keyState[i]--;
        }
    }
}

void hc::Input::onDraw() {
    static char const* const portNames[MaxPorts] = {"Port 1", "Port 2", "Port 3", "Port 4"};

    if (!ImGui::Begin(ICON_FA_GAMEPAD " Input")) {
        return;
    }

    if (ImGui::BeginTabBar("##ports")) {
        for (size_t port = 0; port < MaxPorts; port++) {
            if (ImGui::BeginTabItem(portNames[port])) {
                static auto const getter = [](void* data, int idx, char const** text) -> bool {
                    auto const types = static_cast<std::vector<ControllerInfo> const*>(data);
                    *text = (*types)[idx].desc.c_str();
                    return true;
                };

                auto& types = _controllerTypes[port];
                int selected = _ports[port].selectedType;

                ImGui::Combo("Type", &selected, getter, &types, static_cast<int>(types.size()));

                if (selected != _ports[port].selectedType) {
                    _ports[port].selectedType = selected;
                    _ports[port].selectedDevice = 0;
                    _ports[port].type = _controllerTypes[port][selected].id & RETRO_DEVICE_MASK;
                    _ports[port].padId = NONE_ID;

                    _frontend->setControllerPortDevice(port, _controllerTypes[port][selected].id);
                }

                if (_ports[port].type == RETRO_DEVICE_JOYPAD) {
                    static auto const getter = [](void* data, int idx, char const** text) -> bool {
                        auto const pads = static_cast<std::vector<Pad> const*>(data);
                        *text = (*pads)[idx].controllerName.c_str();
                        return true;
                    };

                    int selected = _ports[port].selectedDevice;
                    ImGui::Combo("Device", &selected, getter, &_pads, static_cast<int>(_pads.size()));

                    if (selected != _ports[port].selectedDevice) {
                        _ports[port].selectedDevice = selected;
                        _ports[port].padId = _pads[selected].id;
                    }

                    if (_ports[port].padId != NONE_ID) {
                        size_t const count = _pads.size();

                        for (size_t i = 0; i < count; i++) {
                            if (_pads[i].id == _ports[port].padId) {
                                drawPad(_pads[i]);
                            }
                        }
                    }
                }
                else if (_ports[port].type == RETRO_DEVICE_KEYBOARD) {
                    drawKeyboard();
                }

                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void hc::Input::onGameUnloaded() {}

void hc::Input::onConsoleUnloaded() {
    for (size_t port = 0; port < MaxPorts; port++) {
        // Disconnect all ports
        _ports[port].selectedType = 0;
        _ports[port].selectedDevice = 0;
        _ports[port].type = RETRO_DEVICE_NONE;
        _ports[port].padId = NONE_ID;

        // Erase the controller types
        _controllerTypes[port].clear();
    }
}

void hc::Input::onQuit() {}

bool hc::Input::setInputDescriptors(retro_input_descriptor const* descriptors) {
    // We just log the descriptors, the information is currently discarded
    _logger->info(TAG "Setting input descriptors");
    _logger->info(TAG "    port device index id description");

    for (size_t i = 0; descriptors[i].description != nullptr; i++) {
        /**
         * At least the Frodo core doesn't properly terminate the input
         * descriptor list with a zeroed entry, we do our best to avoid a crash
         * here.
         */
        if ((descriptors[i].device & RETRO_DEVICE_MASK) > RETRO_DEVICE_POINTER) {
            break;
        }

        if (descriptors[i].id > RETRO_DEVICE_ID_LIGHTGUN_RELOAD) {
            break;
        }

        retro_input_descriptor const* desc = descriptors + i;
        _logger->info(TAG "    %4u %6u %5u %2u %s", desc->port, desc->device, desc->index, desc->id, desc->description);
    }

    return true;
}

bool hc::Input::setKeyboardCallback(retro_keyboard_callback const* callback) {
    (void)callback;
    return false;
}

bool hc::Input::getInputDeviceCapabilities(uint64_t* capabilities) {
    (void)capabilities;
    return false;
}

bool hc::Input::setControllerInfo(retro_controller_info const* info) {
    static char const* const deviceNames[] = {"none", "joypad", "mouse", "keyboard", "lightgun", "analog", "pointer"};

    _logger->info(TAG "Setting controller info");
    _logger->info(TAG "    port id type     description");

    size_t portCount = 0;

    for (; info[portCount].types != nullptr; portCount++) /* nothing */;

    if (portCount > MaxPorts) {
        portCount = MaxPorts;
    }

    for (size_t port = 0; port < portCount; port++) {
        _controllerTypes[port].reserve(info[port].num_types + 2);

        for (unsigned device = 0; device < info[port].num_types; device++) {
            retro_controller_description const* type = info[port].types + device;

            unsigned const deviceType = type->id & RETRO_DEVICE_MASK;
            char const* deviceName = deviceType < sizeof(deviceNames) / sizeof(deviceNames[0]) ? deviceNames[deviceType] : "?";
            _logger->info(TAG "    %4zu %2u %-8s %s", port + 1, type->id >> RETRO_DEVICE_TYPE_SHIFT, deviceName, type->desc);

            if (type->id != RETRO_DEVICE_NONE) {
                _controllerTypes[port].emplace_back(type->desc, type->id);
            }
            else {
                _logger->warn(TAG "Not adding RETRO_DEVICE_NONE as it'll be added later");
            }
        }
    }

    return true;
}

bool hc::Input::getInputBitmasks(bool* supports) {
    *supports = false;
    return false;
}

int16_t hc::Input::state(unsigned portIndex, unsigned deviceId, unsigned index, unsigned id) {
    if (portIndex >= MaxPorts) {
        return 0;
    }

    unsigned const base = deviceId & RETRO_DEVICE_MASK;
    Port const& port = _ports[portIndex];

    if (port.type != base) {
        return 0;
    }

    switch (base) {
        case RETRO_DEVICE_JOYPAD: {
            size_t const count = _pads.size();

            for (size_t i = 0; i < count; i++) {
                if (port.padId == _pads[i].id) {
                    return _pads[i].state[id];
                }
            }

            break;
        }

        case RETRO_DEVICE_ANALOG: {
            size_t const count = _pads.size();

            for (size_t i = 0; i < count; i++) {
                if (port.padId == _pads[i].id) {
                    return id == RETRO_DEVICE_ID_ANALOG_X ? _pads[i].analogs[index].x : _pads[i].analogs[index].y;
                }
            }

            break;
        }

        case RETRO_DEVICE_KEYBOARD: {
            return id < RETROK_LAST ? (_keyState[id] != 0 ? 32767 : 0) : 0;
        }
    }

    return 0;
}

void hc::Input::poll() {}

void hc::Input::addController(int which) {
    if (!SDL_IsGameController(which)) {
        return;
    }

    Pad pad;
    pad.controller = SDL_GameControllerOpen(which);

    if (pad.controller == NULL) {
        _logger->error(TAG "Error opening the controller: %s", SDL_GetError());
        return;
    }

    pad.joystick = SDL_GameControllerGetJoystick(pad.controller);

    if (pad.joystick == NULL) {
        _logger->error(TAG "Error getting the joystick: %s", SDL_GetError());
        SDL_GameControllerClose(pad.controller);
        return;
    }

    pad.id = SDL_JoystickInstanceID(pad.joystick);

    pad.controllerName = SDL_GameControllerName(pad.controller);
    pad.joystickName = SDL_JoystickName(pad.joystick);
    pad.lastDir[0] = pad.lastDir[1] = pad.lastDir[2] =
    pad.lastDir[3] = pad.lastDir[4] = pad.lastDir[5] = -1;
    pad.sensitivity = 0.5f;
    pad.digital = false;
    memset(pad.state, 0, sizeof(pad.state));
    memset(pad.analogs, 0, sizeof(pad.analogs));

    _pads.emplace_back(pad);
    _logger->info(TAG "Controller %s (%s) added", pad.controllerName.c_str(), pad.joystickName.c_str());
}

void hc::Input::drawPad(Pad& pad) {
    static auto const show = [](char const* name, float const width, bool const active) {
        static ImU32 const pressed = IM_COL32(128, 128, 128, 255);
        static ImU32 const unpressed = IM_COL32(64, 64, 64, 255);

        ImVec2 const size(width, 20.0f);
        ImU32 const color = active != 0 ? pressed : unpressed;

        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

        ImGui::Button(name, size);
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
    };

    static auto const skip = [](float const width) {
        ImGui::Dummy(ImVec2(width, 20.0f));
        ImGui::SameLine();
    };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
    ImGui::Columns(2);

    {
        show("L3", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_L3] != 0);
        show("L2", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_L2] != 0);
        show("L", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_L] != 0);
        show("R", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_R] != 0);
        show("R2", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_R2] != 0);
        show("R3", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_R3] != 0);

        ImGui::NewLine();

        show("Select", 62.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_SELECT] != 0);
        show("Start", 62.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_START] != 0);

        ImGui::NewLine();

        skip(20.0f);
        ImGui::SameLine();
        show("U", 21.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_UP] != 0);
        skip(39.0f);
        show("X", 21.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_X] != 0);

        ImGui::NewLine();

        skip(10.0f);
        show("L", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_LEFT] != 0);
        show("R", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_RIGHT] != 0);
        skip(19.0f);
        show("Y", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_Y] != 0);
        show("A", 20.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_A] != 0);

        ImGui::NewLine();

        skip(20.0f);
        show("D", 21.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_DOWN] != 0);
        skip(39.0f);
        show("B", 21.0f, pad.state[RETRO_DEVICE_ID_JOYPAD_B] != 0);
    }

    ImGui::PopStyleVar();
    ImGui::NextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));

    {
        ImGui::Checkbox("Analog to digital", &pad.digital);
        ImGui::SliderFloat("##Sensitivity", &pad.sensitivity, 0.0f, 1.0f, "Sensitivity %.3f");
        ImGui::Text("Left:     %6d / %6d", pad.analogs[0].x, pad.analogs[0].y);
        ImGui::Text("Right:    %6d / %6d", pad.analogs[1].x, pad.analogs[1].y);
        ImGui::Text("Triggers: %6d / %6d", pad.analogs[2].x, pad.analogs[2].y);
    }

    ImGui::PopStyleVar();
    ImGui::Columns(1);
}

void hc::Input::drawKeyboard() {
    typedef char const* Key;

    static Key const     row0[] = {"esc", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", nullptr};
    static uint8_t const siz0[] = {25,    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,    23,    23};

    static unsigned const cod0[] = {
        RETROK_ESCAPE, RETROK_F1, RETROK_F2, RETROK_F3, RETROK_F4, RETROK_F5, RETROK_F6,
        RETROK_F7, RETROK_F8, RETROK_F9, RETROK_F10, RETROK_F11, RETROK_F12
    };

    static Key const     row1[] = {"`",  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "bs", nullptr};
    static uint8_t const siz1[] = {20,   20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  40};

    static unsigned const cod1[] = {
        RETROK_BACKQUOTE, RETROK_1, RETROK_2, RETROK_3, RETROK_4, RETROK_5, RETROK_6, RETROK_7,
        RETROK_8, RETROK_9, RETROK_0, RETROK_MINUS, RETROK_EQUALS, RETROK_BACKSPACE
    };

    static Key const     row2[] = {"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\", nullptr};
    static uint8_t const siz2[] = {30,    20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  30};

    static unsigned const cod2[] = {
        RETROK_TAB, RETROK_q, RETROK_w, RETROK_e, RETROK_r, RETROK_t, RETROK_y, RETROK_u,
        RETROK_i, RETROK_o, RETROK_p, RETROK_LEFTBRACKET, RETROK_RIGHTBRACKET, RETROK_BACKSLASH
    };

    static Key const     row3[] = {"caps", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'",  "enter", nullptr};
    static uint8_t const siz3[] = {40,     20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,   41};

    static unsigned const cod3[] = {
        RETROK_CAPSLOCK, RETROK_a, RETROK_s, RETROK_d, RETROK_f, RETROK_g, RETROK_h,
        RETROK_j, RETROK_k, RETROK_l, RETROK_SEMICOLON, RETROK_QUOTE, RETROK_RETURN
    };

    static Key const     row4[] = {"shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "shift", nullptr};
    static uint8_t const siz4[] = {50,      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  52};

    static unsigned const cod4[] = {
        RETROK_LSHIFT, RETROK_z, RETROK_x, RETROK_c, RETROK_v, RETROK_b, RETROK_n,
        RETROK_m, RETROK_COMMA, RETROK_PERIOD, RETROK_SLASH, RETROK_RSHIFT
    };

    static Key const     row5[] = {"ctrl", "win", "alt", " ", "alt", "ctrl", nullptr};
    static uint8_t const siz5[] = {40,     30,    30,    136, 32,    40};

    static unsigned const cod5[] = {RETROK_LCTRL, RETROK_LSUPER, RETROK_LALT, RETROK_SPACE, RETROK_RALT, RETROK_RCTRL};

    static Key const* const rows[] = {row0, row1, row2, row3, row4, row5};
    static uint8_t const* const sizes[] = {siz0, siz1, siz2, siz3, siz4, siz5};
    static unsigned const* codes[] = {cod0, cod1, cod2, cod3, cod4, cod5};

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));

    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        Key const* keys = rows[i];

        for (size_t j = 0; keys[j] != nullptr; j++) {
            ImVec2 const size(static_cast<float>(sizes[i][j]), 20.0f);

            if (ImGui::Button(keys[j], size)) {
                _keyState[codes[i][j]] = 5;
            }

            ImGui::SameLine();
        }

        ImGui::NewLine();
    }

    ImGui::PopStyleVar();
}

void hc::Input::addController(const SDL_Event* event) {
    addController(event->cdevice.which);
}

void hc::Input::removeController(const SDL_Event* event) {
    size_t const count = _pads.size();

    for (size_t i = 0; i < count; i++) {
        if (_pads[i].id == event->cdevice.which) {
            Pad const& pad = _pads[i];

            _logger->info(TAG "Controller %s (%s) removed", pad.controllerName.c_str(), pad.joystickName.c_str());

            for (size_t port = 0; port < MaxPorts; port++) {
                if (_ports[port].type == RETRO_DEVICE_JOYPAD && _ports[port].padId == pad.id) {
                    _ports[port].selectedType = 0;
                    _ports[port].selectedDevice = 0;
                    _ports[port].type = RETRO_DEVICE_NONE;
                    _ports[port].padId = NONE_ID;

                    _frontend->setControllerPortDevice(port, RETRO_DEVICE_NONE);
                }
            }

            SDL_GameControllerClose(pad.controller);
            _pads.erase(_pads.begin() + i);
            break;
        }
    }

}

void hc::Input::controllerButton(const SDL_Event* event) {
    Pad* pad = nullptr;
    size_t const count = _pads.size();

    for (size_t i = 0; i < count; i++) {
        if (_pads[i].id == event->cbutton.which) {
            pad = &_pads[i];
            break;
        }
    }

    if (pad == nullptr) {
        return;
    }

    unsigned button;

    switch (event->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A: button = RETRO_DEVICE_ID_JOYPAD_B; break;
        case SDL_CONTROLLER_BUTTON_B: button = RETRO_DEVICE_ID_JOYPAD_A; break;
        case SDL_CONTROLLER_BUTTON_X: button = RETRO_DEVICE_ID_JOYPAD_Y; break;
        case SDL_CONTROLLER_BUTTON_Y: button = RETRO_DEVICE_ID_JOYPAD_X; break;
        case SDL_CONTROLLER_BUTTON_BACK: button = RETRO_DEVICE_ID_JOYPAD_SELECT; break;
        case SDL_CONTROLLER_BUTTON_START: button = RETRO_DEVICE_ID_JOYPAD_START; break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: button = RETRO_DEVICE_ID_JOYPAD_L3; break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: button = RETRO_DEVICE_ID_JOYPAD_R3; break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: button = RETRO_DEVICE_ID_JOYPAD_L; break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: button = RETRO_DEVICE_ID_JOYPAD_R; break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: button = RETRO_DEVICE_ID_JOYPAD_UP; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: button = RETRO_DEVICE_ID_JOYPAD_DOWN; break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: button = RETRO_DEVICE_ID_JOYPAD_LEFT; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: button = RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
        case SDL_CONTROLLER_BUTTON_GUIDE: // fallthrough
        default: return;
    }

    pad->state[button] = event->cbutton.state == SDL_PRESSED;
}

void hc::Input::controllerAxis(const SDL_Event* event) {
    Pad* pad = nullptr;
    size_t const count = _pads.size();

    for (size_t i = 0; i < count; i++) {
        if (_pads[i].id == event->cbutton.which) {
            pad = &_pads[i];
            break;
        }
    }

    if (pad == nullptr) {
        return;
    }

    int const threshold = 32767 * (1.0f - pad->sensitivity);
    int positive, negative;
    int button;
    int* lastDir;

    switch (event->caxis.axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
        case SDL_CONTROLLER_AXIS_LEFTY:
        case SDL_CONTROLLER_AXIS_RIGHTX:
        case SDL_CONTROLLER_AXIS_RIGHTY:
            switch (event->caxis.axis) {
                case SDL_CONTROLLER_AXIS_LEFTX:
                    pad->analogs[RETRO_DEVICE_INDEX_ANALOG_LEFT].x = event->caxis.value;
                    positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
                    negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
                    lastDir = pad->lastDir + 0;
                    break;

                case SDL_CONTROLLER_AXIS_LEFTY:
                    pad->analogs[RETRO_DEVICE_INDEX_ANALOG_LEFT].y = event->caxis.value;
                    positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
                    negative = RETRO_DEVICE_ID_JOYPAD_UP;
                    lastDir = pad->lastDir + 1;
                    break;

                case SDL_CONTROLLER_AXIS_RIGHTX:
                    pad->analogs[RETRO_DEVICE_INDEX_ANALOG_RIGHT].x = event->caxis.value;
                    positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
                    negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
                    lastDir = pad->lastDir + 2;
                    break;

                case SDL_CONTROLLER_AXIS_RIGHTY:
                    pad->analogs[RETRO_DEVICE_INDEX_ANALOG_RIGHT].y = event->caxis.value;
                    positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
                    negative = RETRO_DEVICE_ID_JOYPAD_UP;
                    lastDir = pad->lastDir + 3;
                    break;
            }

            if (event->caxis.value < -threshold) {
                button = negative;
            }
            else if (event->caxis.value > threshold) {
                button = positive;
            }
            else {
                button = -1;
            }

            break;

        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            if (event->caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                pad->analogs[RETRO_DEVICE_INDEX_ANALOG_BUTTON].x = event->caxis.value;
                button = RETRO_DEVICE_ID_JOYPAD_L2;
                lastDir = pad->lastDir + 4;
            }
            else {
                pad->analogs[RETRO_DEVICE_INDEX_ANALOG_BUTTON].y = event->caxis.value;
                button = RETRO_DEVICE_ID_JOYPAD_R2;
                lastDir = pad->lastDir + 5;
            }

            break;

        default:
            return;
    }

    if (pad->digital) {
        if (*lastDir != -1) {
            pad->state[*lastDir] = false;
        }

        if (event->caxis.value < -threshold || event->caxis.value > threshold) {
            pad->state[button] = true;
        }

        *lastDir = button;
    }
}

void hc::Input::keyboard(SDL_Event const* event) {
    if (event->key.repeat) {
        return;
    }

    Pad& pad = _pads[1];

    switch (event->key.keysym.sym) {
        case SDLK_z: pad.state[RETRO_DEVICE_ID_JOYPAD_Y] = event->key.state == SDL_PRESSED; break;
        case SDLK_x: pad.state[RETRO_DEVICE_ID_JOYPAD_B] = event->key.state == SDL_PRESSED; break;
        case SDLK_c: pad.state[RETRO_DEVICE_ID_JOYPAD_A] = event->key.state == SDL_PRESSED; break;
        case SDLK_s: pad.state[RETRO_DEVICE_ID_JOYPAD_X] = event->key.state == SDL_PRESSED; break;
        case SDLK_a: pad.state[RETRO_DEVICE_ID_JOYPAD_L] = event->key.state == SDL_PRESSED; break;
        case SDLK_d: pad.state[RETRO_DEVICE_ID_JOYPAD_R] = event->key.state == SDL_PRESSED; break;
        case SDLK_q: pad.state[RETRO_DEVICE_ID_JOYPAD_L2] = event->key.state == SDL_PRESSED; break;
        case SDLK_w: pad.state[RETRO_DEVICE_ID_JOYPAD_START] = event->key.state == SDL_PRESSED; break;
        case SDLK_e: pad.state[RETRO_DEVICE_ID_JOYPAD_R2] = event->key.state == SDL_PRESSED; break;
        case SDLK_1: pad.state[RETRO_DEVICE_ID_JOYPAD_L3] = event->key.state == SDL_PRESSED; break;
        case SDLK_2: pad.state[RETRO_DEVICE_ID_JOYPAD_SELECT] = event->key.state == SDL_PRESSED; break;
        case SDLK_3: pad.state[RETRO_DEVICE_ID_JOYPAD_R3] = event->key.state == SDL_PRESSED; break;
        case SDLK_UP: pad.state[RETRO_DEVICE_ID_JOYPAD_UP] = event->key.state == SDL_PRESSED; break;
        case SDLK_DOWN: pad.state[RETRO_DEVICE_ID_JOYPAD_DOWN] = event->key.state == SDL_PRESSED; break;
        case SDLK_LEFT: pad.state[RETRO_DEVICE_ID_JOYPAD_LEFT] = event->key.state == SDL_PRESSED; break;
        case SDLK_RIGHT: pad.state[RETRO_DEVICE_ID_JOYPAD_RIGHT] = event->key.state == SDL_PRESSED; break;
    }
}
