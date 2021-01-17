#include "Desktop.h"

#include "Logger.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <stdlib.h>
#include <algorithm>

#define TAG "[DSK] "

hc::Desktop::Desktop() : _logger(nullptr) {}

void hc::Desktop::init(Logger* const logger) {
    _logger = logger;
}

void hc::Desktop::add(View* const view, bool const destroy, char const* const id) {
    view->_opened = true;
    Vieww vieww = {view, destroy, id};
    _views.emplace_back(vieww);

    std::sort(_views.begin(), _views.end(), [](Vieww const& a, Vieww const& b) -> bool {
        return strcmp(a.id, b.id) < 0;
    });
}

char const* hc::Desktop::getTitle() {
    return ICON_FA_PLUG " Views";
}

void hc::Desktop::onStarted() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onStarted %s", view->getTitle());
        view->onStarted();
    }
}

void hc::Desktop::onConsoleLoaded() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onConsoleLoaded %s", view->getTitle());
        view->onConsoleLoaded();
    }
}

void hc::Desktop::onGameLoaded() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGameLoaded %s", view->getTitle());
        view->onGameLoaded();
    }
}

void hc::Desktop::onGamePaused() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGamePaused %s", view->getTitle());
        view->onGamePaused();
    }
}

void hc::Desktop::onGameResumed() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGameResumed %s", view->getTitle());
        view->onGameResumed();
    }
}

void hc::Desktop::onGameReset() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGameReset %s", view->getTitle());
        view->onGameReset();
    }
}

void hc::Desktop::onFrame() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        // Don't log stuff per frame
        view->onFrame();
    }
}

void hc::Desktop::onDraw(bool* opened) {
    (void)opened; // the plugin manager is always visible

    if (ImGui::Begin(ICON_FA_PLUG " Views")) {
        ImGui::Columns(2);

        for (auto& vieww : _views) {
            View* const view = vieww.view;

            ImGui::Text("%s", view->getTitle());
            ImGui::NextColumn();

            char label[32];
            snprintf(label, sizeof(label), "Open##%p", static_cast<void*>(&vieww));

            if (ImGuiAl::Button(label, !view->_opened)) {
                view->_opened = true;
            }

            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    ImGui::End();

    for (auto& vieww : _views) {
        View* const view = vieww.view;
        // Don't log stuff per frame

        if (view != this) {
            // Don't recursively draw the plugin manager
            view->onDraw(&view->_opened);
        }
    }
}

void hc::Desktop::onGameUnloaded() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGameUnloaded %s", view->getTitle());
        view->onGameUnloaded();
    }
}

void hc::Desktop::onConsoleUnloaded() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onConsoleUnloaded %s", view->getTitle());
        view->onConsoleUnloaded();
    }
}

void hc::Desktop::onQuit() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onQuit plugin %s", view->getTitle());
        view->onQuit();

        if (view != this && vieww.destroy) {
            delete view;
        }
    }

    _views.clear();
}

int hc::Desktop::push(lua_State* const L) {
    lua_createtable(L, 0, _views.size());

    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        view->push(L);
        lua_setfield(L, -2, vieww.id);
    }

    static struct {char const* const name; char const* const value;} const stringConsts[] = {
        {"_COPYRIGHT", "Copyright (c) 2020 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "hc"},
        {"_URL", "https://github.com/leiradel/hackable-console"},
        {"_DESCRIPTION", "Hackable Console bindings"},

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        {"soExtension", "dll"}
#elif __linux__
        {"soExtension", "so"}
#else
        #error Unsupported platform
#endif
    };

    size_t const stringCount = sizeof(stringConsts) / sizeof(stringConsts[0]);

    for (size_t i = 0; i < stringCount; i++) {
        lua_pushstring(L, stringConsts[i].value);
        lua_setfield(L, -2, stringConsts[i].name);
    }

    return 1;
}
