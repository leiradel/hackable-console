#include "Devices.h"
#include "Perf.h"
#include "Video.h"

#include <IconsFontAwesome4.h>
#include <imgui.h>

#define TAG "[DEV] "

hc::Controller::Controller(Desktop* desktop) : Device(desktop), _deviceIndex(-1), _controller(nullptr), _joystick(nullptr) {
    _lastDir[0] = _lastDir[1] = _lastDir[2] = _lastDir[3] = _lastDir[4] = _lastDir[5] = -1;
    memset(_state, 0, sizeof(_state));
    memset(_analogs, 0, sizeof(_analogs));
    _sensitivity = 0.5f;
    _digital = false;
}

bool hc::Controller::init(SDL_ControllerDeviceEvent const* event) {
    if (event->type != SDL_CONTROLLERDEVICEADDED) {
        _desktop->error(TAG "SDL event is not SDL_CONTROLLERDEVICEADDED");
        return false;
    }

    if (!SDL_IsGameController(event->which)) {
        _desktop->error(TAG "SDL device %d is not a controller", event->which);
        return false;
    }

    _controller = SDL_GameControllerOpen(event->which);

    if (_controller == nullptr) {
        _desktop->error(TAG "Error opening the controller: %s", SDL_GetError());
        return false;
    }

    _joystick = SDL_GameControllerGetJoystick(_controller);

    if (_joystick == nullptr) {
        _desktop->error(TAG "Error getting the joystick: %s", SDL_GetError());
        SDL_GameControllerClose(_controller);
        return false;
    }

    _deviceIndex = event->which;
    _id = SDL_JoystickInstanceID(_joystick);
    _controllerName = SDL_GameControllerName(_controller);
    _joystickName = SDL_JoystickName(_joystick);

    _desktop->info(TAG "Controller %s (%s) added", _controllerName.c_str(), _joystickName.c_str());
    return true;
}

bool hc::Controller::destroy(SDL_ControllerDeviceEvent const* event) {
    if (event->type != SDL_CONTROLLERDEVICEREMOVED) {
        _desktop->error(TAG "SDL event is not SDL_CONTROLLERDEVICEREMOVED");
        return false;
    }

    if (event->which != _deviceIndex) {
        return false;
    }

    _desktop->info(TAG "Controller %s (%s) removed", _controllerName.c_str(), _joystickName.c_str());
    SDL_GameControllerClose(_controller);
    return true;
}

bool hc::Controller::getButton(unsigned id) const {
    return _state[id];
}

int16_t hc::Controller::getAnalog(unsigned index, unsigned id) const {
    return id == RETRO_DEVICE_ID_ANALOG_X ? _analogs[index].x : _analogs[index].y;
}

// Device
char const* hc::Controller::getName() const {
    return _controllerName.c_str();
}

void hc::Controller::draw() {
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
        show("L3", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_L3] != 0);
        show("L2", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_L2] != 0);
        show("L", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_L] != 0);
        show("R", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_R] != 0);
        show("R2", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_R2] != 0);
        show("R3", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_R3] != 0);

        ImGui::NewLine();

        show("Select", 62.0f, _state[RETRO_DEVICE_ID_JOYPAD_SELECT] != 0);
        show("Start", 62.0f, _state[RETRO_DEVICE_ID_JOYPAD_START] != 0);

        ImGui::NewLine();

        skip(20.0f);
        ImGui::SameLine();
        show("U", 21.0f, _state[RETRO_DEVICE_ID_JOYPAD_UP] != 0);
        skip(39.0f);
        show("X", 21.0f, _state[RETRO_DEVICE_ID_JOYPAD_X] != 0);

        ImGui::NewLine();

        skip(10.0f);
        show("L", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_LEFT] != 0);
        show("R", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_RIGHT] != 0);
        skip(19.0f);
        show("Y", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_Y] != 0);
        show("A", 20.0f, _state[RETRO_DEVICE_ID_JOYPAD_A] != 0);

        ImGui::NewLine();

        skip(20.0f);
        show("D", 21.0f, _state[RETRO_DEVICE_ID_JOYPAD_DOWN] != 0);
        skip(39.0f);
        show("B", 21.0f, _state[RETRO_DEVICE_ID_JOYPAD_B] != 0);
    }

    ImGui::PopStyleVar();
    ImGui::NextColumn();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));

    {
        ImGui::Checkbox("Analog to digital", &_digital);
        ImGui::SliderFloat("##Sensitivity", &_sensitivity, 0.0f, 1.0f, "Sensitivity %.3f");
        ImGui::Text("Left:     %6d / %6d", _analogs[0].x, _analogs[0].y);
        ImGui::Text("Right:    %6d / %6d", _analogs[1].x, _analogs[1].y);
        ImGui::Text("Triggers: %6d / %6d", _analogs[2].x, _analogs[2].y);
    }

    ImGui::PopStyleVar();
    ImGui::Columns(1);
}

void hc::Controller::process(SDL_Event const* event) {
    switch (event->type) {
        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERBUTTONDOWN:
            process(&event->cbutton);
            break;

        case SDL_CONTROLLERAXISMOTION:
            process(&event->caxis);
            break;
    }
}

void hc::Controller::process(SDL_ControllerButtonEvent const* event) {
    if (_id != event->which) {
        return;
    }

    unsigned button = 0;

    switch (event->button) {
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

    _state[button] = event->state == SDL_PRESSED;
}

void hc::Controller::process(SDL_ControllerAxisEvent const* event) {
    if (_id != event->which) {
        return;
    }

    int const threshold = 32767 * (1.0f - _sensitivity);
    int positive = 0, negative = 0;
    int button = 0;
    int* lastDir = nullptr;

    switch (event->axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
        case SDL_CONTROLLER_AXIS_LEFTY:
        case SDL_CONTROLLER_AXIS_RIGHTX:
        case SDL_CONTROLLER_AXIS_RIGHTY:
            switch (event->axis) {
                case SDL_CONTROLLER_AXIS_LEFTX:
                    _analogs[RETRO_DEVICE_INDEX_ANALOG_LEFT].x = event->value;
                    positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
                    negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
                    lastDir = _lastDir + 0;
                    break;

                case SDL_CONTROLLER_AXIS_LEFTY:
                    _analogs[RETRO_DEVICE_INDEX_ANALOG_LEFT].y = event->value;
                    positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
                    negative = RETRO_DEVICE_ID_JOYPAD_UP;
                    lastDir = _lastDir + 1;
                    break;

                case SDL_CONTROLLER_AXIS_RIGHTX:
                    _analogs[RETRO_DEVICE_INDEX_ANALOG_RIGHT].x = event->value;
                    positive = RETRO_DEVICE_ID_JOYPAD_RIGHT;
                    negative = RETRO_DEVICE_ID_JOYPAD_LEFT;
                    lastDir = _lastDir + 2;
                    break;

                case SDL_CONTROLLER_AXIS_RIGHTY:
                    _analogs[RETRO_DEVICE_INDEX_ANALOG_RIGHT].y = event->value;
                    positive = RETRO_DEVICE_ID_JOYPAD_DOWN;
                    negative = RETRO_DEVICE_ID_JOYPAD_UP;
                    lastDir = _lastDir + 3;
                    break;
            }

            if (event->value < -threshold) {
                button = negative;
            }
            else if (event->value > threshold) {
                button = positive;
            }
            else {
                button = -1;
            }

            break;

        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            if (event->axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                _analogs[RETRO_DEVICE_INDEX_ANALOG_BUTTON].x = event->value;
                button = RETRO_DEVICE_ID_JOYPAD_L2;
                lastDir = _lastDir + 4;
            }
            else {
                _analogs[RETRO_DEVICE_INDEX_ANALOG_BUTTON].y = event->value;
                button = RETRO_DEVICE_ID_JOYPAD_R2;
                lastDir = _lastDir + 5;
            }

            break;

        default:
            return;
    }

    if (_digital) {
        if (*lastDir != -1) {
            _state[*lastDir] = false;
        }

        if (event->value < -threshold || event->value > threshold) {
            _state[button] = true;
        }

        *lastDir = button;
    }
}

char const* hc::VirtualController::getName() const {
    return "Virtual Controller";
}

void hc::VirtualController::process(SDL_Event const* event) {
    if (event->type != SDL_KEYUP && event->type != SDL_KEYDOWN) {
        return;
    }

    if (event->key.repeat) {
        return;
    }

    bool const down = event->key.state == SDL_PRESSED;
    int16_t const analog = down ? 32767 : 0;

    switch (event->key.keysym.sym) {
        case SDLK_z: _state[RETRO_DEVICE_ID_JOYPAD_Y] = down; break;
        case SDLK_x: _state[RETRO_DEVICE_ID_JOYPAD_B] = down; break;
        case SDLK_c: _state[RETRO_DEVICE_ID_JOYPAD_A] = down; break;
        case SDLK_s: _state[RETRO_DEVICE_ID_JOYPAD_X] = down; break;
        case SDLK_a: _state[RETRO_DEVICE_ID_JOYPAD_L] = down; break;
        case SDLK_d: _state[RETRO_DEVICE_ID_JOYPAD_R] = down; break;
        case SDLK_q: _state[RETRO_DEVICE_ID_JOYPAD_L2] = down; break;
        case SDLK_w: _state[RETRO_DEVICE_ID_JOYPAD_START] = down; break;
        case SDLK_e: _state[RETRO_DEVICE_ID_JOYPAD_R2] = down; break;
        case SDLK_1: _state[RETRO_DEVICE_ID_JOYPAD_L3] = down; break;
        case SDLK_2: _state[RETRO_DEVICE_ID_JOYPAD_SELECT] = down; break;
        case SDLK_3: _state[RETRO_DEVICE_ID_JOYPAD_R3] = down; break;

        case SDLK_UP:
            _state[RETRO_DEVICE_ID_JOYPAD_UP] = down;
            _analogs[0].y = -analog;
            break;

        case SDLK_DOWN:
            _state[RETRO_DEVICE_ID_JOYPAD_DOWN] = down;
            _analogs[0].y = analog;
            break;

        case SDLK_LEFT:
            _state[RETRO_DEVICE_ID_JOYPAD_LEFT] = down;
            _analogs[0].x = -analog;
            break;

        case SDLK_RIGHT:
            _state[RETRO_DEVICE_ID_JOYPAD_RIGHT] = down;
            _analogs[0].x = analog;
            break;
    }
}

hc::Keyboard::Keyboard(Desktop* desktop)
    : Device(desktop)
    , _lock1(0)
    , _lock2(0)
{
    memset(_keyState, 0, sizeof(_keyState));
    memset(_virtualState, 0, sizeof(_virtualState));
}

bool hc::Keyboard::getKey(unsigned id) const {
    uint64_t const now = Perf::getTimeUs();
    bool const lshift = id == RETROK_LSHIFT && (_lock1 == 1 || _lock2 == 1);
    bool const rshift = id == RETROK_RSHIFT && (_lock1 == 2 || _lock2 == 2);
    bool const lcontrol = id == RETROK_LCTRL && (_lock1 == 3 || _lock2 == 3);
    bool const rcontrol = id == RETROK_RCTRL && (_lock1 == 4 || _lock2 == 4);
    return _keyState[id] || (now - _virtualState[id]) < DurationKeepPressedUs || lshift || rshift || lcontrol || rcontrol;
}

// Device
char const* hc::Keyboard::getName() const {
    return "Keyboard";
}

void hc::Keyboard::draw() {
    typedef char const* Key;

    // First row
    static Key const     row0[] = {"esc", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", nullptr};
    static Key const*    ROW0 = row0;
    static uint8_t const siz0[] = {25,    23,   23,   23,   23,   23,   23,   23,   23,   23,   23,    23,    23};

    static unsigned const cod0[] = {
        RETROK_ESCAPE, RETROK_F1, RETROK_F2, RETROK_F3, RETROK_F4, RETROK_F5, RETROK_F6,
        RETROK_F7, RETROK_F8, RETROK_F9, RETROK_F10, RETROK_F11, RETROK_F12
    };

    // Second row
    static Key const     row1[] = {"`",  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "bs", nullptr};
    static Key const     ROW1[] = {"~",  "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "bs", nullptr};
    static uint8_t const siz1[] = {20,   20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  40};

    static unsigned const cod1[] = {
        RETROK_BACKQUOTE, RETROK_1, RETROK_2, RETROK_3, RETROK_4, RETROK_5, RETROK_6, RETROK_7,
        RETROK_8, RETROK_9, RETROK_0, RETROK_MINUS, RETROK_EQUALS, RETROK_BACKSPACE
    };

    // Third row
    static Key const     row2[] = {"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\", nullptr};
    static Key const     ROW2[] = {"tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "|", nullptr};
    static uint8_t const siz2[] = {30,    20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  30};

    static unsigned const cod2[] = {
        RETROK_TAB, RETROK_q, RETROK_w, RETROK_e, RETROK_r, RETROK_t, RETROK_y, RETROK_u,
        RETROK_i, RETROK_o, RETROK_p, RETROK_LEFTBRACKET, RETROK_RIGHTBRACKET, RETROK_BACKSLASH
    };

    // Fourth row
    static Key const     row3[] = {"caps", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'",  "enter", nullptr};
    static Key const     ROW3[] = {"caps", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "enter", nullptr};
    static uint8_t const siz3[] = {40,     20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,   41};

    static unsigned const cod3[] = {
        RETROK_CAPSLOCK, RETROK_a, RETROK_s, RETROK_d, RETROK_f, RETROK_g, RETROK_h,
        RETROK_j, RETROK_k, RETROK_l, RETROK_SEMICOLON, RETROK_QUOTE, RETROK_RETURN
    };

    // Fifth row
    static Key const     row4[] = {"shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "shift", nullptr};
    static Key const     ROW4[] = {"shift", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "shift", nullptr};
    static uint8_t const siz4[] = {50,      20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  52};

    static unsigned const cod4[] = {
        RETROK_LSHIFT, RETROK_z, RETROK_x, RETROK_c, RETROK_v, RETROK_b, RETROK_n,
        RETROK_m, RETROK_COMMA, RETROK_PERIOD, RETROK_SLASH, RETROK_RSHIFT
    };

    // Sixth row
    static Key const     row5[] = {"ctrl", "win", "alt", " ", "alt", "ctrl", nullptr};
    static Key const*    ROW5 = row5;
    static uint8_t const siz5[] = {40,     30,    30,    136, 32,    40};

    static unsigned const cod5[] = {RETROK_LCTRL, RETROK_LSUPER, RETROK_LALT, RETROK_SPACE, RETROK_RALT, RETROK_RCTRL};

    // Layout
    static Key const* const rows[] = {row0, row1, row2, row3, row4, row5};
    static Key const* const ROWS[] = {ROW0, ROW1, ROW2, ROW3, ROW4, ROW5};
    static uint8_t const* const sizes[] = {siz0, siz1, siz2, siz3, siz4, siz5};
    static unsigned const* codes[] = {cod0, cod1, cod2, cod3, cod4, cod5};

    bool const lshift = _lock1 == 1 || _lock2 == 1;
    bool const rshift = _lock1 == 2 || _lock2 == 2;

    Key const* const* const layout = (lshift || rshift) ? ROWS : rows;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));

    for (size_t i = 0; i < 6; i++) {
        Key const* keys = layout[i];

        for (size_t j = 0; keys[j] != nullptr; j++) {
            ImVec2 const size(static_cast<float>(sizes[i][j]), 20.0f);

            if (ImGui::Button(keys[j], size)) {
                _virtualState[codes[i][j]] = Perf::getTimeUs();
            }

            ImGui::SameLine();
        }

        ImGui::NewLine();
    }

    ImGui::PopStyleVar();

    static char const* const lockables[] = {
        "None","Left Shift", "Right Shift", "Left Control", "Right Control", "Left Alt", "Right Alt"
    };

    ImGui::Columns(2);
    ImGui::Combo("Lock##1", &_lock1, lockables, sizeof(lockables) / sizeof(lockables[0]));
    ImGui::NextColumn();
    ImGui::Combo("Lock##2", &_lock2, lockables, sizeof(lockables) / sizeof(lockables[0]));
    ImGui::Columns(1);
}

void hc::Keyboard::process(SDL_Event const* event) {
    if (event->type != SDL_KEYUP && event->type != SDL_KEYDOWN) {
        return;
    }

    if (event->key.repeat) {
        return;
    }

    unsigned key = 0;

    switch (event->key.keysym.sym) {
        case SDLK_RETURN: key = RETROK_RETURN; break;
        case SDLK_ESCAPE: key = RETROK_ESCAPE; break;
        case SDLK_BACKSPACE: key = RETROK_BACKSPACE; break;
        case SDLK_TAB: key = RETROK_TAB; break;
        case SDLK_SPACE: key = RETROK_SPACE; break;
        case SDLK_EXCLAIM: key = RETROK_EXCLAIM; break;
        case SDLK_QUOTEDBL: key = RETROK_QUOTEDBL; break;
        case SDLK_HASH: key = RETROK_HASH; break;
        case SDLK_DOLLAR: key = RETROK_DOLLAR; break;
        case SDLK_AMPERSAND: key = RETROK_AMPERSAND; break;
        case SDLK_QUOTE: key = RETROK_QUOTE; break;
        case SDLK_LEFTPAREN: key = RETROK_LEFTPAREN; break;
        case SDLK_RIGHTPAREN: key = RETROK_RIGHTPAREN; break;
        case SDLK_ASTERISK: key = RETROK_ASTERISK; break;
        case SDLK_PLUS: key = RETROK_PLUS; break;
        case SDLK_COMMA: key = RETROK_COMMA; break;
        case SDLK_MINUS: key = RETROK_MINUS; break;
        case SDLK_PERIOD: key = RETROK_PERIOD; break;
        case SDLK_SLASH: key = RETROK_SLASH; break;
        case SDLK_0: key = RETROK_0; break;
        case SDLK_1: key = RETROK_1; break;
        case SDLK_2: key = RETROK_2; break;
        case SDLK_3: key = RETROK_3; break;
        case SDLK_4: key = RETROK_4; break;
        case SDLK_5: key = RETROK_5; break;
        case SDLK_6: key = RETROK_6; break;
        case SDLK_7: key = RETROK_7; break;
        case SDLK_8: key = RETROK_8; break;
        case SDLK_9: key = RETROK_9; break;
        case SDLK_COLON: key = RETROK_COLON; break;
        case SDLK_SEMICOLON: key = RETROK_SEMICOLON; break;
        case SDLK_LESS: key = RETROK_LESS; break;
        case SDLK_EQUALS: key = RETROK_EQUALS; break;
        case SDLK_GREATER: key = RETROK_GREATER; break;
        case SDLK_QUESTION: key = RETROK_QUESTION; break;
        case SDLK_AT: key = RETROK_AT; break;
        case SDLK_LEFTBRACKET: key = RETROK_LEFTBRACKET; break;
        case SDLK_BACKSLASH: key = RETROK_BACKSLASH; break;
        case SDLK_RIGHTBRACKET: key = RETROK_RIGHTBRACKET; break;
        case SDLK_CARET: key = RETROK_CARET; break;
        case SDLK_UNDERSCORE: key = RETROK_UNDERSCORE; break;
        case SDLK_BACKQUOTE: key = RETROK_BACKQUOTE; break;
        case SDLK_a: key = RETROK_a; break;
        case SDLK_b: key = RETROK_b; break;
        case SDLK_c: key = RETROK_c; break;
        case SDLK_d: key = RETROK_d; break;
        case SDLK_e: key = RETROK_e; break;
        case SDLK_f: key = RETROK_f; break;
        case SDLK_g: key = RETROK_g; break;
        case SDLK_h: key = RETROK_h; break;
        case SDLK_i: key = RETROK_i; break;
        case SDLK_j: key = RETROK_j; break;
        case SDLK_k: key = RETROK_k; break;
        case SDLK_l: key = RETROK_l; break;
        case SDLK_m: key = RETROK_m; break;
        case SDLK_n: key = RETROK_n; break;
        case SDLK_o: key = RETROK_o; break;
        case SDLK_p: key = RETROK_p; break;
        case SDLK_q: key = RETROK_q; break;
        case SDLK_r: key = RETROK_r; break;
        case SDLK_s: key = RETROK_s; break;
        case SDLK_t: key = RETROK_t; break;
        case SDLK_u: key = RETROK_u; break;
        case SDLK_v: key = RETROK_v; break;
        case SDLK_w: key = RETROK_w; break;
        case SDLK_x: key = RETROK_x; break;
        case SDLK_y: key = RETROK_y; break;
        case SDLK_z: key = RETROK_z; break;
        case SDLK_CAPSLOCK: key = RETROK_CAPSLOCK; break;
        case SDLK_F1: key = RETROK_F1; break;
        case SDLK_F2: key = RETROK_F2; break;
        case SDLK_F3: key = RETROK_F3; break;
        case SDLK_F4: key = RETROK_F4; break;
        case SDLK_F5: key = RETROK_F5; break;
        case SDLK_F6: key = RETROK_F6; break;
        case SDLK_F7: key = RETROK_F7; break;
        case SDLK_F8: key = RETROK_F8; break;
        case SDLK_F9: key = RETROK_F9; break;
        case SDLK_F10: key = RETROK_F10; break;
        case SDLK_F11: key = RETROK_F11; break;
        case SDLK_F12: key = RETROK_F12; break;
        case SDLK_PRINTSCREEN: key = RETROK_PRINT; break;
        case SDLK_SCROLLLOCK: key = RETROK_SCROLLOCK; break;
        case SDLK_PAUSE: key = RETROK_PAUSE; break;
        case SDLK_INSERT: key = RETROK_INSERT; break;
        case SDLK_HOME: key = RETROK_HOME; break;
        case SDLK_PAGEUP: key = RETROK_PAGEUP; break;
        case SDLK_DELETE: key = RETROK_DELETE; break;
        case SDLK_END: key = RETROK_END; break;
        case SDLK_PAGEDOWN: key = RETROK_PAGEDOWN; break;
        case SDLK_RIGHT: key = RETROK_RIGHT; break;
        case SDLK_LEFT: key = RETROK_LEFT; break;
        case SDLK_DOWN: key = RETROK_DOWN; break;
        case SDLK_UP: key = RETROK_UP; break;
        case SDLK_KP_DIVIDE: key = RETROK_KP_DIVIDE; break;
        case SDLK_KP_MULTIPLY: key = RETROK_KP_MULTIPLY; break;
        case SDLK_KP_MINUS: key = RETROK_KP_MINUS; break;
        case SDLK_KP_PLUS: key = RETROK_KP_PLUS; break;
        case SDLK_KP_ENTER: key = RETROK_KP_ENTER; break;
        case SDLK_KP_1: key = RETROK_KP1; break;
        case SDLK_KP_2: key = RETROK_KP2; break;
        case SDLK_KP_3: key = RETROK_KP3; break;
        case SDLK_KP_4: key = RETROK_KP4; break;
        case SDLK_KP_5: key = RETROK_KP5; break;
        case SDLK_KP_6: key = RETROK_KP6; break;
        case SDLK_KP_7: key = RETROK_KP7; break;
        case SDLK_KP_8: key = RETROK_KP8; break;
        case SDLK_KP_9: key = RETROK_KP9; break;
        case SDLK_KP_0: key = RETROK_KP0; break;
        case SDLK_KP_PERIOD: key = RETROK_KP_PERIOD; break;
        case SDLK_POWER: key = RETROK_POWER; break;
        case SDLK_KP_EQUALS: key = RETROK_KP_EQUALS; break;
        case SDLK_F13: key = RETROK_F13; break;
        case SDLK_F14: key = RETROK_F14; break;
        case SDLK_F15: key = RETROK_F15; break;
        case SDLK_HELP: key = RETROK_HELP; break;
        case SDLK_MENU: key = RETROK_MENU; break;
        case SDLK_UNDO: key = RETROK_UNDO; break;
        case SDLK_SYSREQ: key = RETROK_SYSREQ; break;
        case SDLK_LCTRL: key = RETROK_LCTRL; break;
        case SDLK_LSHIFT: key = RETROK_LSHIFT; break;
        case SDLK_LALT: key = RETROK_LALT; break;
        case SDLK_RCTRL: key = RETROK_RCTRL; break;
        case SDLK_RSHIFT: key = RETROK_RSHIFT; break;
        case SDLK_RALT: key = RETROK_RALT; break;
        case SDLK_MODE: key = RETROK_MODE; break;

        default: return;
    }

    _keyState[key] = event->key.state == SDL_PRESSED;
}


hc::Mouse::Mouse(Desktop* desktop) : Device(desktop), _video(nullptr) {}

void hc::Mouse::init(Video* video) {
    _video = video;
}

bool hc::Mouse::getPosition(int* x, int* y) {
    return _video->getMousePos(x, y);
}

bool hc::Mouse::getLeftDown() const {
    return ImGui::IsMouseDown(0);
}

bool hc::Mouse::getRightDown() const {
    return ImGui::IsMouseDown(1);
}

char const* hc::Mouse::getName() const {
    return "Mouse";
}

void hc::Mouse::draw() {
    int x = -1, y = -1;
    bool const inside = getPosition(&x, &y);

    if (inside) {
        ImGui::Text("Mouse cursor is at %d, %d", x, y);
    }
    else {
        ImGui::Text("Mouse cursor is outside the emulated framebuffer");
    }
}

void hc::Mouse::process(SDL_Event const* event) {
    (void)event;
}

hc::Devices::Devices(Desktop* desktop)
    : View(desktop)
    , _selected(0)
    , _keyboard(desktop)
    , _mouse(desktop)
    , _virtualController(desktop)
{
    _controllers.emplace_back(&_virtualController);
}

void hc::Devices::init(Video* video) {
    _mouse.init(video);

    for (auto const listener : _listeners) {
        listener->deviceInserted(&_keyboard);
        listener->deviceInserted(&_mouse);

        for (auto const controller : _controllers) {
            listener->deviceInserted(controller);
        }
    }
}

void hc::Devices::process(SDL_Event const* event) {
    switch (event->type) {
        case SDL_CONTROLLERDEVICEADDED:
            addController(&event->cdevice);
            return;

        case SDL_CONTROLLERDEVICEREMOVED:
            removeController(&event->cdevice);
            return;

        case SDL_JOYDEVICEADDED:
            joystickAdded(&event->jdevice);
            return;
    }

    for (auto& controller : _controllers) {
        controller->process(event);
    }

    _keyboard.process(event);
    _mouse.process(event);
}

void hc::Devices::addListener(DeviceListener* listener) {
    _listeners.emplace_back(listener);

    listener->deviceInserted(&_keyboard);
    listener->deviceInserted(&_mouse);

    for (auto const controller : _controllers) {
        listener->deviceInserted(controller);
    }
}

char const* hc::Devices::getTitle() {
    return ICON_FA_KEYBOARD_O " Devices";
}

void hc::Devices::onDraw() {
    static auto const getter = [](void* data, int idx, char const** text) -> bool {
        auto const controllers = static_cast<std::vector<Controller*> const*>(data);

        if (idx == 0) {
            *text = "Keyboard";
        }
        else if (idx == 1) {
            *text = "Mouse";
        }
        else {
            *text = (*controllers)[idx - 2]->getName();
        }

        return true;
    };

    ImGui::Combo("Device", &_selected, getter, &_controllers, static_cast<int>(_controllers.size() + 2));

    if (_selected == 0) {
        _keyboard.draw();
    }
    else if (_selected == 1) {
        _mouse.draw();
    }
    else if (_selected >= 2 && static_cast<size_t>(_selected - 2) < _controllers.size()) {
        Controller* const controller = _controllers[_selected - 2];
        controller->draw();
    }
}

void hc::Devices::addController(SDL_ControllerDeviceEvent const* event) {
    Controller* const controller = new Controller(_desktop);

    if (!controller->init(event)) {
        return;
    }

    _controllers.emplace_back(controller);

    for (auto const listener : _listeners) {
        listener->deviceInserted(controller);
    }
}

void hc::Devices::removeController(SDL_ControllerDeviceEvent const* event) {
    size_t const count = _controllers.size();

    for (size_t i = 0; i < count; i++) {
        Controller* const controller = _controllers[i];

        if (controller->destroy(event)) {
            for (auto const listener : _listeners) {
                listener->deviceRemoved(controller);
            }

            _controllers.erase(_controllers.begin() + i);

            if (controller != &_virtualController) {
                delete controller;
            }

            if (static_cast<size_t>(_selected) == (count + 1)) {
                _selected--;
            }

            return;
        }
    }
}

void hc::Devices::joystickAdded(SDL_JoyDeviceEvent const* event) {
    SDL_JoystickGUID const guid = SDL_JoystickGetDeviceGUID(event->which);
    char const* const mapping = SDL_GameControllerMappingForGUID(guid);
    char const* const name = SDL_JoystickNameForIndex(event->which);

    if (mapping == nullptr) {
        char guidStr[128];
        SDL_JoystickGetGUIDString(guid, guidStr, sizeof(guidStr));
        _desktop->error(TAG "No mapping for joystick \"%s\" (GUID %s), joystick unusable", name, guidStr);
    }
    else {
        SDL_free((void*)mapping);
    }
}
