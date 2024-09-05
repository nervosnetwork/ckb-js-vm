for line in io.lines() do
    if line:find("^Script log: ") ~= nil then
        local line_data = line:gsub("^Script log: ", "")
        if line_data ~= "" then
            print(line_data)
        end
    end
end
