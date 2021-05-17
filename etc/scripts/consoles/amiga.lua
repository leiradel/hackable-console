local hc = require 'hc'

hc.control:addConsole('Commodore Amiga (P-UAE)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'puae_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local map = hc.config:getMemoryMap()

        hc.logger:info('Adding memory region for System RAM')
        hc.logger:info('    %p, %x, %d', map[1].pointer, map[1].start, map[1].length)

        hc.config:addMemory('sram', 'System RAM', false, {map[1].pointer, map[1].start, map[1].length})
    end
})
