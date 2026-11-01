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

    The driver passes the repo's absolute path in SCARAB_SMOKE_REPO_ROOT, for the host paths.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __DIR     <const> = "scripts/path_scheme_smoke/"
local __ANSWER  <const> = 42                                    -- data.json's "answer", module.lua's result
local __MAP     <const> = "resources/tilemap/test.tmx"          -- external tilesets (.tsx) resolved beside it
local __MAP_COLUMNS <const> = 20                                -- test.tmx is 20x20 tiles

local repoRoot = os.getenv( "SCARAB_SMOKE_REPO_ROOT" )

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

check( repoRoot ~= nil, "SCARAB_SMOKE_REPO_ROOT is not set (run through scripts/path_scheme_smoke.py)" )

local hostDir = ( repoRoot or "" ) .. "/" .. __DIR
local schemes = {
    { "plain",          __DIR },
    { "vpath",          "vpath://" .. __DIR },
    { "virtualpath",    "virtualpath://" .. __DIR },
    { "hpath",          "hpath://" .. hostDir },
    { "hostpath",       "hostpath://" .. hostDir },
}

-- Scarab's own loaders: load_json and dofile
for _, scheme in ipairs( schemes ) do
    local label, dir = scheme[1], scheme[2]
    local data, err  = load_json( dir .. "data.json" )

    check( type( data ) == "table" and data.answer == __ANSWER,
           label .. ": load_json gave " .. tostring( data ) .. " (" .. tostring( err ) .. ")" )

    local ok, result = pcall( dofile, dir .. "module.lua" )

    check( ok and result == __ANSWER, label .. ": dofile gave " .. tostring( result ) )
end

-- a sunlight loader: a map and its external tilesets
for _, path in ipairs( { "vpath://" .. __MAP, "hpath://" .. ( repoRoot or "" ) .. "/" .. __MAP } ) do
    local ok, err = tilemap_load_map( path, MAP_ALIGNMENT_TOP_LEFT )
    local columns = tilemap_get_map_info()

    check( ok == true and columns == __MAP_COLUMNS, "tilemap_load_map( " .. path .. " ): " .. tostring( err ) )
end

-- an unknown scheme is refused, not taken as a file name
expect_error( "unknown scheme", "cannot open", load_json( "foo://" .. __DIR .. "data.json" ) )
check( not pcall( dofile, "foo://" .. __DIR .. "module.lua" ), "dofile accepted an unknown scheme" )

local reported = false

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
