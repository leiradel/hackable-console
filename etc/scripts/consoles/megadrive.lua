local hc = require 'hc'

hc.control:addConsole('Mega Drive (PicoDrive)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'picodrive_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function(path)
        local data, size = hc.control:getMemoryData('sram'), hc.control:getMemorySize('sram')
        hc.logger:info('Adding memory region for sram (%p, %d)', data, size)
        hc.config:addMemory('System RAM', false, {data, 0xff0000, size})
    end
})
