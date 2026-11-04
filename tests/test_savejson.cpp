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
 * @brief The save API's pure parts (plans/MASTER_PLAN.md item C1c), on a bare lua_State - no engine,
 * no window: LuaJsonApi's strict Lua -> JSON conversion (and its way back), its UTF-8 check, and
 * LuaSaveApi's save-name rules.
 */

#include <cmath>
#include <limits>
#include <string>
#include "doctest/doctest.h"
#include "lua/luajsonapi.h"
#include "lua/luasaveapi.h"

extern "C"  {
  #include "lauxlib.h"
  #include "lualib.h"
}

using namespace Scarab :: Engine :: Lua;

/** A fresh Lua state that runs a chunk returning the value to convert. */
class LuaValue  {

    public:

    lua_State*  L;

    /**
     * @brief Open a fresh Lua state and push the value a chunk returns.
     *
     * @param szChunk Lua code ending in "return <value>";
     */
    explicit LuaValue( const char* szChunk ) : L( luaL_newstate() )  {

        luaL_openlibs( L );
        REQUIRE( luaL_dostring( L, szChunk ) == LUA_OK );
    }

    /**
     * @brief Close the Lua state.
     */
    ~LuaValue( void )  {

        lua_close( L );
    }

    /**
     * @brief Convert the pushed value.
     *
     * @param out Receives the JSON;
     * @param strError Receives the reason on failure;
     * @return ToJson's result;
     */
    bool Convert( nlohmann :: json& out, std :: string& strError )  {

        int   nTop = lua_gettop( L );
        bool  bOk  = LuaJsonApi :: ToJson( L, -1, "value", out, strError );

        CHECK( lua_gettop( L ) == nTop );        // the stack is left as it was, success or not

        return bOk;
    }

    /**
     * @brief The refusal a chunk's value gets.
     *
     * @return The error message ("" if it was accepted);
     */
    std :: string Refusal( void )  {

        nlohmann :: json  out;
        std :: string     strError;

        return Convert( out, strError ) ? std :: string() : strError;
    }
};

TEST_CASE( "ToJson: arrays, objects and the empty table" )  {

    nlohmann :: json  out;
    std :: string     strError;

    LuaValue  array( "return { 10, 20, 30 }" );
    REQUIRE( array.Convert( out, strError ) );
    CHECK( out == nlohmann :: json :: parse( "[10, 20, 30]" ) );

    LuaValue  object( "return { b = 1, a = { c = true } }" );
    REQUIRE( object.Convert( out, strError ) );
    CHECK( out.dump() == "{\"a\":{\"c\":true},\"b\":1}" );       // keys sorted, whatever Lua's order

    LuaValue  empty( "return {}" );
    REQUIRE( empty.Convert( out, strError ) );
    CHECK( out.dump() == "{}" );
}

TEST_CASE( "ToJson keeps integers and floats apart, exactly" )  {

    nlohmann :: json  out;
    std :: string     strError;
    LuaValue          value( "return { i = 3, f = 3.0, tenth = 0.1, big = math.maxinteger, low = math.mininteger }" );

    REQUIRE( value.Convert( out, strError ) );
    CHECK( out["i"].is_number_integer() );
    CHECK( out["f"].is_number_float() );
    CHECK( out["f"].dump() == "3.0" );
    CHECK( out["tenth"].get<double>() == 0.1 );
    CHECK( out["big"].get<std :: int64_t>() == std :: numeric_limits<std :: int64_t> :: max() );
    CHECK( out["low"].get<std :: int64_t>() == std :: numeric_limits<std :: int64_t> :: min() );
}

TEST_CASE( "ToJson refuses what JSON can't hold, naming where" )  {

    CHECK( LuaValue( "return { f = print }" ).Refusal() == "value.f is a function" );
    CHECK( LuaValue( "return { coroutine.create( print ) }" ).Refusal() == "value[1] is a thread" );
    CHECK( LuaValue( "return { io.stdout }" ).Refusal() == "value[1] is a userdata" );
    CHECK( LuaValue( "return { 1, nil, 3 }" ).Refusal() == "value has integer keys that are not 1..n (a hole, or a key below 1)" );
    CHECK( LuaValue( "return { [0] = 1, [1] = 2 }" ).Refusal() == "value has integer keys that are not 1..n (a hole, or a key below 1)" );
    CHECK( LuaValue( "return { [2] = 1 }" ).Refusal() == "value has integer keys that are not 1..n (a hole, or a key below 1)" );
    CHECK( LuaValue( "return { list = { 1, a = 2 } }" ).Refusal() == "value.list mixes integer and string keys" );
    CHECK( LuaValue( "return { [1.5] = 1 }" ).Refusal() ==
           "value has a non-integer number key (only string keys, or the integer keys 1..n of an array)" );
    CHECK( LuaValue( "return { [true] = 1 }" ).Refusal() ==
           "value has a boolean key (only string keys, or the integer keys 1..n of an array)" );
    CHECK( LuaValue( "local t = {} t.me = t return t" ).Refusal() == "value.me contains itself" );
    CHECK( LuaValue( "return { 0 / 0 }" ).Refusal() == "value[1] is NaN or infinity" );
    CHECK( LuaValue( "return { x = -math.huge }" ).Refusal() == "value.x is NaN or infinity" );
    CHECK( LuaValue( "return { [\"a b\"] = '\\xff' }" ).Refusal() == "value[\"a b\"] is a string that is not valid UTF-8" );
    CHECK( LuaValue( "return { ['\\xc0\\xaf'] = 1 }" ).Refusal() == "value has a key that is not valid UTF-8" );
    CHECK( LuaValue( "return 'text'" ).Refusal() == "" );        // a scalar converts; save_write asks for a table itself
}

TEST_CASE( "ToJson: one table on two branches is fine, nesting has a limit" )  {

    CHECK( LuaValue( "local s = { 1 } return { a = s, b = s }" ).Refusal() == "" );

    std :: string  strDepth = std :: to_string( LuaJsonApi :: MAX_DEPTH );
    std :: string  strAtLimit  = "local t = {} local c = t for _ = 2, " + strDepth + " do c.n = {} c = c.n end return t";
    std :: string  strPastLimit = "local t = {} local c = t for _ = 1, " + strDepth + " do c.n = {} c = c.n end return t";

    CHECK( LuaValue( strAtLimit.c_str() ).Refusal() == "" );
    CHECK( LuaValue( strPastLimit.c_str() ).Refusal().find( "is nested more than " + strDepth + " tables deep" ) != std :: string :: npos );
}

TEST_CASE( "PushJsonValue gives back what ToJson wrote, and refuses a document past the limit" )  {

    LuaValue          value( "return { n = 1, f = 2.5, s = 'caf\\xc3\\xa9', b = false, list = { 'x', 'y' } }" );
    nlohmann :: json  out;
    std :: string     strError;

    REQUIRE( value.Convert( out, strError ) );
    REQUIRE( LuaJsonApi :: PushJsonValue( value.L, nlohmann :: json :: parse( out.dump( 2 ) ), strError ) );
    lua_setglobal( value.L, "back" );
    REQUIRE( luaL_dostring( value.L, "return math.type( back.n ) == 'integer' and math.type( back.f ) == 'float' and "
                                     "back.s == 'caf\\xc3\\xa9' and back.b == false and back.list[2] == 'y'" ) == LUA_OK );
    CHECK( lua_toboolean( value.L, -1 ) );

    std :: string  strDeep;

    for( int nLevel = 0; nLevel <= LuaJsonApi :: MAX_DEPTH; nLevel++ )
        strDeep = "[" + strDeep + "]";

    int  nTop = lua_gettop( value.L );

    CHECK_FALSE( LuaJsonApi :: PushJsonValue( value.L, nlohmann :: json :: parse( strDeep ), strError ) );
    CHECK( strError == "nested more than " + std :: to_string( LuaJsonApi :: MAX_DEPTH ) + " tables deep" );
    CHECK( lua_gettop( value.L ) == nTop );                       // nothing left pushed
}

TEST_CASE( "IsValidUtf8 accepts UTF-8 and nothing else" )  {

    CHECK( LuaJsonApi :: IsValidUtf8( "" ) );
    CHECK( LuaJsonApi :: IsValidUtf8( "plain ASCII" ) );
    CHECK( LuaJsonApi :: IsValidUtf8( "caf\xc3\xa9" ) );                   // 2 bytes
    CHECK( LuaJsonApi :: IsValidUtf8( "\xe2\x82\xac" ) );                  // 3 bytes (euro sign)
    CHECK( LuaJsonApi :: IsValidUtf8( "\xf0\x9f\x98\x80" ) );              // 4 bytes (an emoji)
    CHECK( LuaJsonApi :: IsValidUtf8( "\xf4\x8f\xbf\xbf" ) );              // U+10FFFF, the last one
    CHECK( LuaJsonApi :: IsValidUtf8( std :: string( "a\0b", 3 ) ) );       // NUL is a character

    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xff" ) );                    // never a UTF-8 byte
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\x80" ) );                    // continuation with no lead
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xc3" ) );                    // cut short
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xe2\x82" ) );                // cut short
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xc3\x28" ) );                // not a continuation
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xc0\xaf" ) );                // overlong '/'
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xe0\x80\xaf" ) );            // overlong '/'
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xed\xa0\x80" ) );            // a surrogate (U+D800)
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xf4\x90\x80\x80" ) );        // above U+10FFFF
    CHECK_FALSE( LuaJsonApi :: IsValidUtf8( "\xf8\x88\x80\x80\x80" ) );    // 5-byte form
}

TEST_CASE( "IsValidSaveName keeps a save inside the save directory" )  {

    std :: string  strError;

    CHECK( LuaSaveApi :: IsValidSaveName( "options.json", strError ) );
    CHECK( LuaSaveApi :: IsValidSaveName( "slots/1.json", strError ) );
    CHECK( LuaSaveApi :: IsValidSaveName( "a-b_c.d/e", strError ) );
    CHECK( LuaSaveApi :: IsValidSaveName( "..hidden", strError ) );        // a name, not a '..' part
    CHECK( LuaSaveApi :: IsValidSaveName( "x.tmp.json", strError ) );      // only the END matters

    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "", strError ) );
    CHECK( strError == "argument 'name' can't be empty" );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "../x", strError ) );
    CHECK( strError == "argument 'name' can't have a '..' part" );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "a/../b", strError ) );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "./x", strError ) );
    CHECK( strError == "argument 'name' can't have a '.' part" );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "/x", strError ) );
    CHECK( strError.find( "must be a relative path" ) != std :: string :: npos );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "x/", strError ) );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "a//b", strError ) );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "hpath:///etc/x", strError ) );
    CHECK( strError == "argument 'name' may only hold letters, digits, '-', '_', '.' and '/'" );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "a\\b", strError ) );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "C:x", strError ) );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "sp ace", strError ) );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "caf\xc3\xa9", strError ) );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "x.json.tmp", strError ) );
    CHECK( strError == "argument 'name' can't end in '.tmp' (the temporary file of a save being written)" );
    CHECK_FALSE( LuaSaveApi :: IsValidSaveName( "x.TmP", strError ) );
}
