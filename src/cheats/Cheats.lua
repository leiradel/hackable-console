local table_remove = table.remove
local string_format = string.format

local cheats = {}
local onframe = {}

return function(M)
    M.start = function(memory, settings)
        cheats.memory = memory
        cheats.settings = settings
        cheats.first = memory:snapshot()
        cheats.current = cheats.first
        cheats.set = M.universal()
    end

    M.next = function(operator, operand)
        local snapshot = cheats.memory:snapshot()
        local nextset

        if operand then
            nextset = M.filter(snapshot, operator, operand, cheats.settings)
        else
            nextset = M.filter(snapshot, operator, cheats.current, cheats.settings)
        end

        cheats.current = snapshot
        cheats.set = cheats.set * nextset
        print(string_format('%d result(s)', cheats.set:size()))
    end

    M.list = function()
        for _, addr in cheats.set:elements() do
            print(string_format('%08x %02x %02x', addr, cheats.first:peek(addr), cheats.current:peek(addr)))
        end

        return cheats.set
    end

    return function()
        local count = #onframe
        local i = 1

        while i <= count do
            if onframe[i]() then
                i = i + 1
            else
                table_remove(onframe, i)
                count = count - 1
            end
        end
    end
end
