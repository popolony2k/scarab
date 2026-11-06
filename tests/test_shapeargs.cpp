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
 * @brief LuaShapeApi's argument parsers (plans/MASTER_PLAN.md item C2) on a bare lua_State - no engine,
 * no window: the flat point list (LÖVE's { x1, y1, x2, y2, ... }) and the strict options table.
 */

#include <string>
#include <vector>
#include "doctest/doctest.h"
#include "lua/luashapeapi.h"

extern "C"  {
  #include "lauxlib.h"
  #include "lualib.h"
}

using namespace Scarab :: Engine :: Lua;

/** A fresh Lua state with the value a chunk returns on its stack. */
class ShapeArg  {

    public:

    lua_State*  L;

    /**
     * @brief Open a fresh Lua state and push the value a chunk returns.
     *
     * @param szChunk Lua code ending in "return <value>";
     */
    explicit ShapeArg( const char* szChunk ) : L( luaL_newstate() )  {

        luaL_openlibs( L );
        REQUIRE( luaL_dostring( L, szChunk ) == LUA_OK );
    }

    /**
     * @brief Close the Lua state.
     */
    ~ShapeArg( void )  {

        lua_close( L );
    }

    /**
     * @brief The refusal ReadPoints gives the pushed value ("" when accepted).
     *
     * @param nMinPoints The fewest points;
     * @param points Receives the points when accepted;
     * @return The error message, or "";
     */
    std :: string PointsRefusal( int nMinPoints, std :: vector<SunLight :: Base :: stVector2D>& points )  {

        std :: string  strError;
        int            nTop = lua_gettop( L );
        bool           bOk  = LuaShapeApi :: ReadPoints( L, -1, nMinPoints, points, strError );

        CHECK( lua_gettop( L ) == nTop );          // the stack is left as it was

        return bOk ? std :: string() : strError;
    }

    /**
     * @brief The refusal ReadOptions gives the pushed value ("" when accepted).
     *
     * @param fThickness Receives the thickness;
     * @param fRotation Receives the rotation;
     * @return The error message, or "";
     */
    std :: string OptionsRefusal( double& fThickness, double& fRotation )  {

        std :: string  strError;
        int            nTop = lua_gettop( L );
        bool           bOk  = LuaShapeApi :: ReadOptions( L, -1, fThickness, fRotation, strError );

        CHECK( lua_gettop( L ) == nTop );

        return bOk ? std :: string() : strError;
    }
};

TEST_CASE( "ReadPoints takes a flat list of x, y pairs" )  {

    std :: vector<SunLight :: Base :: stVector2D>  points;
    ShapeArg                                       arg( "return { 1, 2, 3.5, -4 }" );

    REQUIRE( arg.PointsRefusal( LuaShapeApi :: MIN_POLYLINE_POINTS, points ) == "" );
    REQUIRE( points.size() == 2 );
    CHECK( points[0].x == 1.0f );
    CHECK( points[0].y == 2.0f );
    CHECK( points[1].x == 3.5f );
    CHECK( points[1].y == -4.0f );
}

TEST_CASE( "ReadPoints refuses what is not a usable point list, naming why" )  {

    std :: vector<SunLight :: Base :: stVector2D>  points;

    CHECK( ShapeArg( "return 'x'" ).PointsRefusal( 2, points ) == "argument 'points' must be a table" );
    CHECK( ShapeArg( "return { 1, 2, 3 }" ).PointsRefusal( 1, points ) ==
           "argument 'points' must hold x, y pairs: an even count of numbers, got 3" );
    CHECK( ShapeArg( "return { 1, 2 }" ).PointsRefusal( LuaShapeApi :: MIN_POLYLINE_POINTS, points ) ==
           "argument 'points' needs at least 2 points (4 numbers), got 1" );
    CHECK( ShapeArg( "return { 1, 2, 3, 4 }" ).PointsRefusal( LuaShapeApi :: MIN_POLYGON_POINTS, points ) ==
           "argument 'points' needs at least 3 points (6 numbers), got 2" );
    CHECK( ShapeArg( "return { 1, 2, 'x', 4 }" ).PointsRefusal( 2, points ) == "argument 'points' entry 3 must be a finite number" );
    CHECK( ShapeArg( "return { 1, 2, 3, 0 / 0 }" ).PointsRefusal( 2, points ) == "argument 'points' entry 4 must be a finite number" );
    CHECK( ShapeArg( "return { 1, math.huge, 3, 4 }" ).PointsRefusal( 2, points ) == "argument 'points' entry 2 must be a finite number" );
    // nested pairs (another engine's form) are refused: entries must be numbers
    CHECK( ShapeArg( "return { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } }" ).PointsRefusal( 2, points ) == "argument 'points' entry 1 must be a finite number" );
    CHECK( ShapeArg( "return { 1, 2, 3, 4, x = 5 }" ).PointsRefusal( 2, points ) ==
           "argument 'points' must be a plain list of numbers { x1, y1, x2, y2, ... }" );
    CHECK( ShapeArg( "return { 1, 2, nil, 4, 5, 6 }" ).PointsRefusal( 2, points ).find( "argument 'points'" ) == 0 );   // a hole
}

TEST_CASE( "ReadOptions: defaults, both options, and every refusal" )  {

    double  fThickness = 0;
    double  fRotation  = 0;

    CHECK( ShapeArg( "return nil" ).OptionsRefusal( fThickness, fRotation ) == "" );
    CHECK( fThickness == 1.0 );
    CHECK( fRotation == 0.0 );

    CHECK( ShapeArg( "return {}" ).OptionsRefusal( fThickness, fRotation ) == "" );
    CHECK( fThickness == 1.0 );

    CHECK( ShapeArg( "return { thickness = 2.5, rotation = -45 }" ).OptionsRefusal( fThickness, fRotation ) == "" );
    CHECK( fThickness == 2.5 );
    CHECK( fRotation == -45.0 );

    CHECK( ShapeArg( "return 3" ).OptionsRefusal( fThickness, fRotation ) == "argument 'options' must be a table" );
    CHECK( ShapeArg( "return { thickness = 0 }" ).OptionsRefusal( fThickness, fRotation ) == "option 'thickness' must be a number greater than 0" );
    CHECK( ShapeArg( "return { thickness = -1 }" ).OptionsRefusal( fThickness, fRotation ) == "option 'thickness' must be a number greater than 0" );
    CHECK( ShapeArg( "return { thickness = '2' }" ).OptionsRefusal( fThickness, fRotation ) == "option 'thickness' must be a number greater than 0" );
    CHECK( ShapeArg( "return { thickness = 1 / 0 }" ).OptionsRefusal( fThickness, fRotation ) == "option 'thickness' must be a number greater than 0" );
    CHECK( ShapeArg( "return { rotation = 0 / 0 }" ).OptionsRefusal( fThickness, fRotation ) == "option 'rotation' must be a finite number" );
    CHECK( ShapeArg( "return { rotation = true }" ).OptionsRefusal( fThickness, fRotation ) == "option 'rotation' must be a finite number" );
    CHECK( ShapeArg( "return { thikness = 2 }" ).OptionsRefusal( fThickness, fRotation ) ==
           "unknown option 'thikness' (the options are thickness, rotation)" );
    CHECK( ShapeArg( "return { 2 }" ).OptionsRefusal( fThickness, fRotation ) == "unknown option (the options are thickness, rotation)" );
}
