local hc = require 'hc'

hc.control:addConsole('ScummVM', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'scummvm_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end,

    onCoreLoaded = function()
        hc.config:setCoreOption('scummvm_mouse_speed', '1.0')
    end
})
