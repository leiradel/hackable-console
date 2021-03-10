local hc = require 'hc'

hc.control:addConsole('Commodore 64 (C64 Chips)', {
    onConsoleLoaded = function()
        local path = hc.config:getCoresPath() .. 'c64chips_libretro.' .. hc.soExtension
        hc.control:loadCore(path)
    end
})
