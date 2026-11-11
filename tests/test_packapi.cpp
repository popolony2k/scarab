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

/**
 * @brief The pack_* functions' failures (plans/MASTER_PLAN.md items D3 and C8), on a bare lua_State. They
 * exist only in the --pack tool since C8, so scripts/error_reporting_smoke (a game) can no longer call them:
 * its pack rows moved here, unchanged.
 */

#include <filesystem>
#include <string>
#include "doctest/doctest.h"
#include "lua/luapackapi.h"

extern "C"  {
  #include "lauxlib.h"
  #include "lualib.h"
}

using namespace Scarab :: Engine :: Lua;

/** A fresh Lua state with the base library and the pack_* functions. */
class PackLuaState  {

    public:

    lua_State*  L;

    /**
     * @brief Open a Lua state and register the pack functions.
     */
    PackLuaState( void ) : L( luaL_newstate() )  {

        luaL_openlibs( L );
        LuaPackApi :: Register( L );
    }

    /**
     * @brief Close the Lua state.
     */
    ~PackLuaState( void )  {

        lua_close( L );
    }

    /**
     * @brief Run a chunk and return its one result as a string ("" for a non-string, "error: ..." when it
     * raised).
     * @param szChunk The Lua code;
     * @return Its result;
     */
    std :: string Run( const char* szChunk )  {

        if( luaL_dostring( L, szChunk ) != LUA_OK )  {
            std :: string  strError = std :: string( "error: " ) + lua_tostring( L, -1 );

            lua_settop( L, 0 );
            return strError;
        }

        std :: string  strResult = lua_isstring( L, -1 ) ? lua_tostring( L, -1 ) : "";

        lua_settop( L, 0 );

        return strResult;
    }
};

/**
 * @brief The Lua code that calls a pack function and returns its error message, or "no error".
 * @param strCall The call, as Lua code;
 * @return The chunk;
 */
static std :: string ErrorOf( const std :: string& strCall )  {

    return "local value, message = " + strCall + " if value == nil then return message end return 'no error'";
}

TEST_CASE( "pack_* report a failure as nil, message" )  {

    PackLuaState   lua;
    std :: string  strMissing = ( std :: filesystem :: temp_directory_path() / "scarab-pack-test-does-not-exist" ).generic_string();
    std :: string  strGood    = std :: filesystem :: temp_directory_path().generic_string();

    CHECK( lua.Run( ErrorOf( "pack_list_directory( '" + strMissing + "' )" ).c_str() ).find( "is not a directory" ) != std :: string :: npos );
    CHECK( lua.Run( ( "return type( pack_list_directory( '" + strGood + "' ) )" ).c_str() ) == "table" );
    CHECK( lua.Run( ErrorOf( "pack_read_file( '" + strMissing + "' )" ).c_str() ).find( "cannot open" ) != std :: string :: npos );
    CHECK( lua.Run( ErrorOf( "pack_create_archive( '" + strMissing + "/sub/x.zip' )" ).c_str() ).find( "cannot create archive" ) != std :: string :: npos );
    CHECK( lua.Run( ErrorOf( "pack_add_entry( 9999, 'a', 'b' )" ).c_str() ).find( "invalid archive handle" ) != std :: string :: npos );
    CHECK( lua.Run( ErrorOf( "pack_close_archive( 9999 )" ).c_str() ).find( "invalid archive handle" ) != std :: string :: npos );
}
