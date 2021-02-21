local hc = require 'hc'
local lfs = require 'lfs'

local logger, config = hc.logger, hc.config

local scriptsPath = config:getScriptsPath()
logger:info('Looking for Lua scripts in "%s"', scriptsPath)

for file in lfs.dir(scriptsPath) do
    if file:sub(-4, -1) == '.lua' and file ~= 'autorun.lua' then
        logger:info('Found "%s", running...', file)

        local func, err = loadfile(scriptsPath .. file, 't')

        if not func then
            logger:error('%s', err)
        else
            local ok, err = pcall(func)

            if not ok then
                logger:error('%s', err)
            end
        end
    end
end
