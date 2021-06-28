local hc = require 'hc'

hc.control:addConsole('Nintendo Entertainment System (QuickNES)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'quicknes_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local data, size = hc.control:getMemoryData('sram'), hc.control:getMemorySize('sram')
        hc.logger:info('Adding memory region for sram (%p, %d)', data, size)
        hc.config:addMemory('sram', 'System RAM', false, {data, 0, size})
    end
})

hc.control:addConsole('Nintendo Entertainment System (FCEUmm)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'fceumm_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local data, size = hc.control:getMemoryData('sram'), hc.control:getMemorySize('sram')
        hc.logger:info('Adding memory region for sram (%p, %d)', data, size)
        hc.config:addMemory('sram', 'System RAM', false, {data, 0, size})
    end
})
