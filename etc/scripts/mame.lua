hc.logger:info('Adding MAME-like commands to Lua Repl')

hc.repl.register(
    'cheatinit', function(settings, memory)
        settings = settings or 'ub'
        memory = (memory and #memory ~= 0) and memory or 'sram'
        hc.cheats.start(hc.memory[memory], settings)
    end
)

hc.repl.register(
    'cheatnext', function(operator, operand)
        if operator == 'equal' or operator == 'eq' then
            operator = '=='
        elseif operator == 'notequal' or operator == 'ne' then
            operator = '~='
        elseif operator == 'decrease' or operator == 'de' or operator == '+' then
            operator = '<'
        elseif operator == 'increase' or operator == 'in' or operator == '-' then
            operator = '>'
        elseif operator == 'decreaseorequal' or operator == 'deeq' then
            operator = '<='
        elseif operator == 'increaseorequal' or operator == 'ineq' then
            operator = '>='
        else
            error(string.format('invalid operator "%s"', operator or '(nil)'))
        end

        hc.cheats.next(operator, operand)
    end
)

hc.repl.register(
    'cheatlist', function()
        hc.cheats.list()
    end
)
