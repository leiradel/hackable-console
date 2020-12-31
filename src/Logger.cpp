#include "Logger.h"

#include <IconsFontAwesome4.h>

bool Logger::init() {
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

    return true;
}

void Logger::destroy() {}

void Logger::reset() {
    _logger.clear();
}

void Logger::draw() {
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

void Logger::vprintf(enum retro_log_level level, const char* fmt, va_list args) {
    switch (level) {
        case RETRO_LOG_DEBUG: _logger.debug(fmt, args); break;
        case RETRO_LOG_INFO:  _logger.info(fmt, args); break;
        case RETRO_LOG_WARN:  _logger.warning(fmt, args); break;
        default: // fallthrough
        case RETRO_LOG_ERROR: _logger.error(fmt, args); break;
    }

    _logger.scrollToBottom();
}
