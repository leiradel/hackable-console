local hc = require 'hc'

hc.control:addConsole('DOS (DOSBox Pure)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'dosbox_pure_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onCoreLoaded = function()
        hc.config:setCoreOption('dosbox_pure_mouse_speed_factor', '2.0')
        hc.config:setCoreOption('dosbox_pure_mouse_speed_factor_x', '1.0')
    end,

    onGameLoaded = function()
        local map = hc.config:getMemoryMap()
        local pointer, start, length = map[1].pointer, map[1].start, map[1].length

        hc.logger:info('Adding memory region for System RAM')
        hc.logger:info('    %p, %x, %d', map[1].pointer, map[1].start, map[1].length)
        hc.logger:info('    %p, %x, %d', map[2].pointer, map[2].start, map[2].length)
        hc.logger:info('    %p, %x, %d', map[3].pointer, map[3].start, map[3].length)

        hc.memory:addMemory('sram', 'System RAM', false,
            {map[1].pointer, map[1].start, map[1].length},
            {map[2].pointer, map[2].start, map[2].length},
            {map[3].pointer, map[3].start, map[3].length}
        )
    end
})
