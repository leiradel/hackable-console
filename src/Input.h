#pragma once

#include "Desktop.h"
#include "Devices.h"

#include <lrcpp/Components.h>
#include <lrcpp/Frontend.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <stdint.h>
#include <string>
#include <vector>

namespace hc {
    class Input: public View, public lrcpp::Input {
    public:
        Input(Desktop* desktop);
        virtual ~Input() {}

        void init(lrcpp::Frontend* const frontend, Devices* const devices);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onCoreLoaded() override;
        virtual void onDraw() override;
        virtual void onCoreUnloaded() override;

        // lrcpp::Input
        virtual bool setInputDescriptors(retro_input_descriptor const* descriptors) override;
        virtual bool setKeyboardCallback(retro_keyboard_callback const* callback) override;
        virtual bool getInputDeviceCapabilities(uint64_t* capabilities) override;
        virtual bool setControllerInfo(retro_controller_info const* info) override;
        virtual bool getInputBitmasks(bool* supports) override;

        virtual int16_t state(unsigned port, unsigned device, unsigned index, unsigned id) override;
        virtual void poll() override;

    protected:
        enum {
            MaxPorts = 4
        };

        struct ControllerInfo {
            ControllerInfo(char const* desc, unsigned id) : desc(desc), id(id) {}
            std::string desc;
            unsigned id;
        };

        struct Port {
            int selectedType; // for the UI
            int selectedDevice; // for the UI
            unsigned type; // RETRO_DEVICE_*
            Handle<Controller*> controller;
        };

        lrcpp::Frontend* _frontend;
        Devices* _devices;

        /**
         * Available controllers set via RETRO_ENVIRONMENT_SET_CONTROLLER_INFO.
         * Not all cores provide this information, and the Libretro API doesn't
         * specify when the core must set it when it does. The strategy here
         * is: after retro_set_environment is called, which happens right after
         * the core is loaded in lrcpp, we'll check if there were any
         * controllers set in _controllerTypes. If there weren't, we'll add one
         * fake RetroPad controller per port up to MaxPorts.
         */
        std::vector<ControllerInfo> _controllerTypes[MaxPorts];

        // The device attached to each port
        Port _ports[MaxPorts];

        // Last mouse positions to calculate the deltas
        int _lastX;
        int _lastY;
    };
}
