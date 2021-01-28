#pragma once

#include "Desktop.h"
#include "Scriptable.h"
#include "Handle.h"

#include <lrcpp/libretro.h>

#include <SDL.h>

#include <string>
#include <vector>

namespace hc {
    class Device {
    public:
        Device(Desktop* desktop) : _desktop(desktop) {}
        virtual ~Device() {}

        virtual char const* getName() const = 0;
        virtual void draw() = 0;
        virtual void process(SDL_Event const* event) = 0;

    protected:
        Desktop* _desktop;
    };

    class Controller : public Device {
    public:
        Controller(Desktop* desktop);
        virtual ~Controller() {}

        bool init(SDL_ControllerDeviceEvent const* event);
        bool destroy(SDL_ControllerDeviceEvent const* event);

        bool getButton(unsigned id) const;
        int16_t getAnalog(unsigned index, unsigned id) const;

        // Device
        virtual char const* getName() const override;
        virtual void draw() override;
        virtual void process(SDL_Event const* event) override;

    protected:
        struct Axes {
            int16_t x, y;
        };

        void process(SDL_ControllerButtonEvent const* event);
        void process(SDL_ControllerAxisEvent const* event);

        int _deviceIndex;
        SDL_JoystickID _id;
        SDL_GameController* _controller;
        std::string _controllerName;
        SDL_Joystick* _joystick;
        std::string _joystickName;
        int _lastDir[6];
        bool _state[16];
        Axes _analogs[3];
        float _sensitivity;
        bool _digital;
    };

    class VirtualController : public Controller {
    public:
        VirtualController(Desktop* desktop) : Controller(desktop) {}
        virtual ~VirtualController() {}

        // Device
        virtual char const* getName() const override;
        virtual void process(SDL_Event const* event) override;
    };

    class Keyboard : public Device {
    public:
        Keyboard(Desktop* desktop);
        virtual ~Keyboard() {}

        bool getKey(unsigned id) const;

        // Device
        virtual char const* getName() const override;
        virtual void draw() override;
        virtual void process(SDL_Event const* event) override;

    protected:
        enum {
            DurationKeepPressedUs = 100000
        };

        bool _keyState[RETROK_LAST];
        uint64_t _virtualState[RETROK_LAST];
    };

    class Mouse : public Device {
    public:
        Mouse(Desktop* desktop);
        virtual ~Mouse() {}

        bool getPosition(int* x, int* y);
        bool getLeftDown() const;
        bool getRightDown() const;

        // Device
        virtual char const* getName() const override;
        virtual void draw() override;
        virtual void process(SDL_Event const* event) override;

    protected:
        // Mouse coordinates are provided by the video component
        Video* _video;
        int _lastX, _lastY;
    };

    class Devices: public View {
    public:
        Devices(Desktop* desktop);
        virtual ~Devices() {}

        void process(SDL_Event const* event);

        std::vector<Handle<Controller*>> const& getControllers() const { return _controllers; }
        Controller* translate(Handle<Controller*> const handle);

        Keyboard* getKeyboard() { return &_keyboard; }
        Mouse* getMouse() { return &_mouse; }

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;

    protected:
        void addController(SDL_ControllerDeviceEvent const* event);
        void removeController(SDL_ControllerDeviceEvent const* event);
        void joystickAdded(SDL_JoyDeviceEvent const* event);

        HandleAllocator<Controller*> _handles;
        std::vector<Handle<Controller*>> _controllers;
        int _selected;

        Keyboard _keyboard;
        Mouse _mouse;
    };
}
