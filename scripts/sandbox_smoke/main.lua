--[[
 Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
]]

--[[
    Sandbox smoke test - run by scripts/sandbox_smoke.py (and ci.yml, every platform). Pins
    plans/MASTER_PLAN.md item C8: the path rule (reading: the save directory, then the game's own files;
    writing: the save directory) through Lua's own functions and Scarab's loaders, the sandbox's io/os/
    require/loadfile/load, and that nothing outside the game's files and its save directory is reachable.

    SMOKE_SCENARIO picks what runs:
        rule        the whole suite, with a save directory (--save-dir). SMOKE_GAME_DIR is where the game's
                    own files are as a plain path ("scripts/sandbox_smoke/" loose, "" from a .zip),
                    SMOKE_APPDIR_FILE a file under APP_DIR, SMOKE_ZIP=1 from a .zip (the launch folder
                    then holds outside.txt, which must not be readable)
        nodir       no save directory: reading works, writing is refused
        switch      switch.json names save folder "a"; app_set_save_directory moves to "b": the old
                    folder's files must no longer be found, by a plain path or an APP_DIR one
        nosandbox   --no-sandbox: Lua's own functions are back

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure and
    never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __ERRNO_NOT_FOUND <const> = 2      -- ENOENT, the code Lua's own io.open gives for a missing file
local __NUMBER_HEX      <const> = 16     -- numbers.txt's third number, 0x10
local __NUMBER_FLOAT    <const> = 2.5    -- numbers.txt's second number
local __RPLUS_OFFSET    <const> = 2      -- where the r+ test writes into "abcdef"

local scenario = os.getenv( "SMOKE_SCENARIO" ) or "(unset)"
local gameDir  = os.getenv( "SMOKE_GAME_DIR" ) or ""
local failures = {}

-- Record a failure message, prefixed with the scenario's name, when `ok` is false. Every check runs; the
-- failures are printed together at the end.
local function check( ok, message )
    if not ok then failures[#failures + 1] = scenario .. ": " .. message end
end

-- A failing call returns nil and a message containing `needle`.
local function expect_fail( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- Write a progress line to stderr, unbuffered, so a run that crashes shows the last step it reached (the results
-- themselves are printed only at the end).
local function step( name )
    io.stderr:write( "SANDBOX SMOKE STEP " .. scenario .. ": " .. name .. "\n" )
end

-- The whole contents of a file through io.open, or nil when it does not open.
local function read_all( path )
    local file = io.open( path )
    if not file then return nil end
    local text = file:read( "a" )
    file:close()
    return text
end

-- Write a whole file through io.open; true if it was written.
local function write_all( path, text )
    local file, err = io.open( path, "w" )
    if not file then return nil, err end
    file:write( text )
    return file:close()
end

-- A path of the game's own files, as a module name for require ("scripts.sandbox_smoke.mod").
local function module_name( name )
    return ( gameDir .. name ):gsub( "/", "." )
end

-- The engine asks Lua to claim each stage id an sp_load_stage queues: claim any, so none can stop the run.
function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- The rule scenario's reads: the game's own files, and what each read format returns.
local function check_reading()
    check( read_all( gameDir .. "game.txt" ) == "game\n", "a game file reads" )
    check( io.open( APP_DIR .. os.getenv( "SMOKE_APPDIR_FILE" ) ) ~= nil, "a file under APP_DIR reads" )

    local lines = {}
    for line in io.lines( gameDir .. "lines.txt" ) do lines[#lines + 1] = line end
    check( table.concat( lines, "," ) == "one,two,three", "io.lines reads every line" )
    check( select( "#", io.lines( gameDir .. "lines.txt" ) ) == 4, "io.lines( name ) returns four values (Lua 5.4)" )

    local numbers = assert( io.open( gameDir .. "numbers.txt" ) )
    local a, b, c = numbers:read( "n", "n", "n" )
    check( a == 1 and math.type( a ) == "integer" and b == __NUMBER_FLOAT and c == __NUMBER_HEX, "read( 'n' ) reads numbers" )
    check( numbers:read( "n" ) == nil, "read( 'n' ) at the end is nil" )
    numbers:close()

    local file = assert( io.open( gameDir .. "lines.txt" ) )
    check( file:read( "L" ) == "one\n", "read( 'L' ) keeps the newline" )
    check( file:read( 2 ) == "tw", "read( count )" )
    check( file:read( "l" ) == "o", "read( 'l' ) after a count" )
    check( file:seek( "cur" ) == 8 and file:seek( "set", 0 ) == 0, "seek" )
    check( file:read( "a" ) == "one\ntwo\nthree\n" and file:read( "a" ) == "" and file:read( 0 ) == nil, "read( 'a' ) and the end" )
    check( io.type( file ) == "file" and tostring( file ):find( "^file %(" ) ~= nil, "io.type and tostring of an open file" )
    check( file:close() == true and io.type( file ) == "closed file" and tostring( file ) == "file (closed)", "a closed file" )
    check( not pcall( file.read, file ), "reading a closed file raises" )
    check( io.type( io.stdout ) == "file" and io.type( 1 ) == nil, "io.type of Lua's own file and of a number" )

    local value, message, code = io.open( gameDir .. "missing.txt" )
    expect_fail( "a missing file", "missing.txt", value, message )
    check( code == __ERRNO_NOT_FOUND, "a missing file's code is ENOENT" )
    check( not pcall( io.open, gameDir .. "game.txt", "rw" ), "an invalid mode raises, as in Lua" )
    check( not pcall( io.lines, gameDir .. "missing.txt" ), "io.lines of a missing file raises, as in Lua" )
end

-- The rule scenario's writes: into the save directory, read back first, and every write mode.
local function check_writing()
    check( write_all( gameDir .. "game.txt", "saved\n" ) == true, "writing a game file's path" )
    check( read_all( gameDir .. "game.txt" ) == "saved\n", "the save is read before the game's file" )
    check( read_all( APP_DIR .. gameDir .. "game.txt" ) == "saved\n", "an APP_DIR path reads the save first (1b)" )
    check( write_all( APP_DIR .. "appdir.txt", "via APP_DIR" ) == true, "writing an APP_DIR path" )
    check( read_all( "appdir.txt" ) == "via APP_DIR", "an APP_DIR path is written under the part after APP_DIR" )

    local appendFile = assert( io.open( gameDir .. "append.txt", "a" ) )
    appendFile:write( "+more" )
    appendFile:close()
    check( read_all( gameDir .. "append.txt" ) == "base+more", "'a' starts from the game's file" )

    local rplus = assert( io.open( gameDir .. "rplus.txt", "r+" ) )
    rplus:seek( "set", __RPLUS_OFFSET )
    rplus:write( "XY" )
    rplus:close()
    check( read_all( gameDir .. "rplus.txt" ) == "abXYef", "'r+' writes in place" )

    local wplus = assert( io.open( "wplus.txt", "w+" ) )
    check( wplus:write( "abc", 1, __NUMBER_FLOAT ) == wplus, "write returns the file" )
    wplus:seek( "set" )
    check( wplus:read( "a" ) == "abc12.5", "'w+' reads back, numbers written as Lua writes them" )
    wplus:close()

    io.output( "out.txt" )
    io.write( "hello" )
    io.close()
    io.output( io.stdout )
    check( read_all( "out.txt" ) == "hello", "io.output / io.write / io.close" )
    io.input( gameDir .. "lines.txt" )
    check( io.read( "l" ) == "one", "io.input / io.read" )
    io.input( io.stdin )

    do
        local closing <close> = assert( io.open( "tbc.txt", "w" ) )
        closing:write( "tbc" )
    end
    check( read_all( "tbc.txt" ) == "tbc", "a to-be-closed file is written" )

    local collected = assert( io.open( "gc.txt", "w" ) )
    collected:write( "gc" )
    collected = nil         -- lint: allow lua-unused (dropped on purpose: the collector writes it back)
    collectgarbage()
    collectgarbage()
    check( read_all( "gc.txt" ) == "gc", "a collected file is written" )

    local temp = io.tmpfile()
    temp:write( "temporary" )
    temp:seek( "set" )
    check( temp:read( "a" ) == "temporary" and temp:close() == true, "io.tmpfile" )
end

-- The rule scenario's os functions: inside the save directory only.
local function check_os()
    check( os.remove( gameDir .. "game.txt" ) == true, "os.remove of the save" )
    check( read_all( gameDir .. "game.txt" ) == "game\n", "the game's file is back once the save is removed" )
    expect_fail( "os.remove of a game file", "No such file", os.remove( gameDir .. "lines.txt" ) )
    check( read_all( gameDir .. "lines.txt" ) ~= nil, "a game file can't be removed" )

    check( write_all( "r1.txt", "renamed" ) == true and os.rename( "r1.txt", "r2.txt" ) == true, "os.rename" )
    check( read_all( "r1.txt" ) == nil and read_all( "r2.txt" ) == "renamed", "os.rename moved the file" )
    expect_fail( "os.rename of a game file", "No such file", os.rename( gameDir .. "lines.txt", "x.txt" ) )

    local tempName = os.tmpname()
    check( type( tempName ) == "string" and read_all( tempName ) == "", "os.tmpname creates an empty file" )
    check( os.remove( tempName ) == true, "and it can be removed" )

    check( os.execute() == false, "os.execute(): no shell" )
    expect_fail( "os.execute", "not available in the sandbox", os.execute( "echo x" ) )
    expect_fail( "io.popen", "not available in the sandbox", io.popen( "echo x" ) )
end

-- The rule scenario's code loading: require, loadfile, load and dofile, by the rule and text only.
local function check_code()
    check( package.path == "?.lua;?/init.lua" and package.cpath == "", "package.path and cpath" )
    check( require( module_name( "mod" ) ) == "game module", "require a game module" )
    check( package.searchpath( module_name( "mod" ), package.path ) == gameDir .. "mod.lua", "package.searchpath" )

    local ok, message = pcall( require, "no_such_module" )
    check( not ok and message:find( "no C modules in the sandbox", 1, true ) and message:find( "no file 'no_such_module.lua'", 1, true ),
           "require of a missing module: " .. tostring( message ) )
    local loaded, loadMessage, where = package.loadlib( "x", "y" )
    check( loaded == nil and loadMessage:find( "not available", 1, true ) and where == "absent", "package.loadlib" )

    check( write_all( gameDir .. "shadow_mod.lua", "return 'saved'" ) == true, "save a module's copy" )
    check( require( module_name( "shadow_mod" ) ) == "saved", "require finds the saved module first" )
    check( dofile( gameDir .. "shadow_mod.lua" ) == "saved", "dofile finds the saved file first" )

    check( write_all( gameDir .. "broken_mod.lua", "return (" ) == true, "save a module that does not compile" )
    local loadedBroken, brokenMessage = pcall( require, module_name( "broken_mod" ) )
    check( not loadedBroken and tostring( brokenMessage ):find( "error loading module", 1, true ) ~= nil,
           "require raises Lua's error for a module that does not compile: " .. tostring( brokenMessage ) )

    check( loadfile( gameDir .. "mod.lua" )() == "game module", "loadfile" )
    check( loadfile( gameDir .. "envmod.lua", "t", { x = 5 } )() == 5, "loadfile with an environment" )
    expect_fail( "loadfile binary mode", "binary chunks are not available", loadfile( gameDir .. "mod.lua", "b" ) )
    expect_fail( "loadfile missing", "cannot open", loadfile( gameDir .. "missing.lua" ) )
    check( load( "return 1 + 1" )() == 2, "load a string" )

    local pieces = { "return ", "40 + ", "2" }
    local index  = 0
    check( load( function() index = index + 1 return pieces[index] end )() == 42, "load from a reader function" )
    expect_fail( "load bytecode", "attempt to load a binary chunk", load( string.dump( function() return 1 end ) ) )
    expect_fail( "load binary mode", "binary chunks are not available", load( "return 1", "x", "b" ) )

    check( write_all( gameDir .. "data.json", '{ "who": "saved" }' ) == true, "save a data file's copy" )
    check( ( load_json( gameDir .. "data.json" ) or {} ).who == "saved", "load_json follows the rule too" )
    check( save_exists( gameDir .. "lines.txt" ) == false, "a game file is not a save (save_* are saves-only)" )
end

-- The rule scenario's refusals: nothing outside the game's files and the save directory.
local function check_refusals()
    expect_fail( "a host path read", "host path", io.open( "hpath:///etc/hosts" ) )
    expect_fail( "a host path write", "host path", io.open( "hpath:///tmp/scarab-sandbox-smoke.txt", "w" ) )
    expect_fail( "climbing out", "climbs out", io.open( "../../scarab-sandbox-smoke.txt", "w" ) )
    check( io.open( "../../README.md" ) == nil, "reading above the game's files" )
    expect_fail( "os.remove of a host path", "host path", os.remove( "hpath:///tmp/x" ) )
    check( not pcall( dofile, "hpath:///etc/hosts" ), "dofile of a host path" )
    check( load_json( "hpath:///etc/hosts" ) == nil, "load_json of a host path" )
    check( pack_list_directory == nil, "no pack_* in a game" )

    local swapped, swapMessage = debug.setmetatable( io.stderr, getmetatable( io.tmpfile() ) )
    expect_fail( "debug.setmetatable on a file", "not available in the sandbox", swapped, swapMessage )
    check( getmetatable( io.stderr ) ~= getmetatable( io.tmpfile() ), "a file kept its own metatable" )
    local plain = {}
    check( debug.setmetatable( plain, { __index = { answer = 42 } } ) == plain and plain.answer == 42, "debug.setmetatable on a table is Lua's" )
    expect_fail( "debug.getregistry", "not available in the sandbox", debug.getregistry() )

    for _, fn in ipairs( { io.open, io.lines, io.popen, os.remove, os.execute, loadfile, load, package.loadlib,
                           debug.setmetatable, debug.getregistry } ) do
        check( debug.getupvalue( fn, 1 ) == nil, "a replaced function holds no original" )
    end

    if os.getenv( "SMOKE_ZIP" ) == "1" then
        check( io.open( "outside.txt" ) == nil, "from a .zip, the launch folder is not the game's files" )
    end
end

-- Whether the sandbox is really installed, found without touching any file: its package.cpath is empty and its
-- io.open refuses a host path by name (Lua's own would look for a file called "hpath:"). The writing scenarios
-- run only if it is - against Lua's own io/os they would remove and overwrite real files.
local function sandbox_installed()
    local _, message = io.open( "hpath:///scarab-sandbox-probe" )
    return package.cpath == "" and type( message ) == "string" and message:find( "host path", 1, true ) ~= nil
end

-- The rule scenario's last checks (C8a): the engine's objects can't be forged. Before C8a they were light-userdata
-- globals, so a script could set one to any userdata and the engine would use it as its own object. Each old name
-- is set to a Lua file; then a function that reaches each object must still work. Also the lines iterator's file,
-- a C upvalue debug.setupvalue can replace, must be refused, not used.
local function check_forgery()
    for _, name in ipairs( { "scriptProcessorPtr", "timerMapPtr", "rendererProviderPtr", "soundManagerPtr",
                             "spritePoolPtr", "fontPoolPtr", "playerSlotsPtr", "actionMapPtr", "hapticsHolderPtr",
                             "worldTablePtr", "saveDirectoryPtr" } ) do
        _G[name] = io.stdout
    end

    check( type( app_get_save_directory() ) == "string", "the save directory survives a forged global" )
    check( write_all( "forged.txt", "still the save" ) == true, "writing survives a forged global" )
    check( save_exists( "forged.txt" ) == true, "save_exists survives a forged global" )
    check( math.type( screen_get_width() ) == "integer", "the renderer survives a forged global" )
    check( pcall( sound_is_playing, 1 ), "the sound manager survives a forged global" )
    check( pool_register_type( "forged", 1 ) == true, "the sprite pool survives a forged global" )
    check( input_bind_action_key( "forged", KEY_SPACE ) == true, "the action map survives a forged global" )
    check( world_get_count() == 1, "the world table survives a forged global" )
    check( sp_wait( 1 ) == true, "the script processor survives a forged global" )

    local iterator = io.lines( gameDir .. "lines.txt" )
    debug.setupvalue( iterator, 1, io.stdout )
    local ok, message = pcall( iterator )
    check( not ok and tostring( message ):find( "replaced", 1, true ) ~= nil, "a replaced lines iterator file is refused: " .. tostring( message ) )
end

if scenario ~= "nosandbox" and not sandbox_installed() then
    check( false, "the sandbox is not installed: nothing else was run" )

elseif scenario == "rule" then
    step( "reading" )
    check_reading()
    step( "writing" )
    check_writing()
    step( "os" )
    check_os()
    step( "code" )
    check_code()
    step( "refusals" )
    check_refusals()
    step( "forgery" )
    check_forgery()
    step( "done" )

elseif scenario == "nodir" then
    step( "app_get_save_directory" )
    expect_fail( "no save directory", "no save directory", app_get_save_directory() )
    step( "read" )
    check( read_all( gameDir .. "game.txt" ) == "game\n", "reading needs no save directory" )
    step( "io.open w" )
    expect_fail( "writing", "no save directory", io.open( gameDir .. "x.txt", "w" ) )
    step( "os.remove" )
    expect_fail( "os.remove", "no save directory", os.remove( "x.txt" ) )
    step( "os.tmpname" )
    check( not pcall( os.tmpname ), "os.tmpname raises with no save directory" )
    step( "done" )

elseif scenario == "switch" then
    check( write_all( "switch.txt", "a" ) == true, "write into the first folder" )
    check( type( app_set_save_directory( "scarab-sandbox-smoke-b" ) ) == "string", "change the save folder" )
    check( read_all( "switch.txt" ) == nil, "a plain path no longer finds the old folder's file" )
    check( read_all( APP_DIR .. "switch.txt" ) == nil, "an APP_DIR path no longer finds it either" )
    check( save_exists( "switch.txt" ) == false, "nor does save_exists" )
    check( write_all( "switch.txt", "b" ) == true and read_all( APP_DIR .. "switch.txt" ) == "b", "the new folder is used" )

elseif scenario == "nosandbox" then
    check( package.cpath ~= "", "Lua's own package.cpath" )
    check( load( string.dump( function() return 1 end ) )() == 1, "Lua's own load takes bytecode" )
    check( read_all( gameDir .. "game.txt" ) == "game\n", "Lua's own io.open" )
    check( pack_list_directory == nil, "no pack_* in a game, even without the sandbox" )
    check( type( debug.getregistry() ) == "table", "Lua's own debug.getregistry" )

else
    check( false, "unknown SMOKE_SCENARIO" )
end

local reported = false

-- Called by Scarab every frame: the checks ran at load time, so on the first frame print OK and quit if none
-- failed; otherwise print every failure, once, and keep running, so --max-frames runs out and the run fails.
function on_update( dt )
    if reported then return end
    reported = true

    step( "reporting" )
    if #failures == 0 then
        print( "SANDBOX SMOKE OK" )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "SANDBOX SMOKE FAIL: " .. message )
        end
    end
end
