#pragma once

#include "Logger.h"

#include <Components.h>

#include <vector>

namespace hc {
    class Led : public lrcpp::Led {
    public:
        Led() : _logger(nullptr) {}

        bool init(hc::Logger* logger);
        void destroy();
        void reset();
        void draw();

        virtual void setState(int led, int state) override;

    protected:
        hc::Logger* _logger;

        std::vector<int> _states;
    };
}
