#pragma once

#include "Desktop.h"
#include "Scriptable.h"
#include "Memory.h"

#include <lrcpp/Components.h>

extern "C" {
    #include <lua.h>
}

#include <string>
#include <vector>
#include <unordered_map>

namespace hc {
    class CoreMemory : public Memory {
    public:
        CoreMemory(char const* name, bool readonly);
        virtual ~CoreMemory() {}

        bool addBlock(void* data, uint64_t offset, uint64_t base, uint64_t size);

        // Memory
        virtual char const* name() const override { return _name.c_str(); }
        virtual uint64_t base() const override { return _base; }
        virtual uint64_t size() const override { return _size; }
        virtual bool readonly() const override { return _readonly; }
        virtual uint8_t peek(uint64_t address) const override;
        virtual void poke(uint64_t address, uint8_t value) override;

    protected:
        struct Block {
            Block(void* data, uint64_t offset, uint64_t size) : data(data), offset(offset), size(size) {}
            void* data;
            uint64_t offset;
            uint64_t size;
        };

        std::string _name;
        uint64_t _base;
        uint64_t _size;
        bool _readonly;
        std::vector<Block> _blocks;
    };

    class Config: public View, public Scriptable, public lrcpp::Config {
    public:
        Config(Desktop* desktop, MemorySelector* memorySelector);
        virtual ~Config() {}

        bool init();
        bool getSupportNoGame() const;
        std::string const& getRootPath() const;
        std::string const& getAutorunPath() const;
        retro_proc_address_t getExtension(char const* const symbol);

        static Config* check(lua_State* const L, int const index);

        // hc::View
        virtual char const* getTitle() override;
        virtual void onGameLoaded() override;
        virtual void onDraw() override;
        virtual void onCoreUnloaded() override;
        virtual void onQuit() override;

        // hc::Scriptable
        virtual int push(lua_State* const L) override;

        // lrcpp::Config
        virtual bool setPerformanceLevel(unsigned level) override;
        virtual bool getSystemDirectory(char const** directory) override;
        virtual bool getVariable(retro_variable* variable) override;
        virtual bool getVariableUpdate(bool* const updated) override;
        virtual bool setSupportNoGame(bool const supports) override;
        virtual bool getLibretroPath(char const** path) override;
        virtual bool getCoreAssetsDirectory(char const** directory) override;
        virtual bool getSaveDirectory(char const** directory) override;
        virtual bool setProcAddressCallback(retro_get_proc_address_interface const* callback) override;
        virtual bool setSubsystemInfo(retro_subsystem_info const* info) override;
        virtual bool setMemoryMaps(retro_memory_map const* map) override;
        virtual bool getUsername(char const** username) override;
        virtual bool getLanguage(unsigned* language) override;
        virtual bool setSupportAchievements(bool supports) override;
        virtual bool setSerializationQuirks(uint64_t quirks) override;
        virtual bool getAudioVideoEnable(int* enabled) override;
        virtual bool getFastForwarding(bool* is) override;
        virtual bool setCoreOptions(retro_core_option_definition const* options) override;
        virtual bool setCoreOptionsIntl(retro_core_options_intl const* intl) override;
        virtual bool setCoreOptionsDisplay(retro_core_option_display const* display) override;

    protected:
        static int l_getRootPath(lua_State* const L);
        static int l_getAutorunPath(lua_State* const L);
        static int l_getSystemPath(lua_State* const L);
        static int l_getCoreAssetsPath(lua_State* const L);
        static int l_getSavePath(lua_State* const L);
        static int l_getCoresPath(lua_State* const L);
        static int l_getCoreOption(lua_State* const L);
        static int l_setCoreOption(lua_State* const L);
        static int l_getMemoryMap(lua_State* const L);
        static int l_addMemory(lua_State* const L);

        struct SubsystemInfo {
            struct Rom {
                struct Memory {
                    std::string extension;
                    unsigned type;
                };

                std::string description;
                std::string validExtensions;
                bool needFullpath;
                bool blockExtract;
                bool required;
                std::vector<Memory> memory;
            };

            std::string description;
            std::string name;
            std::vector<Rom> roms;
            unsigned identifier;
        };

        struct MemoryDescriptor {
            uint64_t flags;
            void* pointer;
            size_t offset;
            size_t start;
            size_t select;
            size_t disconnect;
            size_t length;
            std::string addressSpace;
        };

        struct CoreOption {
            struct Value {
                std::string value;
                std::string label;
            };

            std::string key;
            std::string label;
            std::string sublabel;

            std::vector<Value> values;
            size_t selected;
            bool visible;
        };

        MemorySelector* _memorySelector;

        std::string _rootPath;
        std::string _autorunPath;
        std::string _systemPath;
        std::string _coreAssetsPath;
        std::string _savePath;
        std::string _coresPath;

        unsigned _performanceLevel;
        bool _supportsNoGame;
        retro_get_proc_address_t _getCoreProc;
        bool _supportAchievements;
        uint64_t _serializationQuirks;

        std::vector<SubsystemInfo> _subsystemInfo;
        std::vector<MemoryDescriptor> _memoryMap;

        std::vector<CoreOption> _coreOptions;
        std::unordered_map<std::string, size_t> _coreMap;
        bool _optionsUpdated;
    };
}
