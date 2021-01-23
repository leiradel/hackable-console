#pragma once

#include "Desktop.h"
#include "Logger.h"

#include <lrcpp/Components.h>

#include <vector>

namespace hc {
    class Led : public View, public lrcpp::Led {
    public:
        Led(Desktop* desktop);
        virtual ~Led() {}

        void init(hc::Logger* const logger);

        static Led* check(lua_State* const L, int const index);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameReset() override;
        virtual void onDraw() override;
        virtual void onGameUnloaded() override;

        // hc::Scriptable
        virtual int push(lua_State* const L) override;

        // lrcpp::Led
        virtual void setState(int led, int state) override;

    protected:
        static int l_setState(lua_State* const L);

        hc::Logger* _logger;

        std::vector<int> _states;
    };
}
