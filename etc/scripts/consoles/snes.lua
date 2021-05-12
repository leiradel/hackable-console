local hc = require 'hc'

hc.control:addConsole('Super Nintendo Entertainment System (Snes9x)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'snes9x_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local data, size = hc.control:getMemoryData('sram'), hc.control:getMemorySize('sram')
        hc.logger:info('Adding memory region for sram (%p, %d)', data, size)
        hc.config:addMemory('sram', 'System RAM', false, {data, 0x7e0000, size})

        local data, size = hc.control:getMemoryData('vram'), hc.control:getMemorySize('vram')
        hc.logger:info('Adding memory region for vram (%p, %d)', data, size)
        hc.config:addMemory('vram', 'Video RAM', false, {data, 0, size})
    end
})
