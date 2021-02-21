local hc = require 'hc'
local lfs = require 'lfs'

local logger, config = hc.logger, hc.config

local function run(file)
    local func, err = loadfile(file, 't')

    if not func then
        logger:error('%s', err)
    else
        local ok, err = pcall(func)

        if not ok then
            logger:error('%s', err)
        end
    end
end

local function recurse(path)
    logger:info('Recursing into %s', path)

    for file in lfs.dir(path) do
        local filePath = path .. '/' .. file
        local mode = lfs.attributes(filePath, 'mode')

        if mode == 'directory' and file ~= '.' and file ~= '..' then
            recurse(filePath)
        elseif mode == 'file' and file:sub(-4, -1) == '.lua' then
            logger:info('Found "%s", running...', filePath)
            run(filePath)
        end
    end
end

local path = config:getScriptsPath()
logger:info('Looking for Lua scripts in "%s"', path)

for file in lfs.dir(path) do
    local filePath = path .. '/' .. file
    local mode = lfs.attributes(filePath, 'mode')

    if mode == 'directory' and file ~= '.' and file ~= '..' then
        recurse(filePath)
    elseif mode == 'file' and file:sub(-4, -1) == '.lua' and file ~= 'autorun.lua' then
        logger:info('Found "%s", running...', filePath)
        run(filePath)
    end
end
