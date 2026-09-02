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
 pack.lua - Scarab's own content-encryption packaging tool. See
 docs/content-encryption-plan.md for the full design.

 Walks a game's own loose source directory, encrypts every file's
 bytes with THIS BUILD's own compiled-in key (SCARAB_CONTENT_KEY - see
 the root CMakeLists.txt), and writes the result into a single .zip -
 the same shape scarab's own --entry/.zip loading already understands
 (EngineHost::ResolveEntryScript), just with every file's content
 opaque to anyone who doesn't have this exact build's own key.

 USAGE - run from THIS REPO'S OWN ROOT, exactly like every samples/
 entry already is (see samples/README.md's own "..".-relative-path
 gotcha - the same mount-based virtual filesystem every OTHER resource
 load in this engine goes through applies to the entry script itself,
 tools/pack.lua included, so it can't be referenced from just anywhere):

     cd /path/to/scarab
     ./build/scarab tools/pack.lua

 pack-config.json (gitignored - see .gitignore - lives at THIS REPO'S
 OWN ROOT, read via load_json like any other config; see
 tools/pack-config.example.json for a starting point):

     { "source_dir": "my_game_source", "output": "my_game.zip" }

 source_dir/output themselves are NOT subject to that same
 restriction, unlike pack-config.json's own location - every
 pack_*/crypto_* primitive this script calls (LuaPackApi/LuaCryptoApi)
 reads/writes real native OS paths directly, completely bypassing the
 mount-based virtual filesystem (that only makes sense once something's
 already been bundled/mounted - packing runs before any of that exists
 at all). So source_dir/output can be absolute paths pointing anywhere
 on disk, including a completely different project having nothing to
 do with this repo - only pack-config.json's own location, and this
 script itself, need to stay reachable from the repo root.

 Like every Scarab entry script, this still needs at least one queued
 sp_* command (see samples/hello-world/docs/README.md) even though
 packing itself finishes almost instantly - and like every sample in
 this repo, there's no programmatic "quit" primitive yet (see
 docs/content-encryption-plan.md's own follow-up notes), so once
 "Done" prints, close the window (or press Esc) to actually exit.
]]

app_set_name( "Scarab - content packer" )
sp_wait( 1 )

local config = load_json( "pack-config.json" )

if config == nil then
    error( "tools/pack.lua: could not read pack-config.json in the current directory - "
        .. "see tools/pack-config.example.json for a starting point" )
end

local sourceDir = config.source_dir
local outputZip = config.output

print( "Packing '" .. sourceDir .. "' -> '" .. outputZip .. "'..." )

local files = pack_list_directory( sourceDir )

if #files == 0 then
    error( "tools/pack.lua: no files found under '" .. sourceDir .. "' - check pack-config.json's source_dir" )
end

local archive = pack_create_archive( outputZip )

if archive == 0 then
    error( "tools/pack.lua: could not create archive at '" .. outputZip .. "'" )
end

local packedCount = 0

for _, relativePath in ipairs( files ) do
    local data = pack_read_file( sourceDir .. "/" .. relativePath )
    local encrypted = crypto_encrypt_data( data )

    if encrypted == nil then
        error( "tools/pack.lua: encryption failed for '" .. relativePath
            .. "' - is SCARAB_CONTENT_KEY configured for this build? (cmake -B build -S . -DSCARAB_CONTENT_KEY=<64 hex chars>)" )
    end

    if not pack_add_entry( archive, relativePath, encrypted ) then
        error( "tools/pack.lua: failed to add '" .. relativePath .. "' to the archive" )
    end

    packedCount = packedCount + 1

    print( "  packed: " .. relativePath .. " (" .. #data .. " -> " .. #encrypted .. " bytes)" )
end

if not pack_close_archive( archive ) then
    error( "tools/pack.lua: failed to finalize the archive at '" .. outputZip .. "'" )
end

print( "Done - packed " .. packedCount .. " file(s) into '" .. outputZip .. "'. Close this window (or press Esc) to exit." )

function on_update( dt )
end
