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
    Path scheme smoke test - run by scripts/path_scheme_smoke.py (and ci.yml, every platform):

        SCARAB_SMOKE_REPO_ROOT=<repo> scarab --headless --fast --max-frames 200 scripts/path_scheme_smoke/project.json

    Pins plans/MASTER_PLAN.md item P9 (sunlight I12, BREAKAGES 61): every path a Lua script hands a
    loader may carry a scheme, resolved by sunlight's FileSystem - none, "vpath://" or
    "virtualpath://" for a virtual path (mounts, archives, the read filter), "hpath://" or
    "hostpath://" for a real OS path, and any other "<letters>://" refused. Checked through Scarab's
    own loaders (load_json, dofile) and a sunlight one (tilemap_load_map): each scheme loads the
    same file, and an unknown one is an error, not a file name.

    The driver passes the repo's absolute path in SCARAB_SMOKE_REPO_ROOT, for the host paths. Host paths
    work only with --no-sandbox (plans/MASTER_PLAN.md item C8): the driver runs the script that way, and
    once more in the sandbox with SCARAB_SMOKE_SANDBOX=1, where every loader - and io.open - must refuse
    them while the virtual schemes still load.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __DIR     <const> = "scripts/path_scheme_smoke/"
local __ANSWER  <const> = 42                                    -- data.json's "answer", module.lua's result
local __MAP     <const> = "resources/tilemap/test.tmx"          -- external tilesets (.tsx) resolved beside it
local __MAP_COLUMNS <const> = 20                                -- test.tmx is 20x20 tiles

local repoRoot  = os.getenv( "SCARAB_SMOKE_REPO_ROOT" )
local inSandbox = os.getenv( "SCARAB_SMOKE_SANDBOX" ) == "1"

local failures = {}

-- Record a failure message when `ok` is false. Every check runs; the failures are printed together at the end.
local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- The engine asks Lua to claim each stage id an sp_load_stage queues: claim any (this test loads its maps itself),
-- so an unclaimed id can never stop the run.
function on_load_stage( stageId ) return true end
sp_wait( 1 )

check( repoRoot ~= nil, "SCARAB_SMOKE_REPO_ROOT is not set (run through scripts/path_scheme_smoke.py)" )

local hostDir = ( repoRoot or "" ) .. "/" .. __DIR
local schemes = {
    { "plain",          __DIR,                        false },
    { "vpath",          "vpath://" .. __DIR,          false },
    { "virtualpath",    "virtualpath://" .. __DIR,    false },
    { "hpath",          "hpath://" .. hostDir,        true },
    { "hostpath",       "hostpath://" .. hostDir,     true },
}

-- Scarab's own loaders (load_json, dofile) and Lua's io.open: a host path loads only without the sandbox
for _, scheme in ipairs( schemes ) do
    local label, dir, isHost = scheme[1], scheme[2], scheme[3]
    local data, err  = load_json( dir .. "data.json" )
    local ok, result = pcall( dofile, dir .. "module.lua" )
    local file       = io.open( dir .. "data.json" )

    if isHost and inSandbox then
        check( data == nil, label .. ": load_json read a host path in the sandbox" )
        check( not ok, label .. ": dofile ran a host path in the sandbox" )
        check( file == nil, label .. ": io.open opened a host path in the sandbox" )
    else
        check( type( data ) == "table" and data.answer == __ANSWER,
               label .. ": load_json gave " .. tostring( data ) .. " (" .. tostring( err ) .. ")" )
        check( ok and result == __ANSWER, label .. ": dofile gave " .. tostring( result ) )
        -- io.open follows the file system only in the sandbox: --no-sandbox gives Lua's own, which opens an
        -- OS path and knows no scheme
        if inSandbox or label == "plain" then
            check( file ~= nil and file:read( "a" ):find( tostring( __ANSWER ), 1, true ) ~= nil,
                   label .. ": io.open could not read data.json" )
        end
    end

    if file then file:close() end
end

-- a sunlight loader: a map and its external tilesets
for _, path in ipairs( { "vpath://" .. __MAP, "hpath://" .. ( repoRoot or "" ) .. "/" .. __MAP } ) do
    local ok, err = tilemap_load_map( path, MAP_ALIGNMENT_TOP_LEFT )
    local columns = tilemap_get_map_info()

    if inSandbox and path:find( "hpath://", 1, true ) == 1 then
        check( ok == nil, "tilemap_load_map( " .. path .. " ) loaded a host path in the sandbox" )
    else
        check( ok == true and columns == __MAP_COLUMNS, "tilemap_load_map( " .. path .. " ): " .. tostring( err ) )
    end
end

-- an unknown scheme is refused, not taken as a file name
expect_error( "unknown scheme", "cannot open", load_json( "foo://" .. __DIR .. "data.json" ) )
check( not pcall( dofile, "foo://" .. __DIR .. "module.lua" ), "dofile accepted an unknown scheme" )

local reported = false

-- Called by Scarab every frame: the checks ran at load time, so on the first frame print OK and quit if none
-- failed; otherwise print every failure, once, and keep running, so --max-frames runs out and the run fails (exit 3).
function on_update( dt )
    if reported then return end
    reported = true

    if #failures == 0 then
        print( "PATH SCHEME SMOKE OK" )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "PATH SCHEME SMOKE FAIL: " .. message )
        end
    end
end
