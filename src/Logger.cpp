#include "Logger.h"

#include <IconsFontAwesome4.h>

extern "C" {
    #include <lauxlib.h>
}

#define TAG "[LUA] "

hc::Logger::Logger() : _mutex(nullptr) {}

bool hc::Logger::init() {
    _mutex = SDL_CreateMutex();

    if (_mutex == nullptr) {
        return false;
    }

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

char const* hc::Logger::getTitle() {
    return ICON_FA_COMMENT " Log";
}

void hc::Logger::onStarted() {}

void hc::Logger::onConsoleLoaded() {}

void hc::Logger::onGameLoaded() {}

void hc::Logger::onGamePaused() {}

void hc::Logger::onGameResumed() {}

void hc::Logger::onGameReset() {}

void hc::Logger::onFrame() {}

void hc::Logger::onDraw(bool* opened) {
    if (!*opened) {
        return;
    }

    if (!ImGui::Begin(ICON_FA_COMMENT " Log", opened)) {
        return;
    }

    SDL_LockMutex(_mutex);
    int const button = _logger.draw();
    SDL_UnlockMutex(_mutex);

    switch (button) {
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

    ImGui::End();
}

void hc::Logger::onGameUnloaded() {}

void hc::Logger::onConsoleUnloaded() {
    _logger.clear();
}

void hc::Logger::onQuit() {
    SDL_DestroyMutex(_mutex);
}

void hc::Logger::vprintf(enum retro_log_level level, const char* format, va_list args) {
    if (level > RETRO_LOG_DEBUG) {
        switch (level) {
            case RETRO_LOG_DEBUG: fprintf(stderr, "[DBG] "); break;
            case RETRO_LOG_INFO:  fprintf(stderr, "[NFO] "); break;
            case RETRO_LOG_WARN:  fprintf(stderr, "[WRN] "); break;
            case RETRO_LOG_ERROR: fprintf(stderr, "[ERR] "); break;
            default: fprintf(stderr, "[???] "); break;
        }

        va_list copy;
        va_copy(copy, args);
        vfprintf(stderr, format, copy);

        if (format[0] != 0 && format[strlen(format) - 1] != '\n') {
            fputc('\n', stderr);
        }

        va_end(copy);
    }

    SDL_LockMutex(_mutex);

    switch (level) {
        case RETRO_LOG_DEBUG: _logger.debug(format, args); break;
        case RETRO_LOG_INFO:  _logger.info(format, args); break;
        case RETRO_LOG_WARN:  _logger.warning(format, args); break;
        default: // fallthrough
        case RETRO_LOG_ERROR: _logger.error(format, args); break;
    }

    _logger.scrollToBottom();
    SDL_UnlockMutex(_mutex);
}

int hc::Logger::push(lua_State* const L) {
    auto const self = static_cast<Logger**>(lua_newuserdata(L, sizeof(Logger*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::Logger")) {
        static luaL_Reg const methods[] = {
            {"debug", l_debug},
            {"info", l_info},
            {"warn", l_warn},
            {"error", l_error},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);

        int const load = luaL_loadstring(L,
            "local string = require 'string'\n"
            "return function(fmt, ...)\n"
            "    return string.format(fmt, ...)\n"
            "end\n"
        );

        if (load != LUA_OK) {
            return lua_error(L);
        }

        lua_call(L, 0, 1);
        lua_setfield(L, -2, "format");

        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Logger* hc::Logger::check(lua_State* const L, int const index) {
    return *static_cast<Logger**>(luaL_checkudata(L, index, "hc::Logger"));
}

int hc::Logger::l_debug(lua_State* const L) {
    auto const self = check(L, 1);
    lua_getfield(L, 1, "format");
    lua_insert(L, 2);
    lua_call(L, lua_gettop(L) - 2, 1);
    self->debug(TAG "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}

int hc::Logger::l_info(lua_State* const L) {
    auto const self = check(L, 1);
    lua_getfield(L, 1, "format");
    lua_insert(L, 2);
    lua_call(L, lua_gettop(L) - 2, 1);
    self->info(TAG "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}

int hc::Logger::l_warn(lua_State* const L) {
    auto const self = check(L, 1);
    lua_getfield(L, 1, "format");
    lua_insert(L, 2);
    lua_call(L, lua_gettop(L) - 2, 1);
    self->warn(TAG "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}

int hc::Logger::l_error(lua_State* const L) {
    auto const self = check(L, 1);
    lua_getfield(L, 1, "format");
    lua_insert(L, 2);
    lua_call(L, lua_gettop(L) - 2, 1);
    self->error(TAG "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;
}
