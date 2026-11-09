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
    Save API smoke test - run by scripts/save_api_smoke.py (and ci.yml, every platform), which picks the
    scenario through SMOKE_SCENARIO:

        api       --save-dir <T>, run from this folder (so shipped.json sits at the virtual root, as a
                  loose game's files do): round trip of every value type, subfolders, every refusal,
                  a FOLDER named like a save, delete, a shipped file of the same name never taken for
                  a save, and that no save function creates a renderer
        fixed     writes FIXED_TABLE as "fixed.json" - the driver compares the bytes with the exact
                  expected text, across two launches (Lua's hash order differs per launch)
        nodir     a project with no save directory: every function refused
        spoiled   saves the driver wrote by hand: one that is not JSON, one that is a bare number
        first / second   the game shipped as a .zip (and an encrypted pack): the first run finds no save
                  and saves, the second reads it back; the shipped file is still not a save

    Pins plans/MASTER_PLAN.md item C1c. The script quits ITSELF only if every check passed (exit 0);
    otherwise it prints every failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __SHIPPED      <const> = "shipped.json"        -- shipped with the game, next to this script
local __FOLDER_NAME  <const> = "folder.json"         -- the driver creates a FOLDER of this name
local __BIG_INTEGER  <const> = math.maxinteger
local __RATIO        <const> = 0.1                   -- not exact in binary: must still round-trip
local __WHOLE_FLOAT  <const> = 3.0
local __LEVEL        <const> = 2                     -- what the archive's first run saves

local scenario = os.getenv( "SMOKE_SCENARIO" ) or "(unset)"
local failures = {}

-- Record a failure message, prefixed with the scenario's name, when `ok` is false. Every check runs; the
-- failures are printed together at the end.
local function check( ok, message )
    if not ok then failures[#failures + 1] = scenario .. ": " .. message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- the same table the driver expects as fixed.json, byte for byte
local function fixed_table()
    return { name = "AAA", score = 3000, ratio = 0.5, sound = true, list = { 1, 2.5, "x" }, empty = {},
             nested = { b = { c = false } } }
end

-- The engine asks Lua to claim each stage id an sp_load_stage queues: claim any (this test loads its maps itself),
-- so an unclaimed id can never stop the run.
function on_load_stage( stageId ) return true end
sp_wait( 1 )

if scenario == "api" then
    -- round trip of every value type
    local cycle = {}
    local value = { integer = 42, negative = -7, big = __BIG_INTEGER, ratio = __RATIO, whole = __WHOLE_FLOAT,
                    text = "caf\xc3\xa9 \"quoted\"\n", yes = true, no = false, list = { 10, 20, 30 },
                    empty = {}, nested = { deeper = { deepest = "ok" } }, shared1 = cycle, shared2 = cycle }

    check( save_write( "all.json", value ) == true, "save_write of every value type" )

    local back = save_read( "all.json" )

    check( type( back ) == "table", "save_read must give the table back" )
    back = back or {}
    check( back.integer == 42 and math.type( back.integer ) == "integer", "an integer stays an integer" )
    check( back.negative == -7, "a negative integer" )
    check( back.big == __BIG_INTEGER and math.type( back.big ) == "integer", "math.maxinteger survives exactly" )
    check( back.ratio == __RATIO and math.type( back.ratio ) == "float", "0.1 comes back as exactly 0.1" )
    check( back.whole == __WHOLE_FLOAT and math.type( back.whole ) == "float", "3.0 stays a float" )
    check( back.text == value.text, "a string with UTF-8, quotes and a newline" )
    check( back.yes == true and back.no == false, "booleans" )
    check( type( back.list ) == "table" and #back.list == 3 and back.list[3] == 30, "an array" )
    check( type( back.empty ) == "table" and next( back.empty ) == nil, "an empty table" )
    check( back.nested and back.nested.deeper and back.nested.deeper.deepest == "ok", "nested tables" )
    check( type( back.shared1 ) == "table" and type( back.shared2 ) == "table", "one table on two branches is written twice" )

    -- overwrite, subfolders, exists, delete
    check( save_write( "all.json", { replaced = true } ) == true and ( save_read( "all.json" ) or {} ).replaced == true, "a save is replaced" )
    check( save_write( "slots/1.json", { slot = 1 } ) == true, "a subfolder is created" )
    check( ( save_read( "slots/1.json" ) or {} ).slot == 1, "a save in a subfolder reads back" )
    check( save_exists( "slots/1.json" ) == true, "save_exists finds a save" )
    check( save_exists( "missing.json" ) == false, "save_exists: false for a missing save" )
    check( save_delete( "slots/1.json" ) == true, "save_delete" )
    check( save_exists( "slots/1.json" ) == false, "a deleted save no longer exists" )
    expect_error( "read after delete", "no save named 'slots/1.json'", save_read( "slots/1.json" ) )
    expect_error( "delete a missing save", "no save named 'missing.json'", save_delete( "missing.json" ) )
    expect_error( "read a missing save", "no save named 'missing.json'", save_read( "missing.json" ) )

    -- a FOLDER with a save's name is not a save
    check( save_exists( __FOLDER_NAME ) == false, "a folder is not a save (save_exists)" )
    expect_error( "read a folder", "no save named", save_read( __FOLDER_NAME ) )
    expect_error( "delete a folder", "no save named", save_delete( __FOLDER_NAME ) )

    -- a shipped file of the same name is never a save
    check( type( load_json( __SHIPPED ) ) == "table", "the shipped file is readable as a game file" )
    check( save_exists( __SHIPPED ) == false, "a shipped file is not a save (save_exists)" )
    expect_error( "read a shipped file", "no save named", save_read( __SHIPPED ) )
    expect_error( "delete a shipped file", "no save named", save_delete( __SHIPPED ) )
    check( save_write( __SHIPPED, { mine = true } ) == true and ( save_read( __SHIPPED ) or {} ).mine == true, "a save may share a shipped file's name" )
    check( save_delete( __SHIPPED ) == true, "deleting that save" )
    check( type( load_json( __SHIPPED ) ) == "table", "the shipped file survives deleting a save of its name" )

    -- refused values, with where
    local selfRef = {}
    selfRef.me = selfRef
    local deep = {}
    local cursor = deep

    for _ = 1, 64 do cursor.next = {}; cursor = cursor.next end    -- 65 tables deep

    expect_error( "a function", "value.f is a function", save_write( "x.json", { f = print } ) )
    expect_error( "a coroutine", "value[1] is a thread", save_write( "x.json", { coroutine.create( print ) } ) )
    expect_error( "a hole", "value has integer keys that are not 1..n", save_write( "x.json", { 1, nil, 3 } ) )
    expect_error( "a key below 1", "value has integer keys that are not 1..n", save_write( "x.json", { [0] = 1 } ) )
    expect_error( "mixed keys", "value.inner mixes integer and string keys", save_write( "x.json", { inner = { 1, a = 2 } } ) )
    expect_error( "a float key", "has a non-integer number key", save_write( "x.json", { [1.5] = 1 } ) )
    expect_error( "a boolean key", "has a boolean key", save_write( "x.json", { [true] = 1 } ) )
    expect_error( "a table key", "has a table key", save_write( "x.json", { [{}] = 1 } ) )
    expect_error( "a cycle", "value.me contains itself", save_write( "x.json", selfRef ) )
    expect_error( "NaN", "value.n is NaN or infinity", save_write( "x.json", { n = 0 / 0 } ) )
    expect_error( "infinity", "value[1] is NaN or infinity", save_write( "x.json", { math.huge } ) )
    expect_error( "bad UTF-8 text", "value[\"a b\"] is a string that is not valid UTF-8", save_write( "x.json", { ["a b"] = "\xff" } ) )
    expect_error( "bad UTF-8 key", "has a key that is not valid UTF-8", save_write( "x.json", { ["\xc0\xaf"] = 1 } ) )
    expect_error( "too deep", "is nested more than 64 tables deep", save_write( "x.json", deep ) )
    check( save_exists( "x.json" ) == false, "a refused value writes nothing" )

    -- bad names and arguments
    expect_error( "name type", "argument 'name' must be a string", save_write( 7, {} ) )
    expect_error( "value type", "argument 'value' must be a table", save_write( "x.json", "text" ) )
    expect_error( "empty name", "argument 'name' can't be empty", save_read( "" ) )
    expect_error( "a .. part", "can't have a '..' part", save_write( "../escape.json", {} ) )
    expect_error( "a . part", "can't have a '.' part", save_exists( "./x.json" ) )
    expect_error( "a leading /", "must be a relative path", save_delete( "/x.json" ) )
    expect_error( "a trailing /", "must be a relative path", save_write( "slots/", {} ) )
    expect_error( "a doubled /", "must be a relative path", save_write( "a//b.json", {} ) )
    expect_error( "a scheme", "may only hold letters", save_write( "hpath:///tmp/x.json", {} ) )
    expect_error( "a backslash", "may only hold letters", save_write( "a\\b.json", {} ) )
    expect_error( "a .tmp name", "can't end in '.tmp'", save_write( "x.json.tmp", {} ) )
    expect_error( "a .TMP name", "can't end in '.tmp'", save_read( "x.TMP" ) )

    check( renderer_get_current() == nil, "the save functions created a renderer" )

elseif scenario == "fixed" then
    check( save_write( "fixed.json", fixed_table() ) == true, "write fixed.json" )

elseif scenario == "nodir" then
    check( app_get_save_directory() == nil, "this project has no save directory" )
    expect_error( "write", "no save directory", save_write( "x.json", {} ) )
    expect_error( "read", "no save directory", save_read( "x.json" ) )
    expect_error( "exists", "no save directory", save_exists( "x.json" ) )
    expect_error( "delete", "no save directory", save_delete( "x.json" ) )
    expect_error( "bad name first", "argument 'name'", save_write( "../x.json", {} ) )   -- arguments are checked first

elseif scenario == "spoiled" then
    expect_error( "a save that is not JSON", "cannot parse save 'broken.json'", save_read( "broken.json" ) )
    expect_error( "a save that is a scalar", "not a JSON object or array", save_read( "scalar.json" ) )

elseif scenario == "first" then
    check( save_exists( "progress.json" ) == false, "a fresh save directory has no save" )
    check( type( load_json( APP_DIR .. "/" .. __SHIPPED ) ) == "table", "the shipped file in the archive is readable" )
    expect_error( "the shipped file is not a save", "no save named", save_read( __SHIPPED ) )
    check( save_write( "progress.json", { level = __LEVEL } ) == true, "save from inside an archive" )

elseif scenario == "second" then
    local progress = save_read( "progress.json" )

    check( type( progress ) == "table" and progress.level == __LEVEL, "the second run must read the save back" )

else
    check( false, "unknown SMOKE_SCENARIO" )
end

local reported = false

-- Called by Scarab every frame: the checks ran at load time, so on the first frame print OK and quit if none
-- failed; otherwise print every failure, once, and keep running, so --max-frames runs out and the run fails (exit 3).
function on_update( dt )
    if reported then return end
    reported = true

    if #failures == 0 then
        print( "SAVE API SMOKE OK" )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "SAVE API SMOKE FAIL: " .. message )
        end
    end
end
