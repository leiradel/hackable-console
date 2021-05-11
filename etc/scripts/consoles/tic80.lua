local hc = require 'hc'

hc.control:addConsole('TIC-80', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'tic80_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local data, size = hc.control:getMemoryData('sram'), hc.control:getMemorySize('sram')
        hc.logger:info('Adding memory region for sram (%p, %d)', data, size)
        hc.config:addMemory('System RAM', false, {data, 0xff0000, size})
    end
})
