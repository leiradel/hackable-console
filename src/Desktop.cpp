#include "Desktop.h"

#include "Logger.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <stdlib.h>

#define TAG "[DSK] "

hc::Desktop::Desktop() : _logger(nullptr) {}

void hc::Desktop::init(Logger* const logger) {
    _logger = logger;
}

void hc::Desktop::add(View* const view, bool const top, bool const free, char const* const id) {
    ViewProperties props = {view, top, free, id, true};
    _views.emplace(id, props);
}

char const* hc::Desktop::getTitle() {
    return ICON_FA_PLUG " Views";
}

void hc::Desktop::onStarted() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onStarted %s", view->getTitle());
        view->onStarted();
    }
}

void hc::Desktop::onConsoleLoaded() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onConsoleLoaded %s", view->getTitle());
        view->onConsoleLoaded();
    }
}

void hc::Desktop::onGameLoaded() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onGameLoaded %s", view->getTitle());
        view->onGameLoaded();
    }
}

void hc::Desktop::onGamePaused() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onGamePaused %s", view->getTitle());
        view->onGamePaused();
    }
}

void hc::Desktop::onGameResumed() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onGameResumed %s", view->getTitle());
        view->onGameResumed();
    }
}

void hc::Desktop::onGameReset() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onGameReset %s", view->getTitle());
        view->onGameReset();
    }
}

void hc::Desktop::onFrame() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        // Don't log stuff per frame
        view->onFrame();
    }
}

void hc::Desktop::onDraw() {
    if (ImGui::Begin(ICON_FA_PLUG " Views")) {
        ImGui::Columns(2);

        for (auto& pair : _views) {
            ViewProperties& props = pair.second;
            View* const view = props.view;

            if (!props.top) {
                continue;
            }

            ImGui::Text("%s", view->getTitle());
            ImGui::NextColumn();

            char label[32];
            snprintf(label, sizeof(label), "Open##%p", reinterpret_cast<void const*>(&props));

            if (ImGuiAl::Button(label, !props.opened)) {
                props.opened = true;
            }

            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    ImGui::End();

    for (auto& pair : _views) {
        ViewProperties& props = pair.second;
        View* const view = pair.second.view;
        // Don't log stuff per frame

        // Don't recursively draw the plugin manager
        if (view != this && props.opened) {
            if (ImGui::Begin(view->getTitle(), &props.opened)) {
                view->onDraw();
                ImGui::End();
            }
        }
    }
}

void hc::Desktop::onGameUnloaded() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onGameUnloaded %s", view->getTitle());
        view->onGameUnloaded();
    }
}

void hc::Desktop::onConsoleUnloaded() {
    for (auto const& pair : _views) {
        View* const view = pair.second.view;
        _logger->debug(TAG "onConsoleUnloaded %s", view->getTitle());
        view->onConsoleUnloaded();
    }
}

void hc::Desktop::onQuit() {
    for (auto const& pair : _views) {
        ViewProperties const& props = pair.second;
        View* const view = props.view;
        _logger->debug(TAG "onQuit plugin %s", view->getTitle());
        view->onQuit();

        if (view != this && props.free) {
            delete view;
        }
    }

    _views.clear();
}

int hc::Desktop::push(lua_State* const L) {
    lua_createtable(L, 0, _views.size());

    for (auto const& pair : _views) {
        ViewProperties const& props = pair.second;
        View* const view = props.view;
        view->push(L);
        lua_setfield(L, -2, props.id);
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