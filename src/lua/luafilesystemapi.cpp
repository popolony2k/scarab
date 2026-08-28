/*
 * luafilesystemapi.cpp
 *
 *  Created on: Aug 27, 2026
 *      Author: popolony2k
 */

#include "lua/luafilesystemapi.h"
#include "filesystem/filesystemfactory.h"

#include <string>
#include <vector>

extern "C"
{
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
             * @param pLuaState Lua state to be used by engine call.
             * @return Number of values the loaded chunk itself returned
             * (required by the Lua C API calling convention);
             */
            int LuaFileSystemApi :: DoFile( lua_State *pLuaState )  {

                const char  *szFileName = lua_tostring( pLuaState, 1 );

                if( !szFileName )
                    return luaL_error( pLuaState, "dofile: expected a file name" );

                lua_settop( pLuaState, 1 );

                int  nLoadStatus;

                {
                    std :: vector<unsigned char>  data;

                    if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( szFileName, data ) )
                        return luaL_error( pLuaState, "cannot open %s", szFileName );

                    std :: string  strChunkName = std :: string( "@" ) + szFileName;

                    nLoadStatus = luaL_loadbuffer( pLuaState, reinterpret_cast<const char *>( data.data() ), data.size(), strChunkName.c_str() );
                }

                if( nLoadStatus != LUA_OK )
                    return lua_error( pLuaState );  // error message already pushed by luaL_loadbuffer

                lua_call( pLuaState, 0, LUA_MULTRET );

                return lua_gettop( pLuaState ) - 1;  // don't count the leftover filename at index 1
            }

            /**
             * @brief Register the file-loading Lua overrides. Must run
             * AFTER luaL_openlibs() - it's replacing that library's own
             * dofile, not adding a new global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            void LuaFileSystemApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "dofile", LuaFileSystemApi :: DoFile );
            }
        }
    }
}
