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
 * @brief LuaEngineUtil's strict argument readers and helpers (plans/MASTER_PLAN.md item D3), on a
 * bare lua_State - no engine, no window. Every Lua function reads its arguments through these, so
 * their accept/refuse rules and messages are what every script sees.
 */

#include <cmath>
#include <limits>
#include <string>
#include "doctest/doctest.h"
#include "lua/luaengineutil.h"

extern "C"
{
  #include "lauxlib.h"
}

using namespace Scarab :: Engine :: Lua;

/** A fresh Lua state whose stack holds exactly the values pushed by the test. */
class LuaStack  {

    public:

    lua_State  *L;

    LuaStack( void ) : L( luaL_newstate() )  {
    }

    ~LuaStack( void )  {

        lua_close( L );
    }
};

static const stNamedConstant  s_aColors[] = { { "COLOR_RED", 1 }, { "COLOR_BLUE", 4 } };


TEST_CASE( "ReadIntegerArg: a whole number in, anything else refused with the argument's name" )  {

    LuaStack     stack;
    lua_Integer  nValue = 0;
    std :: string  strError;

    lua_pushinteger( stack.L, 7 );           // 1
    lua_pushnumber( stack.L, 3.0 );          // 2: a float with a whole value is fine
    lua_pushnumber( stack.L, 3.5 );          // 3
    lua_pushstring( stack.L, "7" );          // 4: no string coercion
    lua_pushnumber( stack.L, NAN );          // 5

    CHECK( LuaEngineUtil :: ReadIntegerArg( stack.L, 1, "x", nValue, strError ) );
    CHECK( nValue == 7 );
    CHECK( LuaEngineUtil :: ReadIntegerArg( stack.L, 2, "x", nValue, strError ) );
    CHECK( nValue == 3 );

    for( int nIndex : { 3, 4, 5, 6 } )  {    // 6: missing
        strError.clear();
        CHECK_FALSE( LuaEngineUtil :: ReadIntegerArg( stack.L, nIndex, "x", nValue, strError ) );
        CHECK( strError == "argument 'x' must be an integer" );
    }
}

TEST_CASE( "ReadIntegerArgInRange and ReadIntArg check the range, inclusive" )  {

    LuaStack     stack;
    lua_Integer  nValue = 0;
    int          nInt   = 0;
    std :: string  strError;

    lua_pushinteger( stack.L, 0 );
    lua_pushinteger( stack.L, 255 );
    lua_pushinteger( stack.L, 256 );
    lua_pushinteger( stack.L, ( lua_Integer ) std :: numeric_limits<int> :: max() + 1 );

    CHECK( LuaEngineUtil :: ReadIntegerArgInRange( stack.L, 1, "r", 0, 255, nValue, strError ) );
    CHECK( LuaEngineUtil :: ReadIntegerArgInRange( stack.L, 2, "r", 0, 255, nValue, strError ) );
    CHECK( nValue == 255 );
    CHECK_FALSE( LuaEngineUtil :: ReadIntegerArgInRange( stack.L, 3, "r", 0, 255, nValue, strError ) );
    CHECK( strError == "argument 'r' must be between 0 and 255, got 256" );

    CHECK( LuaEngineUtil :: ReadIntArg( stack.L, 2, "n", nInt, strError ) );
    CHECK( nInt == 255 );
    strError.clear();
    CHECK_FALSE( LuaEngineUtil :: ReadIntArg( stack.L, 4, "n", nInt, strError ) );   // does not fit an int
    CHECK( strError.find( "argument 'n'" ) == 0 );
}

TEST_CASE( "ReadOptionalIntegerArg: nil or absent gives the default, a bad value is still refused" )  {

    LuaStack     stack;
    lua_Integer  nValue = 0;
    std :: string  strError;

    lua_pushnil( stack.L );
    lua_pushinteger( stack.L, 9 );
    lua_pushnumber( stack.L, 1.5 );

    CHECK( LuaEngineUtil :: ReadOptionalIntegerArg( stack.L, 1, "d", -1, nValue, strError ) );
    CHECK( nValue == -1 );
    CHECK( LuaEngineUtil :: ReadOptionalIntegerArg( stack.L, 2, "d", -1, nValue, strError ) );
    CHECK( nValue == 9 );
    CHECK( LuaEngineUtil :: ReadOptionalIntegerArg( stack.L, 4, "d", -1, nValue, strError ) );
    CHECK( nValue == -1 );
    CHECK_FALSE( LuaEngineUtil :: ReadOptionalIntegerArg( stack.L, 3, "d", -1, nValue, strError ) );
    CHECK( strError == "argument 'd' must be an integer" );
}

TEST_CASE( "ReadNumberArg and ReadNumberArgInRange: NaN and out-of-range numbers are refused" )  {

    LuaStack  stack;
    double    fValue = 0.0;
    std :: string  strError;

    lua_pushnumber( stack.L, 0.25 );
    lua_pushnumber( stack.L, 1.5 );
    lua_pushnumber( stack.L, NAN );
    lua_pushstring( stack.L, "0.5" );

    CHECK( LuaEngineUtil :: ReadNumberArg( stack.L, 1, "f", fValue, strError ) );
    CHECK( fValue == 0.25 );
    CHECK( LuaEngineUtil :: ReadNumberArgInRange( stack.L, 1, "opacity", 0.0, 1.0, fValue, strError ) );
    CHECK_FALSE( LuaEngineUtil :: ReadNumberArgInRange( stack.L, 2, "opacity", 0.0, 1.0, fValue, strError ) );
    CHECK( strError == "argument 'opacity' must be between 0 and 1, got 1.5" );
    CHECK_FALSE( LuaEngineUtil :: ReadNumberArgInRange( stack.L, 3, "opacity", 0.0, 1.0, fValue, strError ) );
    CHECK( strError.find( "argument 'opacity' must be between 0 and 1, got " ) == 0 );
    CHECK_FALSE( LuaEngineUtil :: ReadNumberArg( stack.L, 4, "f", fValue, strError ) );
    CHECK( strError == "argument 'f' must be a number" );
}

TEST_CASE( "ReadBooleanArg, ReadStringArg and CheckFunctionArg accept their own type only" )  {

    LuaStack  stack;
    bool      bValue = false;
    std :: string  strValue;
    std :: string  strError;

    lua_pushboolean( stack.L, 1 );                   // 1
    lua_pushinteger( stack.L, 1 );                   // 2
    lua_pushstring( stack.L, "hello" );              // 3
    lua_pushinteger( stack.L, 5 );                   // 4
    lua_pushcfunction( stack.L, []( lua_State * ) -> int { return 0; } );   // 5

    CHECK( LuaEngineUtil :: ReadBooleanArg( stack.L, 1, "visible", bValue, strError ) );
    CHECK( bValue );
    CHECK_FALSE( LuaEngineUtil :: ReadBooleanArg( stack.L, 2, "visible", bValue, strError ) );
    CHECK( strError == "argument 'visible' must be a boolean" );
    CHECK( LuaEngineUtil :: ReadOptionalBooleanArg( stack.L, 6, "visible", true, bValue, strError ) );
    CHECK( bValue );

    CHECK( LuaEngineUtil :: ReadStringArg( stack.L, 3, "name", strValue, strError ) );
    CHECK( strValue == "hello" );
    CHECK_FALSE( LuaEngineUtil :: ReadStringArg( stack.L, 4, "name", strValue, strError ) );   // no number coercion
    CHECK( strError == "argument 'name' must be a string" );

    CHECK( LuaEngineUtil :: CheckFunctionArg( stack.L, 5, "handler", strError ) );
    CHECK_FALSE( LuaEngineUtil :: CheckFunctionArg( stack.L, 3, "handler", strError ) );
    CHECK( strError == "argument 'handler' must be a function" );
}

TEST_CASE( "ReadConstantArg accepts only a value of its constant group" )  {

    LuaStack  stack;
    int       nValue = 0;
    std :: string  strError;

    lua_pushinteger( stack.L, 4 );
    lua_pushinteger( stack.L, 2 );
    lua_pushnil( stack.L );                          // eg. a misspelt COLOR_* name

    CHECK( LuaEngineUtil :: ReadConstantArg( stack.L, 1, "color", s_aColors, 2, "COLOR_*", nValue, strError ) );
    CHECK( nValue == 4 );
    CHECK_FALSE( LuaEngineUtil :: ReadConstantArg( stack.L, 2, "color", s_aColors, 2, "COLOR_*", nValue, strError ) );
    CHECK( strError == "argument 'color': unknown COLOR_* value 2" );
    CHECK_FALSE( LuaEngineUtil :: ReadConstantArg( stack.L, 3, "color", s_aColors, 2, "COLOR_*", nValue, strError ) );
    CHECK( strError == "argument 'color' must be a COLOR_* constant" );
}

TEST_CASE( "PushError and PushTrue: the nil, message / true convention" )  {

    LuaStack  stack;

    CHECK( LuaEngineUtil :: PushError( stack.L, "no map is loaded" ) == 2 );
    CHECK( lua_isnil( stack.L, -2 ) );
    CHECK( std :: string( lua_tostring( stack.L, -1 ) ) == "no map is loaded" );

    lua_settop( stack.L, 0 );
    CHECK( LuaEngineUtil :: PushTrue( stack.L ) == 1 );
    CHECK( lua_isboolean( stack.L, -1 ) );
    CHECK( lua_toboolean( stack.L, -1 ) == 1 );
}

TEST_CASE( "FormatNumber writes a number as a user would" )  {

    CHECK( LuaEngineUtil :: FormatNumber( 3.8125 ) == "3.8125" );
    CHECK( LuaEngineUtil :: FormatNumber( 16.0 ) == "16" );
    CHECK( LuaEngineUtil :: FormatNumber( 0.0625 ) == "0.0625" );
}

TEST_CASE( "ShortestDecimal: the shortest decimal that converts back to exactly the same float" )  {

    // A float holds 0.69 as 0.6899999761...; Lua must read 0.69, and writing that back gives the same float.
    for( double fAuthored : { 0.0, 0.1, 0.25, 0.3, 0.5, 0.65, 0.69, 0.75, 1.0 } )  {
        float   fStored = ( float ) fAuthored;
        double  fRead   = LuaEngineUtil :: ShortestDecimal( fStored );

        CHECK( fRead == fAuthored );
        CHECK( ( float ) fRead == fStored );
    }

    float  fOdd = 0.123456789f;                       // needs every digit a float has

    CHECK( ( float ) LuaEngineUtil :: ShortestDecimal( fOdd ) == fOdd );
}
