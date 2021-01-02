#include "Logger.h"

#include <IconsFontAwesome4.h>

bool hc::Logger::init() {
    static char const* actions[] = {
        ICON_FA_FILES_O " Copy",
        ICON_FA_FILE_O " Clear",
        NULL
    };

    _logger.setActions(actions);

    _logger.setLabel(ImGuiAl::Log::Level::Debug, ICON_FA_BUG " Debug");
    _logger.setLabel(ImGuiAl::Log::Level::Info, ICON_FA_INFO " Info");
    _logger.setLabel(ImGuiAl::Log::Level::Warning, ICON_FA_EXCLAMATION_TRIANGLE " Warn");
    _logger.setLabel(ImGuiAl::Log::Level::Error, ICON_FA_BOMB " Error");
    _logger.setCumulativeLabel(ICON_FA_SORT_AMOUNT_DESC " Cumulative");
    _logger.setFilterHeaderLabel(ICON_FA_FILTER " Filters");
    _logger.setFilterLabel(ICON_FA_SEARCH " Filter (inc,-exc)");

    _logger.setLevel(ImGuiAl::Log::Level::Info);
    return true;
}

void hc::Logger::destroy() {}

void hc::Logger::reset() {
    _logger.clear();
}

void hc::Logger::draw() {
    switch (_logger.draw()) {
        case 1: {
            std::string buffer;

            _logger.iterate([&buffer](ImGuiAl::Log::Info const& header, char const* const line) -> bool {
                switch (static_cast<ImGuiAl::Log::Level>(header.metaData)) {
                    case ImGuiAl::Log::Level::Debug:   buffer += "[DEBUG] "; break;
                    case ImGuiAl::Log::Level::Info:    buffer += "[INFO ] "; break;
                    case ImGuiAl::Log::Level::Warning: buffer += "[WARN ] "; break;
                    case ImGuiAl::Log::Level::Error:   buffer += "[ERROR] "; break;
                }

                buffer += line;
                buffer += '\n';

                return true;
            });

            ImGui::SetClipboardText(buffer.c_str());
            break;
        }

        case 2: {
            _logger.clear();
            break;
        }
    }
}

void hc::Logger::vprintf(enum retro_log_level level, const char* format, va_list args) {
    if (level > RETRO_LOG_DEBUG) {
        va_list copy;
        va_copy(copy, args);
        vfprintf(stderr, format, copy);
        fputc('\n', stderr);
        va_end(copy);
    }

    switch (level) {
        case RETRO_LOG_DEBUG: _logger.debug(format, args); break;
        case RETRO_LOG_INFO:  _logger.info(format, args); break;
        case RETRO_LOG_WARN:  _logger.warning(format, args); break;
        default: // fallthrough
        case RETRO_LOG_ERROR: _logger.error(format, args); break;
    }

    _logger.scrollToBottom();
}

void hc::Logger::debug(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_DEBUG, format, args);
    va_end(args);
}

void hc::Logger::info(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_INFO, format, args);
    va_end(args);
}

void hc::Logger::warn(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_WARN, format, args);
    va_end(args);
}

void hc::Logger::error(char const* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(RETRO_LOG_ERROR, format, args);
    va_end(args);
}
