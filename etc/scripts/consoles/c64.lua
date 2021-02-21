local hc = require 'hc'

local config = hc.config
local logger = hc.logger

hc.addConsole {
    name = 'Commodore 64 (Vice accurate)',
    path = config:getCoresPath() .. 'vice_x64sc_libretro.' .. hc.soExtension,

    onLoadCore = function()
        return true
    end,

    onCoreLoaded = function(core)
        return true
    end,

    onLoadGame = function(core, path)
        return true
    end,

    onGameLoaded = function(core)
        -- Vice only exposes memory after a few frames
        while true do
            local data, size = core:getMemoryData('sram'), core:getMemorySize('sram')

            if size ~= 0 then
                logger:info('Adding memory region for sram (%p, %d)', data, size)
                hc.addMemoryRegion('System RAM', data, 0x0000, size, false)
                return true
            end

            logger:warn('Waiting for core to expose memory')
            core:run()
        end
    end
}

hc.addConsole {
    name = 'Commodore 64 (Vice fast)',
    path = config:getCoresPath() .. 'vice_x64_libretro.' .. hc.soExtension,

    onLoadCore = function()
        return true
    end,

    onCoreLoaded = function(core)
        return true
    end,

    onLoadGame = function(core, path)
        return true
    end,

    onGameLoaded = function(core)
        -- Vice only exposes memory after a few frames
        while true do
            local data, size = core:getMemoryData('sram'), core:getMemorySize('sram')

            if size ~= 0 then
                logger:info('Adding memory region for sram (%p, %d)', data, size)
                hc.addMemoryRegion('System RAM', data, 0x0000, size, false)
                return true
            end

            logger:warn('Waiting for core to expose memory')
            core:run()
        end
    end
}

hc.addConsole {
    name = 'Commodore 64 (Frodo)',
    path = config:getCoresPath() .. 'frodo_libretro.' .. hc.soExtension,

    onLoadCore = function()
        return true
    end,

    onCoreLoaded = function(core)
        return true
    end,

    onLoadGame = function(core, path)
        return true
    end,

    onGameLoaded = function(core)
        return true
    end
}
