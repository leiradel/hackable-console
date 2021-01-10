#include "Input.h"

#include <IconsFontAwesome4.h>

static const uint32_t s_controller[] = {
    #include "controller.inl"
};

#define KEYBOARD_ID -1
#define TAG "[INP] "

hc::Input::Input() : _logger(nullptr), _frontend(nullptr), _texture(0), _ports(0) {}

void hc::Input::init(Logger* logger, lrcpp::Frontend* frontend) {
    _logger = logger;
    _frontend = frontend;

    GLint previous_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 768, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_controller);

    glBindTexture(GL_TEXTURE_2D, previous_texture);

    // Add the keyboard controller
    Pad keyb;

    keyb.id = KEYBOARD_ID;
    keyb.controller = NULL;
    keyb.controllerName = "Keyboard";
    keyb.joystick = NULL;
    keyb.joystickName = "Keyboard";
    keyb.lastDir[0] = keyb.lastDir[1] = keyb.lastDir[2] =
    keyb.lastDir[3] = keyb.lastDir[4] = keyb.lastDir[5] = -1;
    memset(keyb.state, 0, sizeof(keyb.state));
    keyb.sensitivity = 0.5f;
    keyb.port = 0;
    keyb.devId = RETRO_DEVICE_NONE;

    _pads.emplace(keyb.id, keyb);
    _logger->info(TAG "Controller %s (%s) added", keyb.controllerName.c_str(), keyb.joystickName.c_str());

    ControllerDescription ctrl;
    ctrl.desc = "None";
    ctrl.id = RETRO_DEVICE_NONE;

    for (size_t i = 0; i < sizeof(_ids) / sizeof(_ids[0]); i++) {
        _ids[i].emplace_back(ctrl);
    }

    // Add controllers already connected
    int const max = SDL_NumJoysticks();

    for (int i = 0; i < max; i++) {
        addController(i);
    }
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

unsigned hc::Input::getController(unsigned port)
{
    port++;

    for (auto const& pair : _pads) {
        auto const& pad = pair.second;

        if (pad.port == (int)port) {
            return pad.devId;
        }
    }

    // The controller was removed
    return RETRO_DEVICE_NONE;
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
void hc::Input::onConsoleLoaded() {}
void hc::Input::onGameLoaded() {}
void hc::Input::onGamePaused() {}
void hc::Input::onGameResumed() {}
void hc::Input::onGameReset() {}
void hc::Input::onFrame() {}

void hc::Input::onDraw() {
    drawPads();
    drawKeyboard();
}

void hc::Input::onGameUnloaded() {}

void hc::Input::onConsoleUnloaded() {
    _descriptors.clear();
    _controllers.clear();

    _ports = 0;

    ControllerDescription ctrl;
    ctrl.desc = "None";
    ctrl.id = RETRO_DEVICE_NONE;

    for (size_t i = 0; i < sizeof(_ids) / sizeof(_ids[0]); i++) {
        _ids[i].clear();
        _ids[i].emplace_back(ctrl);
    }

    for (auto pair : _pads) {
        Pad& pad = pair.second;

        pad.port = 0;
        pad.devId = RETRO_DEVICE_NONE;
    }
}

void hc::Input::onQuit() {
    glDeleteTextures(1, &_texture);
}

bool hc::Input::setInputDescriptors(retro_input_descriptor const* descriptors) {
    _logger->info(TAG "Setting input descriptors");
    _logger->info(TAG "    port device index id description");

    size_t count = 0;

    for (; descriptors[count].description != nullptr; count++) {
        // At least the Frodo core doesn't properly terminate the input
        // descriptor list with a zeroed entry, we do our best to avoid a crash
        if ((descriptors[count].device & RETRO_DEVICE_MASK) > RETRO_DEVICE_POINTER) {
            break;
        }

        if (descriptors[count].id > RETRO_DEVICE_ID_LIGHTGUN_RELOAD) {
            break;
        }
    }

    _descriptors.clear();
    _descriptors.reserve(count);

    for (size_t i = 0; i < count; i++) {
        retro_input_descriptor const* desc = descriptors + i;

        _logger->info(TAG "    %4u %6u %5u %2u %s", desc->port, desc->device, desc->index, desc->id, desc->description);

        InputDescriptor tempdesc;

        tempdesc.port = desc->port;
        tempdesc.device = desc->device;
        tempdesc.index = desc->index;
        tempdesc.id = desc->id;
        tempdesc.description = desc->description != nullptr ? desc->description : "";

        _descriptors.emplace_back(tempdesc);

        unsigned const port = tempdesc.port;

        if (port < sizeof(_ids) / sizeof(_ids[0])) {
            ControllerDescription ctrl;
            ctrl.desc = "RetroPad";
            ctrl.id = RETRO_DEVICE_JOYPAD;

            _ids[port].emplace_back(ctrl);
            _ports |= UINT64_C(1) << port;
        }
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
    _logger->info(TAG "Setting controller info");
    _logger->info(TAG "       id description");

    size_t count = 0;

    for (; info[count].types != nullptr; count++) /* nothing */;

    _controllers.clear();
    _controllers.reserve(count);

    for (size_t i = 0; i < count; i++) {
        Controller tempctrl;

        tempctrl.types.reserve(info[i].num_types);

        for (unsigned j = 0; j < info[i].num_types; j++) {
            retro_controller_description const* type = info[i].types + j;

            _logger->info(TAG "    %5u %s", type->id, type->desc);

            ControllerDescription tempdesc;
            tempdesc.desc = type->desc;
            tempdesc.id = type->id;

            if ((type->id & RETRO_DEVICE_MASK) == RETRO_DEVICE_JOYPAD) {
                unsigned const port = i;

                if (port < sizeof(_ids) / sizeof(_ids[0])) {
                    bool found = false;

                    for (auto& element : _ids[port]) {
                        if (element.id == type->id) {
                            // Overwrite the generic RetroPad description with the one from the controller info
                            element.desc = type->desc;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        ControllerDescription tempdesc2;
                        tempdesc2.desc = type->desc;
                        tempdesc2.id = type->id;
                        _ids[port].emplace_back(tempdesc2);
                    }

                    _ports |= UINT64_C(1) << port;
                }
            }

            tempctrl.types.emplace_back(tempdesc);
        }

        _controllers.emplace_back(tempctrl);
    }

    return true;
}

bool hc::Input::getInputBitmasks(bool* supports) {
    *supports = false;
    return false;
}

int16_t hc::Input::state(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)index;

    port++;

    for (auto const& pair : _pads) {
        Pad const& pad = pair.second;

        if (pad.port == (int)port && pad.devId == device) {
            return pad.state[id] ? 32767 : 0;
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

    if (_pads.find(pad.id) == _pads.end()) {
        pad.controllerName = SDL_GameControllerName(pad.controller);
        pad.joystickName = SDL_JoystickName(pad.joystick);
        pad.lastDir[0] = pad.lastDir[1] = pad.lastDir[2] =
        pad.lastDir[3] = pad.lastDir[4] = pad.lastDir[5] = -1;
        pad.sensitivity = 0.5f;
        pad.port = 0;
        pad.devId = RETRO_DEVICE_NONE;
        memset(pad.state, 0, sizeof(pad.state));

        _pads.insert(std::make_pair(pad.id, pad));
        _logger->info(TAG "Controller %s (%s) added", pad.controllerName.c_str(), pad.joystickName.c_str());
    }
    else {
        SDL_GameControllerClose(pad.controller);
    }
}

void hc::Input::drawPads() {
    if (!ImGui::Begin(ICON_FA_GAMEPAD " Controllers")) {
        return;
    }

    unsigned count = 1;

    for (auto& pair : _pads) {
        Pad& pad = pair.second;

        char label[512];
        snprintf(label, sizeof(label), "%s (%u)", pad.controllerName.c_str(), count);

        if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
            char id[32];
            snprintf(id, sizeof(id), "%p", static_cast<void*>(&pad));
            ImGui::Columns(2, id);

            {
                ImVec2 const pos = ImGui::GetCursorPos();
                drawPad(17);

                for (unsigned button = 0; button < 16; button++) {
                    if (pad.state[button]) {
                        ImGui::SetCursorPos(pos);
                        drawPad(button);
                    }
                }
            }

            ImGui::NextColumn();

            {
                char labels[1024];
                char* aux = labels;

                aux += snprintf(aux, sizeof(labels) - (aux - labels), "Disconnected") + 1;

                uint64_t bit = 1;

                for (unsigned i = 0; i < 64; i++, bit <<= 1) {
                    if ((_ports & bit) != 0) {
                        aux += snprintf(aux, sizeof(labels) - (aux - labels), "Connect to port %u", i + 1) + 1;
                    }
                }

                *aux = 0;

                ImGui::PushItemWidth(-1.0f);

                char label[64];
                snprintf(label, sizeof(label), "##port%p", static_cast<void*>(&pad));

                ImGui::Combo(label, &pad.port, labels);
                ImGui::PopItemWidth();
            }

            {
                char labels[512];
                unsigned ids[32];
                char* aux = labels;
                int count = 0;
                int selected = 0;

                aux += snprintf(aux, sizeof(labels) - (aux - labels), "None") + 1;
                ids[count++] = RETRO_DEVICE_NONE;

                if (_controllers.size() != 0) {
                    if (pad.port > 0 && (size_t)pad.port <= _controllers.size()) {
                        Controller const& ctrl = _controllers[pad.port - 1];

                        for (auto const& type : ctrl.types) {
                            if ((type.id & RETRO_DEVICE_MASK) == RETRO_DEVICE_JOYPAD) {
                                if (type.id == pad.devId) {
                                    selected = count;
                                }

                                aux += snprintf(aux, sizeof(labels) - (aux - labels), "%s", type.desc.c_str()) + 1;
                                ids[count++] = type.id;
                            }
                        }
                    }
                }
                else {
                    // No ports were specified, add the default RetroPad controller if the port is valid

                    if (pad.port != 0) {
                        aux += snprintf(aux, sizeof(labels) - (aux - labels), "RetroPad") + 1;

                        if (pad.devId == RETRO_DEVICE_JOYPAD) {
                            selected = 1;
                        }
                    }
                }

                *aux = 0;

                ImGui::PushItemWidth(-1.0f);

                char label[64];
                snprintf(label, sizeof(label), "##device%p", static_cast<void*>(&pad));

                int old = selected;
                ImGui::Combo(label, &selected, labels);

                if (_controllers.size() != 0) {
                    pad.devId = ids[selected];
                }
                else {
                    pad.devId = selected == 0 ? RETRO_DEVICE_NONE : RETRO_DEVICE_JOYPAD;
                }

                if (old != selected) {
                    _frontend->setControllerPortDevice(pad.port, pad.devId);
                }

                ImGui::PopItemWidth();
            }

            {
                char label[64];
                snprintf(label, sizeof(label), "##sensitivity%p", static_cast<void*>(&pad));

                ImGui::PushItemWidth(-1.0f);
                ImGui::SliderFloat(label, &pad.sensitivity, 0.0f, 1.0f, "Sensitivity %.3f");
                ImGui::PopItemWidth();
            }

            ImGui::Columns(1);
        }
    }

    ImGui::End();
}

void hc::Input::drawPad(unsigned button) {
    unsigned const y = button / 6;
    unsigned const x = button - y * 6;

    float const xx = x * 128.0f;
    float const yy = y * 64.0f;

    ImVec2 const size = ImVec2(113.0f, 63.0f);
    ImVec2 const uv0 = ImVec2(xx / 768.0f, yy / 192.0f);
    ImVec2 const uv1 = ImVec2((xx + 128.0f) / 768.0f, (yy + 64.0f) / 192.0f);

    ImGui::Image((ImTextureID)(uintptr_t)_texture, size, uv0, uv1);
}

void hc::Input::drawKeyboard() {
    typedef char const* Key;

    if (!ImGui::Begin(ICON_FA_KEYBOARD_O " Keyboard")) {
        return;
    }

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
        RETROK_j, RETROK_k, RETROK_l, RETROK_g, RETROK_SEMICOLON, RETROK_QUOTE, RETROK_RETURN
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
                (void)codes[i][j];
            }

            ImGui::SameLine();
        }

        ImGui::NewLine();
    }

    ImGui::PopStyleVar();
    ImGui::End();
}

void hc::Input::addController(const SDL_Event* event) {
    addController(event->cdevice.which);
}

void hc::Input::removeController(const SDL_Event* event) {
    auto it = _pads.find(event->cdevice.which);

    if (it != _pads.end()) {
        Pad const& pad = it->second;

        _logger->info(TAG "Controller %s (%s) removed", pad.controllerName.c_str(), pad.joystickName.c_str());

        _frontend->setControllerPortDevice(pad.port, RETRO_DEVICE_NONE);
        SDL_GameControllerClose(pad.controller);
        _pads.erase(it);
    }
}

void hc::Input::controllerButton(const SDL_Event* event) {
    auto it = _pads.find(event->cbutton.which);

    if (it == _pads.end()) {
        return;
    }

    Pad& pad = it->second;
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

    pad.state[button] = event->cbutton.state == SDL_PRESSED;
}

void hc::Input::controllerAxis(const SDL_Event* event) {
    auto it = _pads.find(event->caxis.which);

    if (it == _pads.end()) {
        return;
    }

    Pad& pad = it->second;

    int const threshold = 32767 * pad.sensitivity;
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
                    positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
                    negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
                    lastDir = pad.lastDir + 0;
                    break;

                case SDL_CONTROLLER_AXIS_LEFTY:
                    positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
                    negative = RETRO_DEVICE_ID_JOYPAD_UP;
                    lastDir = pad.lastDir + 1;
                    break;

                case SDL_CONTROLLER_AXIS_RIGHTX:
                    positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
                    negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
                    lastDir = pad.lastDir + 2;
                    break;

                case SDL_CONTROLLER_AXIS_RIGHTY:
                    positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
                    negative = RETRO_DEVICE_ID_JOYPAD_UP;
                    lastDir = pad.lastDir + 3;
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
                button = RETRO_DEVICE_ID_JOYPAD_L2;
                lastDir = pad.lastDir + 4;
            }
            else {
                button = RETRO_DEVICE_ID_JOYPAD_R2;
                lastDir = pad.lastDir + 5;
            }

            break;

        default:
            return;
    }

    if (*lastDir != -1) {
        pad.state[*lastDir] = false;
    }

    if (event->caxis.value < -threshold || event->caxis.value > threshold) {
        pad.state[button] = true;
    }

    *lastDir = button;
}

void hc::Input::keyboard(SDL_Event const* event) {
    if (event->key.repeat) {
        return;
    }

    SDL_Event evt;
    evt.cbutton.which = KEYBOARD_ID;
    evt.cbutton.state = event->key.state;

    switch (event->key.keysym.sym) {
        case SDLK_s: evt.cbutton.button = SDL_CONTROLLER_BUTTON_A; break;
        case SDLK_d: evt.cbutton.button = SDL_CONTROLLER_BUTTON_B; break;
        case SDLK_a: evt.cbutton.button = SDL_CONTROLLER_BUTTON_X; break;
        case SDLK_w: evt.cbutton.button = SDL_CONTROLLER_BUTTON_Y; break;
        case SDLK_BACKSPACE: evt.cbutton.button = SDL_CONTROLLER_BUTTON_BACK; break;
        case SDLK_RETURN: evt.cbutton.button = SDL_CONTROLLER_BUTTON_START; break;
        case SDLK_1: evt.cbutton.button = SDL_CONTROLLER_BUTTON_LEFTSTICK; break;
        case SDLK_3: evt.cbutton.button = SDL_CONTROLLER_BUTTON_RIGHTSTICK; break;
        case SDLK_q: evt.cbutton.button = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; break;
        case SDLK_e: evt.cbutton.button = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; break;
        case SDLK_UP: evt.cbutton.button = SDL_CONTROLLER_BUTTON_DPAD_UP; break;
        case SDLK_DOWN: evt.cbutton.button = SDL_CONTROLLER_BUTTON_DPAD_DOWN; break;
        case SDLK_LEFT: evt.cbutton.button = SDL_CONTROLLER_BUTTON_DPAD_LEFT; break;
        case SDLK_RIGHT: evt.cbutton.button = SDL_CONTROLLER_BUTTON_DPAD_RIGHT; break;
        case SDLK_2: evt.cbutton.button = SDL_CONTROLLER_BUTTON_GUIDE; break;
        default: return;
    }

    controllerButton(&evt);
}
