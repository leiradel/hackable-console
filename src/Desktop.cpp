#include "Desktop.h"

#include "Logger.h"
#include "Config.h"
#include "Video.h"
#include "Led.h"
#include "Audio.h"
#include "Input.h"
#include "Perf.h"

#include <imguial_button.h>
#include <IconsFontAwesome4.h>

#include <stdlib.h>
#include <math.h>

#define TAG "[DSK] "

hc::Desktop::Desktop() : View(nullptr), _logger(nullptr) {}

void hc::Desktop::init(Logger* const logger) {
    _logger = logger;
    _drawCount = 0;
    _frameCount = 0;
}

void hc::Desktop::addView(View* const view, bool const top, bool const free) {
    auto const props = new ViewProperties {view, top, free, true};
    _views.emplace(props);
}

void hc::Desktop::removeView(View const* const view) {
    for (auto const& props : _views) {
        if (props->view == view) {
            props->opened = false;
            return;
        }
    }
}

double hc::Desktop::drawFps() {
    double const fps = static_cast<double>(_drawCount) * 1000000.0f / static_cast<double>(_drawTimer.getTimeUs());
    return isnan(fps) || isinf(fps) ? 0.0 : fps;
}

void hc::Desktop::resetDrawFps() {
    if (_drawTimer.started()) {
        _drawCount = 0;
        _drawTimer.reset();
    }
}

double hc::Desktop::frameFps() {
    double const fps = static_cast<double>(_frameCount) * 1000000.0f / static_cast<double>(_frameTimer.getTimeUs());
    return isnan(fps) || isinf(fps) ? 0.0 : fps;
}

void hc::Desktop::resetFrameFps() {
    if (_frameTimer.started()) {
        _frameCount = 0;
        _frameTimer.reset();
    }
}

void hc::Desktop::onStarted() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onStarted %s", view->getTitle());
        view->onStarted();
    }

    _drawTimer.start();
    _drawCount = 0;
}

void hc::Desktop::onCoreLoaded() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onCoreLoaded %s", view->getTitle());
        view->onCoreLoaded();
    }
}

void hc::Desktop::onGameLoaded() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onGameLoaded %s", view->getTitle());
        view->onGameLoaded();
    }
}

void hc::Desktop::onGameStarted() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onGameStarted %s", view->getTitle());
        view->onGameStarted();
    }

    _frameTimer.start();
    _frameCount = 0;
}

void hc::Desktop::onGamePaused() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onGamePaused %s", view->getTitle());
        view->onGamePaused();
    }

    _frameTimer.pause();
}

void hc::Desktop::onGameResumed() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onGameResumed %s", view->getTitle());
        view->onGameResumed();
    }

    _frameTimer.resume();
}

void hc::Desktop::onGameReset() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onGameReset %s", view->getTitle());
        view->onGameReset();
    }
}

void hc::Desktop::onFrame() {
    if (!_frameTimer.paused()) {
        _frameCount++;
    }

    for (auto const& props : _views) {
        View* const view = props->view;
        // Don't log stuff per frame
        view->onFrame();
    }
}

void hc::Desktop::onStep() {
    for (auto const& props : _views) {
        View* const view = props->view;
        // Don't log stuff per frame
        view->onStep();
    }
}

void hc::Desktop::onDraw() {
    _drawCount++;

    ImGui::ShowDemoWindow();

    if (ImGui::Begin(getTitle())) {
        ImGui::Columns(2);

        for (auto const& props : _views) {
            View* const view = props->view;

            if (!props->top) {
                continue;
            }

            ImGui::Text("%s", view->getTitle());
            ImGui::NextColumn();

            char label[32];
            snprintf(label, sizeof(label), "Open##%p", reinterpret_cast<void const*>(props));

            if (ImGuiAl::Button(label, !props->opened)) {
                props->opened = true;
            }

            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    ImGui::End();

    for (auto const& props : _views) {
        View* const view = props->view;
        // Don't log stuff per frame

        // Don't recursively draw the plugin manager
        if (view != this && props->opened) {
            if (ImGui::Begin(view->getTitle(), &props->opened)) {
                ImGui::PushID(view);
                view->onDraw();
                ImGui::PopID();
                ImGui::End();
            }
        }
    }

    for (;;) {
        bool done = true;

        for (auto const& props : _views) {
            if (!props->opened) {
                if (props->free) {
                    delete props->view;
                }

                _views.erase(props);
                done = false;
                break;
            }
        }

        if (done) {
            break;
        }
    }
}

void hc::Desktop::onGameUnloaded() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onGameUnloaded %s", view->getTitle());
        view->onGameUnloaded();
    }

    _frameTimer.stop();
}

void hc::Desktop::onCoreUnloaded() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onCoreUnloaded %s", view->getTitle());
        view->onCoreUnloaded();
    }
}

void hc::Desktop::onQuit() {
    for (auto const& props : _views) {
        View* const view = props->view;
        debug(TAG "onQuit plugin %s", view->getTitle());
        view->onQuit();

        if (view != this && props->free) {
            delete view;
        }
    }

    _views.clear();
    _drawTimer.stop();
}

void hc::Desktop::vprintf(retro_log_level level, char const* format, va_list args) {
    _logger->vprintf(level, format, args);
}

void hc::Desktop::debug(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_DEBUG, format, args);
    va_end(args);
}

void hc::Desktop::info(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_INFO, format, args);
    va_end(args);
}

void hc::Desktop::warn(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_WARN, format, args);
    va_end(args);
}

void hc::Desktop::error(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_ERROR, format, args);
    va_end(args);
}
