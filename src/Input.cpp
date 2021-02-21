#include "Input.h"
#include "Video.h"

#include <IconsFontAwesome4.h>

extern "C" {
    #include "lauxlib.h"
}

#include <algorithm>

#define NONE_ID -1
#define KEYBOARD_ID -2
#define TAG "[INP] "

hc::Input::Input(Desktop* desktop) : View(desktop), _frontend(nullptr) {}

void hc::Input::init(lrcpp::Frontend* const frontend) {
    _frontend = frontend;
}

char const* hc::Input::getTitle() {
    return ICON_FA_GAMEPAD " Input";
}

void hc::Input::onCoreLoaded() {
    for (size_t port = 0; port < MaxPorts; port++) {
        // All ports start disconnected
        _ports[port].selectedType = 0;
        _ports[port].selectedDevice = -1;
        _ports[port].type = RETRO_DEVICE_NONE;
        _ports[port].controller = nullptr;

        /**
         * Add None and RetroPad controllers to the ports, RetroArch always has
         * these controllers.
         */
        _controllerTypes[port].insert(_controllerTypes[port].begin(), ControllerInfo("RetroPad", RETRO_DEVICE_JOYPAD));
        _controllerTypes[port].insert(_controllerTypes[port].begin(), ControllerInfo("None", RETRO_DEVICE_NONE));
    }

    // TODO auto-assign controllers to ports here? Do it in the Lua console script?
}

void hc::Input::onDraw() {
    static char const* const portNames[MaxPorts] = {"Port 1", "Port 2", "Port 3", "Port 4"};

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
                    _ports[port].selectedDevice = -1;
                    _ports[port].type = _controllerTypes[port][selected].id & RETRO_DEVICE_MASK;
                    _ports[port].controller = nullptr;

                    _frontend->setControllerPortDevice(port, _controllerTypes[port][selected].id);
                }

                if (_ports[port].type == RETRO_DEVICE_JOYPAD) {
                    static auto const getter = [](void* data, int idx, char const** text) -> bool {
                        auto controllers = static_cast<std::vector<Controller*> const*>(data);
                        *text = (*controllers)[idx]->getName();
                        return true;
                    };

                    int selected = _ports[port].selectedDevice;
                    ImGui::Combo("Device", &selected, getter, &_controllers, static_cast<int>(_controllers.size()));

                    if (selected != _ports[port].selectedDevice) {
                        _ports[port].selectedDevice = selected;
                        _ports[port].controller = _controllers[selected];
                    }
                }

                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }
}

void hc::Input::onCoreUnloaded() {
    for (size_t port = 0; port < MaxPorts; port++) {
        // Disconnect all ports
        _ports[port].selectedType = 0;
        _ports[port].selectedDevice = -1;
        _ports[port].type = RETRO_DEVICE_NONE;
        _ports[port].controller = nullptr;

        // Erase the controller types
        _controllerTypes[port].clear();
    }
}

void hc::Input::deviceInserted(Device* device) {
    if (dynamic_cast<Keyboard*>(device) != nullptr) {
        _keyboard = dynamic_cast<Keyboard*>(device);
    }
    else if (dynamic_cast<Mouse*>(device) != nullptr) {
        _mouse = dynamic_cast<Mouse*>(device);
    }
    else if (dynamic_cast<Controller*>(device) != nullptr) {
        _controllers.emplace_back(dynamic_cast<Controller*>(device));

        std::sort(_controllers.begin(), _controllers.end(), [](Controller* const& a, Controller* const& b) -> bool {
            return strcmp(a->getName(), b->getName()) < 0;
        });
    }
    else {
        _desktop->warn(TAG "Cannot handle device \"%s\"", device->getName());
    }
}

void hc::Input::deviceRemoved(Device* device) {
    if (device == _keyboard) {
        _keyboard = nullptr;
    }
    else if (device == _keyboard) {
        _mouse = dynamic_cast<Mouse*>(device);
    }
    else if (dynamic_cast<Controller*>(device) != nullptr) {
        for (size_t port = 0; port < MaxPorts; port++) {
            if (_ports[port].controller == device) {
                _ports[port].selectedType = 0;
                _ports[port].selectedDevice = -1;
                _ports[port].type = RETRO_DEVICE_NONE;
                _ports[port].controller = nullptr;
            }
        }

        for (auto it = _controllers.begin(); it != _controllers.end(); ++it) {
            if (*it == device) {
                _controllers.erase(it);
                break;
            }
        }
    }
}

bool hc::Input::setInputDescriptors(retro_input_descriptor const* descriptors) {
    // We just log the descriptors, the information is currently discarded
    _desktop->info(TAG "Setting input descriptors");
    _desktop->info(TAG "    port device index id description");

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
        _desktop->info(TAG "    %4u %6u %5u %2u %s", desc->port, desc->device, desc->index, desc->id, desc->description);
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

    _desktop->info(TAG "Setting controller info");
    _desktop->info(TAG "    port id type     description");

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
            _desktop->info(TAG "    %4zu %2u %-8s %s", port + 1, type->id >> RETRO_DEVICE_TYPE_SHIFT, deviceName, type->desc);

            if (type->id != RETRO_DEVICE_NONE) {
                _controllerTypes[port].emplace_back(type->desc, type->id);
            }
            else {
                _desktop->warn(TAG "Not adding RETRO_DEVICE_NONE as it'll be added later");
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

    switch (base) {
        case RETRO_DEVICE_JOYPAD: {
            return port.controller != nullptr ? (port.controller->getButton(id) ? 32767 : 0) : 0;
        }

        case RETRO_DEVICE_ANALOG: {
            return port.controller != nullptr ? port.controller->getAnalog(index, id) : 0;
        }

        case RETRO_DEVICE_KEYBOARD: {
            return _keyboard != nullptr ? (_keyboard->getKey(id) ? 32767 : 0) : 0;
        }

        case RETRO_DEVICE_MOUSE: {
            if (_mouse == nullptr) {
                return 0;
            }

            int x = 0, y = 0;
            bool const inside = _mouse->getPosition(&x, &y);

            switch (id) {
                case RETRO_DEVICE_ID_MOUSE_X: {
                    int dx = 0;

                    if (inside) {
                        dx = x - _lastX;
                        _lastX = x;
                    }

                    return dx;
                }

                case RETRO_DEVICE_ID_MOUSE_Y: {
                    int dy = 0;

                    if (inside) {
                        dy = y - _lastY;
                        _lastY = y;
                    }

                    return dy;
                }

                case RETRO_DEVICE_ID_MOUSE_LEFT: return inside ? (_mouse->getLeftDown() ? 32767 : 0) : 0;
                case RETRO_DEVICE_ID_MOUSE_RIGHT: return inside ? (_mouse->getRightDown() ? 32767 : 0) : 0;
            }

            break;
        }
    }

    return 0;
}

void hc::Input::poll() {}
