lfs = require 'lfs'

-- Get the host system value on first run
extension_to_system = dll:'windows', dylib:'macos', so:'linux'

-- Get the binary module extension this lua build is using
module_extension = string.match package.cpath, '?%.(%w+)'

-- Save the host system value
host_system = extension_to_system[module_extension] or 'unknown'

--!
--! We use the lua build configuration to get the host system, this works in most cases but some issues may arise on MacOS.
os.host = -> host_system

-- Returns true if the path is a directory
os.isfile = (path) -> (lfs.attributes path, 'mode') == 'file'

-- Returns true if the path is a file
os.isdir = (path) -> (lfs.attributes path, 'mode') == 'directory'

-- Creates the directory at the given path
os.mkdir = (path) -> lfs.mkdir path

-- Creates all directories in the path
os.mkdirs = (path) ->
    return true if os.isdir path

    result = true
    partial = ''
    for name in path\gmatch '[^/\\]+'
        partial ..= "#{name}/"
        unless os.isdir partial
            result and= (os.mkdir partial) ~= nil
    result
