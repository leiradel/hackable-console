local hc = require 'hc'

hc.control:addConsole('Sony PlayStation (PCSX ReARMed)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'pcsx_rearmed_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onGameLoaded = function()
        local data, size = hc.control:getMemoryData('sram'), hc.control:getMemorySize('sram')
        hc.logger:info('Adding memory region for sram (%p, %d)', data, size)
        hc.config:addMemory('sram', 'System RAM', false, {data, 0, size})

        local data, size = hc.control:getMemoryData('save'), hc.control:getMemorySize('save')
        hc.logger:info('Adding memory region for save (%p, %d)', data, size)
        hc.config:addMemory('save', 'Save RAM', false, {data, 0, size})
    end
})
