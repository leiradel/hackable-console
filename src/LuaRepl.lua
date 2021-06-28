return function(M)
    local tonumber = tonumber

    -- Rewrite show to apply tostring to all arguments
    local original_show = M.show
    M.show = function(...)
        local args = {...}

        for i = 1, #args do
            args[i] = tostring(args[i])
        end

        original_show(table.concat(args, ''))
    end

    -- Change the global print to use show
    print = function(...)
        local args = {...}

        for i = 1, #args do
            args[i] = tostring(args[i])
        end

        M.green()
        original_show(table.concat(args, '\t'))
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
            table.remove(history, cursor)
        end

        history[#history + 1] = line

        while #history > limit do
            table.remove(history, 1)
        end

        cursor = 0
    end

    local run_command = function(line)
        local command, args = line:match('!%s*([^%s]+)%s*(.*)')

        if not command then
            command, args = line:match('!%s*(.*)')
        end

        local chunk = command and commands[command]

        if not chunk then
            M.red()
            M.show('unknown command')
            return
        end

        local argtable = {}

        for arg in args:gmatch('([^,]+)') do
            argtable[#argtable + 1] = arg:match("^%s*(.-)%s*$")
        end

        hc.logger:debug('Running command: %s(%s)', command, table.concat(argtable, ', '))

        M.green()
        M.show('> ', line)

        local res = {xpcall(chunk, debug.traceback, table.unpack(argtable))}

        if res[1] then
            table.remove(res, 1)
            M.green()
            M.show(#res == 0 and 'nil' or table.unpack(res))
            add(line)
        else
            M.red()
            M.show(res[2])
        end
    end

    local try_expression = function(line)
        local expr = buffer .. (#buffer == 0 and '' or '; ') .. line
        local chunk, err = load('return ' .. expr .. ';', '=stdin', 't')

        if chunk then
            M.green()
            M.show('> ', line)
            local res = {xpcall(chunk, debug.traceback)}

            if res[1] then
                table.remove(res, 1)
                M.green()
                M.show(#res == 0 and 'nil' or table.unpack(res))
                add(expr)
                buffer = ''
                return true
            end
        end

        return false
    end

    local run_statement = function(line)
        local stmt = buffer .. (#buffer == 0 and '' or '; ') .. line
        local chunk, err = load(stmt, '=stdin', 't')

        if chunk then
            M.green()
            M.show('> ', line)
            local ok, err = xpcall(chunk, debug.traceback)

            if ok then
                add(stmt)
                buffer = ''
                return true
            else
                M.red()
                M.show(err)
            end
        else
            if err:sub(-5, -1) ~= '<eof>' then
                M.red()
                M.show(err)
                buffer = ''
            else
                M.yellow()
                M.show('>> ', line)
                buffer = stmt
            end
        end

        return false
    end

    local execute = function(line)
        if line:sub(1, 1) == '!' then
            run_command(line)
        elseif not try_expression(line) then
            run_statement(line)
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
