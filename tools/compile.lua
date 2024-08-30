for line in io.lines() do
    if line:find("^Script log: ") ~= nil then
        local cleaned_line = line:gsub("^Script log: ", "")
        print(cleaned_line)
    end
end
