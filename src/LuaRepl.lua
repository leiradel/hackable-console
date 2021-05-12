return function(term)
    local term_print = term.print
    local string_format = string.format
    local table_concat = table.concat
    local table_remove = table.remove
    local global_load = load
    local global_xpcall = xpcall
    local debug_traceback = debug.traceback
    local global_tostring = tostring
    local global_tonumber = tonumber

    -- Patch term.print to apply tostring to all arguments
    term.print = function(self, ...)
        local args = {...}

        for i = 1, #args do
            args[i] = global_tostring(args[i])
        end

        term_print(self, table_concat(args, ''))
        return self
    end

    -- Change the global print to use term.print
    print = function(...)
        local args = {...}

        for i = 1, #args do
            args[i] = global_tostring(args[i])
        end

        term:green():print(table_concat(args, '\t'))
    end

    -- Return a function that receives and runs commands typed in Lua
    local buffer = ''
    local history = {}
    local limit = 100

    return function(line)
        if line == 'history' then
            for i = 1, #history do
                term:print(string_format('%3d %s', i, history[i]))
            end

            return
        elseif line:sub(1, 1) == '!' then
            local i = #line == 1 and #history or global_tonumber(line:sub(2, -1))

            if i >= 1 and i <= #history then
                local line = history[i]
                table_remove(history, i)
                return line
            end

            return
        end

        buffer = buffer .. (#buffer == 0 and '' or '; ') .. line
        local chunk, err = global_load('return ' .. buffer .. ';', '=stdin', 't')

        if chunk then
            term:green():print('> ', line)
            local res = {global_xpcall(chunk, debug_traceback)}

            if res[1] then
                table_remove(res, 1)
                term:green():print(table.unpack(res))

                buffer = ''
                history[#history + 1] = line

                while #history > limit do
                    table_remove(history, 1)
                end

                return
            end
        end

        local chunk, err = global_load(buffer, '=stdin', 't')

        if not chunk then
            if err:sub(-5, -1) ~= '<eof>' then
                term:red():print(err)
                buffer = ''
                return
            end

            term:yellow():print('>> ', line)
            return
        end

        history[#history + 1] = buffer

        while #history > limit do
            table_remove(history, 1)
        end

        buffer = ''
        term:green():print('> ', line)
        local ok, err = global_xpcall(chunk, debug_traceback)

        if not ok then
            term:red():print(err)
        end
    end
end
