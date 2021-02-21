#pragma once

#include "Desktop.h"
#include "Scriptable.h"

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

    class DeviceListener {
    public:
        virtual void deviceInserted(Device* device) = 0;
        virtual void deviceRemoved(Device* device) = 0;
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

        int _lock1;
        int _lock2;
    };

    class Mouse : public Device {
    public:
        Mouse(Desktop* desktop);
        virtual ~Mouse() {}

        void init(Video* video);

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

        void init(Video* video);
        void process(SDL_Event const* event);
        void addListener(DeviceListener* listener);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onDraw() override;

    protected:
        void addController(SDL_ControllerDeviceEvent const* event);
        void removeController(SDL_ControllerDeviceEvent const* event);
        void joystickAdded(SDL_JoyDeviceEvent const* event);

        std::vector<Controller*> _controllers;
        int _selected;

        Keyboard _keyboard;
        Mouse _mouse;
        VirtualController _virtualController;

        std::vector<DeviceListener*> _listeners;
    };
}
