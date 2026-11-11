/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "lua/luafilesystemapi.h"
#include "lua/luacryptoapi.h"
#include "lua/luasandboxapi.h"
#include "filesystem/filesystemfactory.h"

#include <string>
#include <vector>

extern "C"  {
  #include "lauxlib.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Replacement for Lua's own built-in dofile(filename) -
             * loads and runs a Lua chunk from SunLight::FileSystem instead
             * of a raw fopen(), otherwise matching the standard function's
             * own documented behavior exactly: loads the named file as a
             * Lua chunk and executes it, returning every value the chunk
             * itself returns; a missing file or a load/compile error
             * raises a genuine Lua error (not a silent nil), same as the
             * built-in.
             *
             * Reimplemented as "read the whole file, luaL_loadbuffer it"
             * rather than patching Lua's own lauxlib.c internals -
             * matches the same "load it all up front, hand it to a
             * from-memory API" shape already used for textures/sound/
             * tilemaps. The chunk name is prefixed with '@' (the same
             * convention luaL_loadfile itself uses internally) so error
             * messages/tracebacks still report the real file path, not a
             * generic "[string ...]" - eg. a syntax error still reads
             * "src/foo.lua:12: ...", identical to before
             * this override existed.
             *
             * The vector/string holding the file's own bytes and chunk
             * name are deliberately scoped to go out of scope via normal
             * C++ stack unwinding *before* either error path below
             * (luaL_error/lua_error) - those may abort back to Lua via
             * longjmp depending on how this Lua build handles errors,
             * which wouldn't run C++ destructors for anything still alive
             * on this stack frame at that point.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return Number of values the loaded chunk itself returned
             * (required by the Lua C API calling convention);
             *
             * @luaname{dofile(filename)}
             * @luadoc
             * Scarab overrides Lua's own built-in `dofile` to load and
             * run the named file through `SunLight::FileSystem` instead
             * of a raw `fopen()` — otherwise it behaves exactly like the
             * standard function: loads the file as a Lua chunk and runs
             * it, returning whatever the chunk itself returns, and
             * raises a genuine Lua error (not a silent `nil`) if the
             * file is missing or fails to compile.
             *
             * This is what lets `dofile` calls reach into a mounted
             * `.zip` archive the same way every other resource load in
             * the engine does — a loose directory or an archived build
             * both work identically, with no path-construction code
             * anywhere needing to know the difference.
             *
             * Transparently runs content encrypted by
             * `crypto_encrypt_data`/`tools/pack.lua` too — nothing to
             * opt into on the calling side; this build's own
             * `SCARAB_CONTENT_KEY` is tried automatically, falling back
             * to the file as plain Lua source if it doesn't apply (no
             * key configured, or the file just isn't encrypted).
             *
             * The file is found by the path rule (the save directory,
             * then the game's own files - see "The sandbox and how
             * paths are found"), and in the sandbox only text chunks
             * run: a precompiled one is an error.
             * @luaexample
             * -- runs identically whether the game is a loose directory or a mounted .zip
             * dofile( BASE_PATH .. "scripts/stages/1st_stage_corsair.lua" )
             */
            int LuaFileSystemApi :: DoFile( lua_State* pLuaState )  {

                const char*  szFileName = lua_tostring( pLuaState, 1 );

                if( !szFileName )
                    return luaL_error( pLuaState, "dofile: expected a file name" );

                lua_settop( pLuaState, 1 );

                if( LoadChunk( pLuaState, szFileName, LuaSandboxApi :: GetChunkMode( pLuaState ) ) != LUA_OK )
                    return lua_error( pLuaState );  // the message is already pushed

                lua_call( pLuaState, 0, LUA_MULTRET );

                return lua_gettop( pLuaState ) - 1;  // don't count the leftover filename at index 1
            }

            /**
             * @brief Load a Lua chunk from a file through SunLight::FileSystem (the path rule, plans/MASTER_PLAN.md
             * item C8), shared by dofile, loadfile, require and the entry script: the whole file read, decrypted
             * when it is content-encrypted (LuaCryptoApi::TryDecryptBytes, which leaves plain text as it is), and
             * loaded with luaL_loadbufferx. The chunk is named '@' + the path, as luaL_loadfile names it, so
             * errors and tracebacks show the file. The file's bytes are released before this returns, so a
             * caller may raise the error right after.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param strPath The file's path;
             * @param szMode Lua's chunk mode: "t" (text only - the sandbox's) or "bt";
             * @return LUA_OK with the chunk pushed, or an error status with the message pushed;
             */
            int LuaFileSystemApi :: LoadChunk( lua_State* pLuaState, const std :: string& strPath, const char* szMode )  {

                std :: vector<unsigned char>  data;

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strPath, data ) )  {
                    lua_pushfstring( pLuaState, "cannot open %s", strPath.c_str() );
                    return LUA_ERRFILE;
                }

                // Transparent content-encryption support (see the root README.md's own "Content encryption"
                // section): plain text is left as it is.
                std :: vector<unsigned char>  decrypted;

                if( LuaCryptoApi :: TryDecryptBytes( data, decrypted ) )
                    data = std :: move( decrypted );

                std :: string  strChunkName = std :: string( "@" ) + strPath;

                return luaL_loadbufferx( pLuaState, reinterpret_cast<const char *>( data.data() ), data.size(), strChunkName.c_str(), szMode );
            }

            /**
             * @brief Register the file-loading Lua overrides. Must run
             * AFTER luaL_openlibs() - it's replacing that library's own
             * dofile, not adding a new global.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaFileSystemApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "dofile", LuaFileSystemApi :: DoFile );
            }
        }
    }
}
