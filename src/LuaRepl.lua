return function(M)
    local string_format = string.format
    local table_concat = table.concat
    local table_remove = table.remove
    local global_load = load
    local global_xpcall = xpcall
    local debug_traceback = debug.traceback
    local global_tostring = tostring
    local global_tonumber = tonumber

    -- Rewrite show to apply tostring to all arguments
    local original_show = M.show
    M.show = function(...)
        local args = {...}

        for i = 1, #args do
            args[i] = global_tostring(args[i])
        end

        original_show(table_concat(args, ''))
    end

    -- Change the global print to use show
    print = function(...)
        local args = {...}

        for i = 1, #args do
            args[i] = global_tostring(args[i])
        end

        M.green()
        original_show(table_concat(args, '\t'))
    end

    -- Add a function to allow other modules to register special commands
    local commands = {}

    M.register = function(command, executor)
        if commands[command] then
            hc.logger:warn('Command "%s" already exists, overwriting', command)
        end

        commands[command] = executor
        hc.logger:info('Command "%s" registered', command)
    end

    local buffer = ''
    local history = {}
    local limit = 1000
    local cursor = 0

    local add = function(line)
        if cursor >= 1 and cursor <= #history and history[cursor] == line then
            table_remove(history, cursor)
        end

        history[#history + 1] = line

        while #history > limit do
            table_remove(history, 1)
        end

        cursor = 0
    end

    local execute = function(line)
        if line:sub(1, 1) == '!' then
            -- Registered command
            local command, args = line:match('!%s*([^%s]+)%s*(.*)')

            if not command then
                command, args = line:match('!%s*(.*)')
            end

            local executor = command and commands[command]

            if not executor then
                M.red()
                M.show('unknown command')
                return
            end

            local argtable = {}

            for arg in args:gmatch('([^,]+)') do
                argtable[#argtable + 1] = arg
            end

            return executor(table.unpack(argtable))
        end

        buffer = buffer .. (#buffer == 0 and '' or '; ') .. line
        local chunk, err = global_load('return ' .. buffer .. ';', '=stdin', 't')

        if chunk then
            M.green()
            M.show('> ', line)
            local res = {global_xpcall(chunk, debug_traceback)}

            if res[1] then
                table_remove(res, 1)
                M.green()
                M.show(table.unpack(res))

                add(line)
                buffer = ''
                return
            end
        end

        local chunk, err = global_load(buffer, '=stdin', 't')

        if not chunk then
            if err:sub(-5, -1) ~= '<eof>' then
                M.red()
                M.show(err)
                buffer = ''
                return
            end

            M.yellow()
            M.show('>> ', line)
            return
        end

        add(buffer)
        buffer = ''
        M.green()
        M.show('> ', line)

        local ok, err = global_xpcall(chunk, debug_traceback)

        if not ok then
            M.red()
            M.show(err)
        end
    end

    local hist = function(up)
        if #history == 0 then
            return
        end

        if up then
            if cursor == 0 then
                cursor = #history
            elseif cursor > 1 then
                cursor = cursor - 1
            else
                cursor = #history
            end
        else
            if cursor == 0 then
                cursor = 1
            elseif cursor < #history then
                cursor = cursor + 1
            else
                cursor = 1
            end
        end

        return history[cursor]
    end

    return execute, hist
end
