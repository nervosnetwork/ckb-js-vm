local spack, sunpack = string.pack, string.unpack

local function get_file_size(path)
    local f = assert(io.open(path))
    local size = f:seek("end")
    f:close()
    return size
end

function tablelength(t)
    local count = 0
    for _ in pairs(t) do count = count + 1 end
    return count
end

local function append_to_stream(str, stream) stream:write(str) end

local function append_file_to_stream(path, stream)
    local infile = assert(io.open(path, "r"))
    local s = infile:read("*a")
    infile:close()
    append_to_stream(s, stream)
    stream:write('\0')
end

local function append_string_null_to_stream(str, stream)
    local s = spack("z", str)
    append_to_stream(s, stream)
end

local function append_integer_to_stream(num, stream)
    local s = spack("<I4", num)
    append_to_stream(s, stream)
end

local function pack(files, stream)
    local num = tablelength(files)
    append_integer_to_stream(num, stream)

    local offset = 0
    local length = 0
    for name, path in pairs(files) do
        print("packing file " .. path .. ' to ' .. name)
        append_integer_to_stream(offset, stream)
        length = #name + 1
        append_integer_to_stream(length, stream)
        offset = offset + length
        append_integer_to_stream(offset, stream)
        length = get_file_size(path)
        append_integer_to_stream(length, stream)
        offset = offset + length + 1
    end

    for name, path in pairs(files) do
        append_string_null_to_stream(name, stream)
        append_file_to_stream(path, stream)
    end
end

local function is_windows() return package.config:sub(1, 1) == "\\" end

-- I found no easy, platform-agnostic way to create a directory. Below is far from satisfactory.
-- Many cases are not covered, cf
-- https://stackoverflow.com/questions/22824905/how-good-is-using-q-in-lua-to-escape-shell-arguments
local function create_directory(dir)
    local command = is_windows() and ('mkdir %q'):format(dir) or
                        ('mkdir -p %q'):format(dir)
    local succeeded, msg = os.execute(command)
    if not succeeded then
        print('Executing ' .. command .. 'failed: ' .. msg)
        os.exit(1)
    end
end

local function copy_stream_to_file(directory, filename, stream, length)
    local path_separator = package.config:sub(1, 1)
    filename = filename:gsub('/', path_separator)
    local file = directory .. path_separator .. filename
    local dir = file:match("(.*[" .. path_separator .. "])")
    create_directory(dir)
    print('unpacking file ' .. filename .. ' to ' .. file)
    local f = assert(io.open(file, "w+"))
    local content = stream:read(length)
    f:write(content)
    f:close()
end

local function read_integer_from_stream(stream)
    local str = stream:read(4)
    return sunpack("<I4", str)
end

local function read_string_null_from_stream(stream, length)
    local str = stream:read(length)
    return sunpack("z", str)
end

local function unpack(directory, stream)
    local num_of_files = read_integer_from_stream(stream)

    local metadata = {}
    for i = 1, num_of_files do
        local metadatum = {}
        metadatum['file_name_offset'] = read_integer_from_stream(stream)
        metadatum['file_name_length'] = read_integer_from_stream(stream)
        metadatum['file_content_offset'] = read_integer_from_stream(stream)
        metadatum['file_content_length'] = read_integer_from_stream(stream)
        metadata[i] = metadatum
    end

    local blob_start = stream:seek()
    for _, metadatum in pairs(metadata) do
        stream:seek('set', blob_start + metadatum['file_name_offset'])
        local filename = read_string_null_from_stream(stream,
                                                      metadatum['file_name_length'])
        stream:seek('set', blob_start + metadatum['file_content_offset'])
        copy_stream_to_file(directory, filename, stream,
                            metadatum['file_content_length'])
    end
end

local function usage(msg)
    if msg ~= nil then print(msg) end
    print(arg[0] .. ' pack output_file [files] | ' .. arg[0] ..
              ' unpack input_file [directory]')
end

local function is_ideal_path(path)
    return path:find("^/") == nil and path:find("^[.][.]") == nil and
               path:find("^[a-zA-Z]:") == nil
end

local function normalize_relative_path(path)
    path = path:gsub("^[.]/", "")
    path = path:gsub("/[.]/", "/")
    return path
end

local function must_normalize_path(path)
    if not is_ideal_path(path) then
        error(
            'Either not a relative path or a relative path referring to ..: ' ..
                path)
    end
    return normalize_relative_path(path)
end

local function do_pack()
    if #arg == 1 then
        usage('You must specify the output file.')
        os.exit()
    end

    local outfile = arg[2]
    local stream = assert(io.open(outfile, "w+"))
    local files = {}
    local n = 0
    if #arg ~= 2 then
        for i = 3, #arg do
            n = n + 1
            file = arg[i]
            files[must_normalize_path(file)] = file
        end
    else
        for file in io.lines() do
            n = n + 1
            files[must_normalize_path(file)] = file
        end
    end
    if n == 0 then
        usage('You must at least specify one file to pack')
        os.exit()
    end
    pack(files, stream)
end

local function do_unpack()
    if #arg == 1 then
        usage('You must specify the input file when unpacking.')
        os.exit()
    end

    local infile = arg[2]
    local stream = assert(io.open(infile, "r"))
    local directory = '.'
    if #arg ~= 2 then directory = arg[3] end
    unpack(directory, stream)
end

if #arg == 0 or (arg[1] ~= 'pack' and arg[1] ~= 'unpack') then
    usage('Please specify whether to pack or unpack')
    os.exit()
end

if arg[1] == 'pack' then
    do_pack()
else
    do_unpack()
end
