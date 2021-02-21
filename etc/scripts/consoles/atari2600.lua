local hc = require 'hc'

hc.control:addConsole('Atari 2600 (Stella)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'stella_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function(path)
        local data, size = hc.control:getMemoryData('sram'), hc.control:getMemorySize('sram')
        hc.logger:info('Adding memory region for sram (%p, %d)', data, size)
        hc.memory:addRegion('System RAM', false, {data, 0x0080, size})
    end
})
