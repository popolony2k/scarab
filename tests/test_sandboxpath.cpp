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
 * @brief Where the sandbox writes a path (plans/MASTER_PLAN.md item C8): LuaSandboxApi::ToSavePath, the pure
 * rule that turns what a script gave into a path inside the save directory - or refuses it.
 */

#include <string>
#include "doctest/doctest.h"
#include "lua/luasandboxapi.h"

using namespace Scarab :: Engine :: Lua;

/** APP_DIR as the engine gives it: a virtual path ending in '/'. */
#define __APP_DIR  "/Games/scarab/build/"


/**
 * @brief The save path of a path, or "refused: <reason>".
 * @param strPath The path a script gave;
 * @return Where it is written;
 */
static std :: string SavePathOf( const std :: string& strPath )  {

    std :: string  strSavePath;
    std :: string  strError;

    if( !LuaSandboxApi :: ToSavePath( strPath, __APP_DIR, strSavePath, strError ) )
        return "refused: " + strError;

    return strSavePath;
}

TEST_CASE( "ToSavePath keeps a relative path as it is" )  {

    CHECK( SavePathOf( "settings.txt" ) == "settings.txt" );
    CHECK( SavePathOf( "slots/1.json" ) == "slots/1.json" );
    CHECK( SavePathOf( "./slots/./1.json" ) == "slots/1.json" );
    CHECK( SavePathOf( "slots/old/../1.json" ) == "slots/1.json" );
}

TEST_CASE( "ToSavePath writes an APP_DIR path under the part after APP_DIR (decision 1b)" )  {

    CHECK( SavePathOf( __APP_DIR "src/ai.lua" ) == "src/ai.lua" );
    CHECK( SavePathOf( "vpath://" __APP_DIR "x.txt" ) == "x.txt" );
    // Only a leading APP_DIR is taken off.
    CHECK( SavePathOf( "mods" __APP_DIR "x.txt" ) == "mods/Games/scarab/build/x.txt" );
}

TEST_CASE( "ToSavePath drops vpath:// and a leading '/'" )  {

    CHECK( SavePathOf( "vpath://a/b.txt" ) == "a/b.txt" );
    CHECK( SavePathOf( "virtualpath://a.txt" ) == "a.txt" );
    CHECK( SavePathOf( "/a/b.txt" ) == "a/b.txt" );
    CHECK( SavePathOf( "//a.txt" ) == "a.txt" );
}

TEST_CASE( "ToSavePath refuses what is not a file inside the save directory" )  {

    CHECK( SavePathOf( "hpath:///tmp/x.txt" ).find( "refused: a host path" ) == 0 );
    CHECK( SavePathOf( "hostpath://C:/x.txt" ).find( "refused: a host path" ) == 0 );
    CHECK( SavePathOf( "foo://x.txt" ) == "refused: unknown path scheme" );
    CHECK( SavePathOf( "../x.txt" ) == "refused: climbs out of the save directory" );
    CHECK( SavePathOf( "a/../../x.txt" ) == "refused: climbs out of the save directory" );
    CHECK( SavePathOf( "" ) == "refused: not a file name" );
    CHECK( SavePathOf( "slots/" ) == "refused: not a file name" );
    CHECK( SavePathOf( __APP_DIR ) == "refused: not a file name" );
}
