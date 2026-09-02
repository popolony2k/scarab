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
 pack.lua - Scarab's own content-encryption packaging tool. See the
 root README.md's own "Content encryption" section for the full design.

 Walks a game's own loose source directory, encrypts every file's
 bytes with THIS BUILD's own compiled-in key (SCARAB_CONTENT_KEY - see
 the root CMakeLists.txt), and writes the result into a single .zip -
 the same shape scarab's own --entry/.zip loading already understands
 (EngineHost::ResolveEntryScript), just with every file's content
 opaque to anyone who doesn't have this exact build's own key.

 USAGE - two equivalent ways to run this:

 1) `scarab --pack <config.json>` (recommended - see main.cpp), from
    ANY directory, no cd required. main.cpp reads <config.json> itself
    (a real native OS path, anywhere on disk) and hands source_dir/
    output to this script as the SCARAB_PACK_SOURCE_DIR/
    SCARAB_PACK_OUTPUT environment variables (read below via Lua's own
    os.getenv - no engine changes needed on this side at all); this
    script itself is copied next to the scarab executable at build
    time (scarab_copy_binaries, root CMakeLists.txt) and located via
    the same APP_DIR mount every archived (.zip) game's own entry
    script already resolves through - see main.cpp's own --pack
    handling for the exact mechanism.

 2) Directly, run from THIS REPO'S OWN ROOT, exactly like every
    samples/ entry already is (see samples/README.md's own "..".-
    relative-path gotcha - the same mount-based virtual filesystem
    every OTHER resource load in this engine goes through applies to
    the entry script itself, tools/pack.lua included, so it can't be
    referenced from just anywhere this way):

        cd /path/to/scarab
        ./build/scarab tools/pack.lua

    pack-config.json (gitignored - see .gitignore - lives at THIS
    REPO'S OWN ROOT, read via load_json like any other config; see
    tools/pack-config.example.json for a starting point):

        { "source_dir": "my_game_source", "output": "my_game.zip" }

 Either way, source_dir/output are NOT subject to the mount
 restriction that governs pack-config.json's own location (form 2) or
 this script's own location (both forms) - every pack_*/crypto_*
 primitive this script calls (LuaPackApi/LuaCryptoApi) reads/writes
 real native OS paths directly, completely bypassing the mount-based
 virtual filesystem (that only makes sense once something's already
 been bundled/mounted - packing runs before any of that exists at
 all). So source_dir/output can be absolute paths pointing anywhere on
 disk, including a completely different project having nothing to do
 with this repo.

 Like every Scarab entry script, this still needs at least one queued
 sp_* command (see samples/hello-world/docs/README.md) even though
 packing itself finishes almost instantly - and like every sample in
 this repo, there's no programmatic "quit" primitive yet, so once
 "Done" prints, close the window (or press Esc) to actually exit.
]]

app_set_name( "Scarab - content packer" )
sp_wait( 1 )

-- Set only when launched via `scarab --pack <config.json>` (main.cpp) -
-- see this file's own USAGE comment above, form 1.
local sourceDir = os.getenv( "SCARAB_PACK_SOURCE_DIR" )
local outputZip = os.getenv( "SCARAB_PACK_OUTPUT" )

if sourceDir == nil or outputZip == nil then
    -- Direct-invocation form (USAGE form 2) - pack-config.json read from
    -- the current directory (this repo's own root, by convention).
    local config = load_json( "pack-config.json" )

    if config == nil then
        error( "tools/pack.lua: could not read pack-config.json in the current directory - "
            .. "see tools/pack-config.example.json for a starting point, or use "
            .. "'scarab --pack <config.json>' instead (see this file's own USAGE comment)" )
    end

    sourceDir = config.source_dir
    outputZip = config.output
end

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
