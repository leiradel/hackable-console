local hc = require 'hc'

hc.control:addConsole('ZX Spectrum (Fuse)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'fuse_libretro.' .. hc.soExtension
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

        hc.config:addMemory('System RAM', false, table.unpack(blocks))
    end
})

hc.control:addConsole('ZX Spectrum (Chips)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'zx48k_libretro.' .. hc.soExtension
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

        hc.config:addMemory('System RAM', false, table.unpack(blocks))
    end
})
