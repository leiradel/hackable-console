#include "Desktop.h"

#include "Logger.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <stdlib.h>
#include <algorithm>

#define TAG "[DSK] "

char const* hc::View::getTypeName() {
    switch (getType()) {
        case Type::Audio: return "audio";
        case Type::Config: return "config";
        case Type::Input: return "input";
        case Type::Led: return "led";
        case Type::Logger: return "logger";
        case Type::Perf: return "perf";
        case Type::Video: return "video";
        case Type::Control: return "control";
        case Type::Memory: return "memory";
        case Type::Manager: return "manager";
    }

    abort();
}

hc::Desktop::Desktop() : _logger(nullptr) {}

void hc::Desktop::init(Logger* const logger) {
    _logger = logger;
}

void hc::Desktop::add(View* const view, bool const destroy) {
    view->_opened = true;
    Vieww vieww = {view, destroy};
    _views.emplace_back(vieww);

    std::sort(_views.begin(), _views.end(), [](Vieww const& a, Vieww const& b) -> bool {
        return strcmp(a.view->getName(), b.view->getName()) < 0;
    });
}

char const* hc::Desktop::getName() {
    return "hc::Desktop built-in view manager";
}

char const* hc::Desktop::getVersion() {
    return "0.0.0";
}

char const* hc::Desktop::getLicense() {
    return "MIT";
}

char const* hc::Desktop::getCopyright() {
    return "Copyright (c) Andre Leiradella";
}

char const* hc::Desktop::getUrl() {
    return "https://github.com/leiradel/hackable-console";
}

void hc::Desktop::onStarted() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onStarted plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
        view->onStarted();
    }
}

void hc::Desktop::onConsoleLoaded() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onConsoleLoaded plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
        view->onConsoleLoaded();
    }
}

void hc::Desktop::onGameLoaded() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGameLoaded plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
        view->onGameLoaded();
    }
}

void hc::Desktop::onGamePaused() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGamePaused plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
        view->onGamePaused();
    }
}

void hc::Desktop::onGameResumed() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGameResumed plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
        view->onGameResumed();
    }
}

void hc::Desktop::onGameReset() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onGameReset plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
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

    if (ImGui::Begin(ICON_FA_PLUG " Plugins")) {
        ImGui::Columns(3);

        for (auto& vieww : _views) {
            View* const view = vieww.view;

            ImGui::Text("%s", view->getName());
            ImGui::NextColumn();
            ImGui::Text("%s", view->getVersion());
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
        _logger->debug(TAG "onGameUnloaded plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
        view->onGameUnloaded();
    }
}

void hc::Desktop::onConsoleUnloaded() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onConsoleUnloaded plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
        view->onConsoleUnloaded();
    }
}

void hc::Desktop::onQuit() {
    for (auto const& vieww : _views) {
        View* const view = vieww.view;
        _logger->debug(TAG "onQuit plugin %s (%s): %s", view->getName(), view->getVersion(), view->getCopyright());
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
        lua_setfield(L, -2, view->getTypeName());
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
