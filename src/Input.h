#pragma once

#include "Plugin.h"
#include "Logger.h"

#include <Components.h>
#include <Frontend.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace hc {
    class Input: public Plugin, public lrcpp::Input {
    public:
        Input();
        virtual ~Input() {}

        void init(Logger* logger, lrcpp::Frontend* frontend);
        void processEvent(SDL_Event const* event);
        unsigned getController(unsigned port);

        // hc::Plugin
        virtual char const* getName() override;
        virtual char const* getVersion() override;
        virtual char const* getLicense() override;
        virtual char const* getCopyright() override;
        virtual char const* getUrl() override;

        virtual void onStarted() override;
        virtual void onConsoleLoaded() override;
        virtual void onGameLoaded() override;
        virtual void onGamePaused() override;
        virtual void onGameResumed() override;
        virtual void onGameReset() override;
        virtual void onFrame() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;
        virtual void onConsoleUnloaded() override;
        virtual void onQuit() override;

        // lrcpp::Input
        virtual bool setInputDescriptors(retro_input_descriptor const* descriptors) override;
        virtual bool setKeyboardCallback(retro_keyboard_callback const* callback) override;
        virtual bool getInputDeviceCapabilities(uint64_t* capabilities) override;
        virtual bool setControllerInfo(retro_controller_info const* info) override;
        virtual bool getInputBitmasks(bool* supports) override;

        virtual int16_t state(unsigned port, unsigned device, unsigned index, unsigned id) override;
        virtual void poll() override;

    protected:
        struct Pad {
            SDL_JoystickID id;
            SDL_GameController* controller;
            std::string controllerName;
            SDL_Joystick* joystick;
            std::string joystickName;
            int lastDir[6];
            bool state[16];
            float sensitivity;

            int port;
            unsigned devId;
        };

        struct ControllerDescription {
            std::string desc;
            unsigned id;
        };

        struct Controller {
            std::vector<ControllerDescription> types;
        };

        struct InputDescriptor {
            unsigned port;
            unsigned device;
            unsigned index;
            unsigned id;
            std::string description;
        };

        void addController(int which);
        void drawPads();
        void drawPad(unsigned button);
        void drawKeyboard();
        void addController(SDL_Event const* event);
        void removeController(SDL_Event const* event);
        void controllerButton(SDL_Event const* event);
        void controllerAxis(SDL_Event const* event);
        void keyboard(SDL_Event const* event);

        Logger* _logger;
        lrcpp::Frontend* _frontend;

        GLuint _texture;

        std::unordered_map<SDL_JoystickID, Pad> _pads;
        std::vector<InputDescriptor> _descriptors;
        std::vector<Controller> _controllers;
        uint64_t _ports;
        std::vector<ControllerDescription> _ids[64];

        uint8_t _keyState[RETROK_LAST];
    };
}
