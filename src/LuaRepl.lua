return function(show, green, yellow, red)
    local string_format = string.format
    local table_concat = table.concat
    local table_remove = table.remove
    local global_load = load
    local global_xpcall = xpcall
    local debug_traceback = debug.traceback
    local global_tostring = tostring
    local global_tonumber = tonumber

    -- Rewrite show to apply tostring to all arguments
    local original_show = show
    show = function(...)
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

        green()
        original_show(table_concat(args, '\t'))
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
    end

    local execute = function(line)
        buffer = buffer .. (#buffer == 0 and '' or '; ') .. line
        local chunk, err = global_load('return ' .. buffer .. ';', '=stdin', 't')

        if chunk then
            green()
            show('> ', line)
            local res = {global_xpcall(chunk, debug_traceback)}

            if res[1] then
                table_remove(res, 1)
                green()
                show(table.unpack(res))

                add(line)
                buffer = ''
                return
            end
        end

        local chunk, err = global_load(buffer, '=stdin', 't')

        if not chunk then
            if err:sub(-5, -1) ~= '<eof>' then
                red()
                show(err)
                buffer = ''
                return
            end

            yellow()
            show('>> ', line)
            return
        end

        add(buffer)
        buffer = ''
        green()
        show('> ', line)

        local ok, err = global_xpcall(chunk, debug_traceback)

        if not ok then
            red()
            show(err)
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
