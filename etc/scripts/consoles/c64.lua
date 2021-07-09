local hc = require 'hc'

hc.control:addConsole('Commodore 64 (C64 Chips)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'c64chips_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end
})

hc.control:addConsole('Commodore 64 (Vice Fast)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'vice_x64_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local map = hc.config:getMemoryMap()
        local blocks = {}

        hc.logger:info('Adding memory region for System RAM')

        for _, desc in pairs(map) do
            blocks[#blocks + 1] = {desc.pointer, desc.start, desc.length}
            hc.logger:info('    %p, %x, %d', desc.pointer, desc.start, desc.length)
        end

        hc.config:addMemory('sram', 'System RAM', false, table.unpack(blocks))
    end
})

hc.control:addConsole('Commodore 64 (Vice Accurate)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'vice_x64sc_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local map = hc.config:getMemoryMap()
        local blocks = {}

        hc.logger:info('Adding memory region for System RAM')

        for _, desc in pairs(map) do
            blocks[#blocks + 1] = {desc.pointer, desc.start, desc.length}
            hc.logger:info('    %p, %x, %d', desc.pointer, desc.start, desc.length)
        end

        hc.config:addMemory('sram', 'System RAM', false, table.unpack(blocks))
    end
})
