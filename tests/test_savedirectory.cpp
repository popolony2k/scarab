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
 * @brief The save directory's pure rules (plans/MASTER_PLAN.md item C1b): which names a game may give
 * its save folder, where the OS's per-user data folder is (from a fake environment), and which names an
 * action may have - nothing here touches the real disk or the real environment.
 */

#include <filesystem>
#include <map>
#include <string>
#include "doctest/doctest.h"
#include "engine/savedirectory.h"
#include "lua/luainputapi.h"

using namespace Scarab :: Engine;


/**
 * @brief A fake environment for UserDataFolder.
 * @param vars The variables it has;
 * @return An EnvLookup reading them (nullptr for any other);
 */
static SaveDirectory :: EnvLookup FakeEnv( std :: map<std :: string, std :: string> vars )  {

    return [vars]( const char* szName ) -> const char*  {
        std :: map<std :: string, std :: string> :: const_iterator  it = vars.find( szName );

        return ( it == vars.end() ) ? nullptr : it -> second.c_str();
    };
}

TEST_CASE( "A save directory's name is one folder name: letters, digits and -_. only" )  {

    std :: string  strError;

    CHECK( SaveDirectory :: IsValidName( "Caravellius", strError ) );
    CHECK( SaveDirectory :: IsValidName( "my-game_2.0", strError ) );

    CHECK_FALSE( SaveDirectory :: IsValidName( "", strError ) );
    CHECK_FALSE( SaveDirectory :: IsValidName( ".", strError ) );
    CHECK_FALSE( SaveDirectory :: IsValidName( "..", strError ) );
    CHECK_FALSE( SaveDirectory :: IsValidName( "a/b", strError ) );          // a path into another folder
    CHECK_FALSE( SaveDirectory :: IsValidName( "a\\b", strError ) );
    CHECK_FALSE( SaveDirectory :: IsValidName( "../escape", strError ) );
    CHECK_FALSE( SaveDirectory :: IsValidName( "/abs", strError ) );
    CHECK_FALSE( SaveDirectory :: IsValidName( "C:", strError ) );
    CHECK_FALSE( SaveDirectory :: IsValidName( "my game", strError ) );
    CHECK( strError.find( "'my game'" ) != std :: string :: npos );
}

TEST_CASE( "The per-user data folder follows the OS's own rule" )  {

    std :: string  strFolder;
    std :: string  strError;

#if defined( _WIN32 )
    REQUIRE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( { { "APPDATA", "C:\\Users\\me\\AppData\\Roaming" } } ), strFolder, strError ) );
    CHECK( strFolder == ( std :: filesystem :: path( "C:\\Users\\me\\AppData\\Roaming" ) / "Game" ).string() );
    CHECK_FALSE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( {} ), strFolder, strError ) );
    CHECK( strError.find( "APPDATA" ) != std :: string :: npos );
#elif defined( __APPLE__ )
    REQUIRE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( { { "HOME", "/Users/me" } } ), strFolder, strError ) );
    CHECK( strFolder == "/Users/me/Library/Application Support/Game" );
    CHECK_FALSE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( {} ), strFolder, strError ) );
    CHECK( strError.find( "HOME" ) != std :: string :: npos );
#else
    REQUIRE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( { { "HOME", "/home/me" } } ), strFolder, strError ) );
    CHECK( strFolder == "/home/me/.local/share/Game" );
    REQUIRE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( { { "HOME", "/home/me" }, { "XDG_DATA_HOME", "/data" } } ), strFolder, strError ) );
    CHECK( strFolder == "/data/Game" );
    // the XDG spec: a relative XDG_DATA_HOME is invalid and ignored
    REQUIRE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( { { "HOME", "/home/me" }, { "XDG_DATA_HOME", "rel" } } ), strFolder, strError ) );
    CHECK( strFolder == "/home/me/.local/share/Game" );
    CHECK_FALSE( SaveDirectory :: UserDataFolder( "Game", FakeEnv( {} ), strFolder, strError ) );
#endif
}

TEST_CASE( "An action's name can't hold what would corrupt the bindings file" )  {

    std :: string  strError;

    CHECK( Scarab :: Engine :: Lua :: LuaInputApi :: IsValidActionName( "jump", strError ) );
    CHECK( Scarab :: Engine :: Lua :: LuaInputApi :: IsValidActionName( "move-left_2.alt", strError ) );

    CHECK_FALSE( Scarab :: Engine :: Lua :: LuaInputApi :: IsValidActionName( "", strError ) );
    CHECK_FALSE( Scarab :: Engine :: Lua :: LuaInputApi :: IsValidActionName( "a=b", strError ) );     // "name=..." in the file
    CHECK_FALSE( Scarab :: Engine :: Lua :: LuaInputApi :: IsValidActionName( "a\nb", strError ) );    // one binding per line
    CHECK_FALSE( Scarab :: Engine :: Lua :: LuaInputApi :: IsValidActionName( "#jump", strError ) );   // a comment there
    CHECK_FALSE( Scarab :: Engine :: Lua :: LuaInputApi :: IsValidActionName( "my jump", strError ) );
}
