#include "Config.h"
#include "Logger.h"

#include <fnkdat.h>
#include <IconsFontAwesome4.h>

extern "C" {
    #include "lauxlib.h"
}

#include <inttypes.h>

#define TAG "[CFG] "

static void getFlags(char flags[7], uint64_t const mcflags) {
    flags[0] = 'M';
    flags[2] = 'A';
    flags[6] = 0;

    switch (mcflags & (RETRO_MEMDESC_MINSIZE_2 | RETRO_MEMDESC_MINSIZE_4 | RETRO_MEMDESC_MINSIZE_8)) {
        case RETRO_MEMDESC_MINSIZE_2: flags[1] = '2'; break;
        case RETRO_MEMDESC_MINSIZE_4: flags[1] = '4'; break;
        case RETRO_MEMDESC_MINSIZE_8: flags[1] = '8'; break;
        default: flags[1] = '1'; break;
    }

    switch (mcflags & (RETRO_MEMDESC_ALIGN_2 | RETRO_MEMDESC_ALIGN_4 | RETRO_MEMDESC_ALIGN_8)) {
        case RETRO_MEMDESC_ALIGN_2: flags[3] = '2'; break;
        case RETRO_MEMDESC_ALIGN_4: flags[3] = '4'; break;
        case RETRO_MEMDESC_ALIGN_8: flags[3] = '8'; break;
        default: flags[3] = '1'; break;
    }

    flags[4] = (mcflags & RETRO_MEMDESC_BIGENDIAN) != 0 ? 'B' : 'b';
    flags[5] = (mcflags & RETRO_MEMDESC_CONST) != 0 ? 'C' : 'c';
}

hc::Config::Config(Desktop* desktop)
    : View(desktop)
    , _logger(nullptr)
    , _performanceLevel(0)
    , _supportsNoGame(false)
    , _supportAchievements(false)
    , _serializationQuirks(0)
    , _optionsUpdated(false)
    , _vsync(true)
    , _vsyncChanged(true)
    , _syncExact(false)
    , _syncExactChanged(true)
{
    memset(&_getCoreProc, 0, sizeof(_getCoreProc));
}

bool hc::Config::init() {
    _logger = _desktop->getLogger();

    if (fnkdat(NULL, 0, 0, FNKDAT_INIT) != 0) {
        _logger->error(TAG "Error initializing fnkdat");
        return false;
    }

    char path[1024];

    if (fnkdat("autorun.lua", path, sizeof(path), FNKDAT_USER | FNKDAT_CREAT) != 0) {
        _logger->error(TAG "Error getting the autorun.lua file path");
        return false;
    }

    _autorunPath = path;
    _logger->info(TAG "The autorun.lua file path is \"%s\"", path);

    if (fnkdat("system/", path, sizeof(path), FNKDAT_USER | FNKDAT_CREAT) != 0) {
        _logger->error(TAG "Error getting the system path");
        return false;
    }

    _systemPath = path;
    _logger->info(TAG "The system path is \"%s\"", path);

    if (fnkdat("assets/", path, sizeof(path), FNKDAT_USER | FNKDAT_CREAT) != 0) {
        _logger->error(TAG "Error getting the core assets path");
        return false;
    }

    _coreAssetsPath = path;
    _logger->info(TAG "The core assets path is \"%s\"", path);

    if (fnkdat("saves/", path, sizeof(path), FNKDAT_USER | FNKDAT_CREAT) != 0) {
        _logger->error(TAG "Error getting the saves path");
        return false;
    }

    _savePath = path;
    _logger->info(TAG "The save path is \"%s\"", path);

    if (fnkdat("cores/", path, sizeof(path), FNKDAT_USER | FNKDAT_CREAT) != 0) {
        _logger->error(TAG "Error getting the cores path");
        return false;
    }

    _coresPath = path;
    _logger->info(TAG "The cores path is \"%s\"", path);

    return true;
}

bool hc::Config::getSupportNoGame() const {
    return _supportsNoGame;
}

const std::string& hc::Config::getRootPath() const {
    return _rootPath;
}

const std::string& hc::Config::getAutorunPath() const {
    return _autorunPath;
}

bool hc::Config::vsync(bool* const on) {
    *on = _vsync;
    bool const changed = _vsyncChanged;
    _vsyncChanged = false;
    return changed;
}

bool hc::Config::syncExact(bool* const on) {
    *on = _syncExact;
    bool const changed = _syncExactChanged;
    _syncExactChanged = false;
    return changed;
}

char const* hc::Config::getTitle() {
    return ICON_FA_WRENCH " Configuration";
}

void hc::Config::onGameLoaded() {
    _optionsUpdated = false;
}

void hc::Config::onDraw() {
    bool checked = _vsync;
    ImGui::Checkbox("Vsync", &checked);

    if (checked != _vsync) {
        _vsync = checked;
        _vsyncChanged = true;

        if (_syncExact) {
            _syncExact = false;
            _syncExactChanged = true;
        }
    }

    checked = _syncExact;
    ImGui::Checkbox("Sync to exact content framerate", &checked);

    if (checked != _syncExact) {
        _syncExact = checked;
        _syncExactChanged = true;

        if (_vsync) {
            _vsync = false;
            _vsyncChanged = true;
        }
    }

    for (auto& option : _coreOptions) {
        if (!option.visible) {
            continue;
        }

        static auto const getter = [](void* const data, int const idx, char const** const text) -> bool {
            auto const option = (CoreOption const*)data;

            if ((size_t)idx < option->values.size()) {
                *text = option->values[idx].label.c_str();
                return true;
            }

            return false;
        };

        int selected = static_cast<int>(option.selected);
        int old = selected;

        ImGui::Combo(option.label.c_str(), &selected, getter, (void*)&option, option.values.size());

        if (old != selected) {
            option.selected = static_cast<unsigned>(selected);
            _optionsUpdated = true;
            _logger->info(TAG "Variable \"%s\" changed to \"%s\"", option.key.c_str(), option.values[selected].value.c_str());
        }
    }
}

void hc::Config::onCoreUnloaded() {
    _performanceLevel = 0;
    _supportsNoGame = false;
    _supportAchievements = false;
    _serializationQuirks = 0;
    _optionsUpdated = false;

    memset(&_getCoreProc, 0, sizeof(_getCoreProc));

    _subsystemInfo.clear();
    _memoryMap.clear();
    _coreOptions.clear();
    _coreMap.clear();
}

void hc::Config::onQuit() {
    fnkdat(NULL, 0, 0, FNKDAT_UNINIT);
}

bool hc::Config::setPerformanceLevel(unsigned level) {
    _performanceLevel = level;
    _logger->info(TAG "Set performance level to %u", level);
    return true;
}

bool hc::Config::getSystemDirectory(char const** directory) {
    *directory = _systemPath.c_str();
    _logger->info(TAG "Returning \"%s\" for system directory", *directory);
    return true;
}

bool hc::Config::getVariable(retro_variable* variable) {
    auto const found = _coreMap.find(variable->key);

    if (found != _coreMap.end()) {
        auto const& option = _coreOptions[found->second];
        variable->value = option.values[option.selected].value.c_str();
        _logger->info(TAG "Found value \"%s\" for variable \"%s\"", variable->value, variable->key);
        return true;
    }

    variable->value = nullptr;
    _logger->error(TAG "Variable \"%s\" not found", variable->key);
    return false;
}

bool hc::Config::getVariableUpdate(bool* const updated) {
    *updated = _optionsUpdated;
    _optionsUpdated = false;
    return true;
}

bool hc::Config::setSupportNoGame(bool const supports) {
    _supportsNoGame = supports;
    _logger->info(TAG "Set supports no game to %s", supports ? "true" : "false");
    return true;
}

bool hc::Config::getLibretroPath(char const** path) {
    *path = _coresPath.c_str();
    _logger->info(TAG "Returning \"%s\" for libretro path", *path);
    return true;
}

bool hc::Config::getCoreAssetsDirectory(char const** directory) {
    *directory = _coreAssetsPath.c_str();
    _logger->info(TAG "Returning \"%s\" for core assets directory", *directory);
    return true;
}

bool hc::Config::getSaveDirectory(char const** directory) {
    *directory = _savePath.c_str();
    _logger->info(TAG "Returning \"%s\" for save directory", *directory);
    return true;
}

bool hc::Config::setProcAddressCallback(retro_get_proc_address_interface const* callback) {
    _getCoreProc = callback->get_proc_address;
    _logger->info(TAG "Set get procedure address to %p", callback->get_proc_address);
    return true;
}

bool hc::Config::setSubsystemInfo(retro_subsystem_info const* info) {
    _logger->info(TAG "Setting subsystem information");

    for (; info->desc != nullptr; info++) {
        _logger->info(TAG "    desc  = %s", info->desc);
        _logger->info(TAG "    ident = %s", info->ident);
        _logger->info(TAG "    id    = %u", info->id);

        SubsystemInfo tempinfo;
        tempinfo.description = info->desc;
        tempinfo.name = info->ident;
        tempinfo.identifier = info->id;

        for (unsigned i = 0; i < info->num_roms; i++) {
            retro_subsystem_rom_info const* const rom = info->roms + i;

            _logger->info(TAG "        roms[%u].desc             = %s", i, rom->desc);
            _logger->info(TAG "        roms[%u].valid_extensions = %s", i, rom->valid_extensions);
            _logger->info(TAG "        roms[%u].need_fullpath    = %d", i, rom->need_fullpath);
            _logger->info(TAG "        roms[%u].block_extract    = %d", i, rom->block_extract);
            _logger->info(TAG "        roms[%u].required         = %d", i, rom->required);

            SubsystemInfo::Rom temprom;
            temprom.description = rom->desc;
            temprom.validExtensions = rom->valid_extensions;
            temprom.needFullpath = rom->need_fullpath;
            temprom.blockExtract = rom->block_extract;
            temprom.required = rom->required;

            for (unsigned j = 0; j < info->roms[i].num_memory; j++) {
                retro_subsystem_memory_info const* const memory = rom->memory + j;

                _logger->info(TAG "            memory[%u].type         = %u", j, memory->type);
                _logger->info(TAG "            memory[%u].extension    = %s", j, memory->extension);

                SubsystemInfo::Rom::Memory tempmem;
                tempmem.extension = memory->extension;
                tempmem.type = memory->type;

                temprom.memory.emplace_back(tempmem);
            }

            tempinfo.roms.emplace_back(temprom);
        }

        _subsystemInfo.emplace_back(tempinfo);
    }

    return true;
}

bool hc::Config::setMemoryMaps(retro_memory_map const* map) {
    _logger->info(TAG "Setting memory maps");

    auto const descriptors = static_cast<retro_memory_descriptor*>(malloc(map->num_descriptors * sizeof(retro_memory_descriptor)));

    if (descriptors == nullptr) {
        _logger->error(TAG "Out of memory setting memory descriptors");
        return false;
    }

    memcpy(descriptors, map->descriptors, sizeof(retro_memory_descriptor) * map->num_descriptors);
    
    if (!preprocessMemoryDescriptors(descriptors, map->num_descriptors)) {
        _logger->warn(TAG "Error processing memory descriptors, but will continue");
    }

    _logger->info(TAG "    ndx flags  ptr                offset   start    select   disconn  len      addrspace");

    for (unsigned i = 0; i < map->num_descriptors; i++) {
        retro_memory_descriptor const* const descriptor = descriptors + i;

        char flags[7];
        getFlags(flags, descriptor->flags);

        _logger->info(
            TAG "    %3u %s 0x%016" PRIxPTR " %08X %08X %08X %08X %08X %s",
            i, flags, (uintptr_t)descriptor->ptr, descriptor->offset, descriptor->start, descriptor->select,
            descriptor->disconnect, descriptor->len, descriptor->addrspace != nullptr ? descriptor->addrspace : ""
        );

        MemoryDescriptor tempdesc;
        tempdesc.flags = descriptor->flags;
        tempdesc.pointer = descriptor->ptr;
        tempdesc.offset = descriptor->offset;
        tempdesc.start = descriptor->start;
        tempdesc.select = descriptor->select;
        tempdesc.disconnect = descriptor->disconnect;
        tempdesc.length = descriptor->len;
        tempdesc.addressSpace = descriptor->addrspace != nullptr ? descriptor->addrspace : "";

        _memoryMap.emplace_back(tempdesc);
    }

    free(static_cast<void*>(descriptors));
    return true;
}

bool hc::Config::getUsername(char const** username) {
    static char const* const value = "hackcon";

    _logger->warn(TAG "Returning fixed username: %s", value);
    *username = value;
    return true;
}

bool hc::Config::getLanguage(unsigned* language) {
    _logger->warn(TAG "Returning fixed language English");
    *language = RETRO_LANGUAGE_ENGLISH;
    return true;
}

bool hc::Config::setSupportAchievements(bool supports) {
    _supportAchievements = supports;
    _logger->info(TAG "Set support achievement to %s", supports ? "true" : "false");
    return true;
}

bool hc::Config::setSerializationQuirks(uint64_t quirks) {
    _serializationQuirks = quirks;
    _logger->info(TAG "Set serialization quirks to %" PRIu64, quirks);
    return true;
}

bool hc::Config::getAudioVideoEnable(int* enabled) {
    *enabled = 3;
    return true;
}

bool hc::Config::getFastForwarding(bool* is) {
    *is = false;
    return true;
}
bool hc::Config::setCoreOptions(retro_core_option_definition const* options) {
    _logger->info(TAG "Setting core options");

    for (; options->key != nullptr; options++) {
        _logger->info(TAG "    key     = %s", options->key);
        _logger->info(TAG "    desc    = %s", options->desc);
        _logger->info(TAG "    info    = %s", options->info);
        _logger->info(TAG "    default = %s", options->default_value);

        CoreOption tempopt;
        tempopt.key = options->key;
        tempopt.label = options->desc;
        tempopt.sublabel = options->info != nullptr ? options->info : "";
        tempopt.selected = 0;
        tempopt.visible = true;

        for (unsigned i = 0; options->values[i].value != nullptr; i++) {
            retro_core_option_value const* const value = options->values + i;

            _logger->debug(TAG "        value = %s", value->value);
            _logger->debug(TAG "        label = %s", value->label);

            CoreOption::Value tempval;
            tempval.value = value->value;
            tempval.label = value->label != nullptr ? value->label : value->value;

            tempopt.values.emplace_back(tempval);
        }

        _coreMap.emplace(tempopt.key, _coreOptions.size());
        _coreOptions.emplace_back(tempopt);
    }

    return true;
}

bool hc::Config::setCoreOptionsIntl(retro_core_options_intl const* intl) {
    _logger->warn(TAG "Using English for the core options");
    return setCoreOptions(intl->us);
}

bool hc::Config::setCoreOptionsDisplay(retro_core_option_display const* display) {
    _logger->info(TAG "Setting core options display");

    for (; display->key != nullptr; display++) {
        _logger->info(TAG "    \"%s\" is %s", display->key, display->visible ? "visible" : "invisible");

        auto found = _coreMap.find(display->key);

        if (found != _coreMap.end()) {
            auto& option = _coreOptions[found->second];
            option.visible = display->visible;
        }
        else {
            _logger->warn(TAG "        key not found in core options");
        }
    }

    return true;
}

int hc::Config::push(lua_State* const L) {
    auto const self = static_cast<Config**>(lua_newuserdata(L, sizeof(Config*)));
    *self = this;

    if (luaL_newmetatable(L, "hc::Config")) {
        static luaL_Reg const methods[] = {
            {"getRootPath", l_getRootPath},
            {"getAutorunPath", l_getAutorunPath},
            {"getSystemPath", l_getSystemPath},
            {"getCoreAssetsPath", l_getCoreAssetsPath},
            {"getSavePath", l_getSavePath},
            {"getCoresPath", l_getCoresPath},
            {"getCoreOption", l_getCoreOption},
            {"setCoreOption", l_setCoreOption},
            {"getMemoryMap", l_getMemoryMap},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

hc::Config* hc::Config::check(lua_State* const L, int const index) {
    return *static_cast<Config**>(luaL_checkudata(L, index, "hc::Config"));
}

int hc::Config::l_getRootPath(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushlstring(L, self->_rootPath.c_str(), self->_rootPath.length());
    return 1;
}

int hc::Config::l_getAutorunPath(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushlstring(L, self->_autorunPath.c_str(), self->_autorunPath.length());
    return 1;
}

int hc::Config::l_getSystemPath(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushlstring(L, self->_systemPath.c_str(), self->_systemPath.length());
    return 1;
}

int hc::Config::l_getCoreAssetsPath(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushlstring(L, self->_coreAssetsPath.c_str(), self->_coreAssetsPath.length());
    return 1;
}

int hc::Config::l_getSavePath(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushlstring(L, self->_savePath.c_str(), self->_savePath.length());
    return 1;
}

int hc::Config::l_getCoresPath(lua_State* const L) {
    auto const self = check(L, 1);
    lua_pushlstring(L, self->_coresPath.c_str(), self->_coresPath.length());
    return 1;
}

int hc::Config::l_getCoreOption(lua_State* const L) {
    auto const self = check(L, 1);
    size_t length = 0;
    char const* key = luaL_checklstring(L, 2, &length);

    auto const found = self->_coreMap.find(std::string(key, length));

    if (found == self->_coreMap.end()) {
        return luaL_error(L, "unknown core option: \"%s\"", key);
    }

    CoreOption const& option = self->_coreOptions[found->second];

    lua_createtable(L, 0, 6);

    lua_pushlstring(L, option.key.c_str(), option.key.length());
    lua_setfield(L, -2, "key");
    lua_pushlstring(L, option.label.c_str(), option.label.length());
    lua_setfield(L, -2, "label");
    lua_pushlstring(L, option.sublabel.c_str(), option.sublabel.length());
    lua_setfield(L, -2, "sublabel");
    lua_pushinteger(L, option.selected);
    lua_setfield(L, -2, "selected");
    lua_pushboolean(L, option.visible);
    lua_setfield(L, -2, "visible");

    size_t const count = option.values.size();
    lua_createtable(L, count, 0);

    for (size_t i = 0; i < count; i++) {
        auto const& value = option.values[i];
        lua_createtable(L, 0, 2);

        lua_pushlstring(L, value.value.c_str(), value.value.length());
        lua_setfield(L, -2, "value");
        lua_pushlstring(L, value.label.c_str(), value.label.length());
        lua_setfield(L, -2, "label");

        lua_rawseti(L, -2, i + 1);
    }

    lua_setfield(L, -2, "values");
    return 1;
}

int hc::Config::l_setCoreOption(lua_State* const L) {
    auto const self = check(L, 1);
    size_t length = 0;
    char const* const key = luaL_checklstring(L, 2, &length);

    auto const found = self->_coreMap.find(std::string(key, length));

    if (found == self->_coreMap.end()) {
        return luaL_error(L, "unknown core option: \"%s\"", key);
    }

    CoreOption& option = self->_coreOptions[found->second];

    if (lua_isinteger(L, 3)) {
        lua_Integer const i = lua_tointeger(L, 3);
        size_t const index = i - 1;

        if (index >= option.values.size()) {
            return luaL_error(L, "invalid index into core option values: %I, \"%s\"", i, key);
        }

        option.selected = index;
        return 0;
    }
    else {
        size_t vlen = 0;
        char const* const val = luaL_checklstring(L, 3, &vlen);
        std::string const value(val, vlen);

        size_t const count = option.values.size();

        for (size_t i = 0; i < count; i++) {
            if (option.values[i].value == value) {
                option.selected = i;
                return 0;
            }
        }

        return luaL_error(L, "invalid value for core option: \"%s\", \"%s\"", val, key);
    }
}

int hc::Config::l_getMemoryMap(lua_State* const L) {
    auto const self = check(L, 1);

    size_t const count = self->_memoryMap.size();
    lua_createtable(L, count, 0);

    for (size_t i = 0; i < count; i++) {
        auto const& desc = self->_memoryMap[i];
        lua_createtable(L, 0, 8);

        char flags[7];
        getFlags(flags, desc.flags);

        lua_pushstring(L, flags);
        lua_setfield(L, -2, "flags");

        lua_pushlightuserdata(L, desc.pointer);
        lua_setfield(L, -2, "pointer");

        lua_pushinteger(L, desc.offset);
        lua_setfield(L, -2, "offset");

        lua_pushinteger(L, desc.start);
        lua_setfield(L, -2, "start");

        lua_pushinteger(L, desc.select);
        lua_setfield(L, -2, "select");

        lua_pushinteger(L, desc.disconnect);
        lua_setfield(L, -2, "disconnect");

        lua_pushinteger(L, desc.length);
        lua_setfield(L, -2, "length");

        lua_pushlstring(L, desc.addressSpace.c_str(), desc.addressSpace.size());
        lua_setfield(L, -2, "addressSpace");

        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}
