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
 * @brief LuaCryptoApi: content encryption keyed by the compile-time SCARAB_CONTENT_KEY. With a key,
 * crypto_encrypt_data's output must decrypt back through TryDecryptBytes (the read filter every
 * sunlight read goes through) and anything else must be refused; without one, nothing encrypts and
 * nothing decrypts.
 */

#include <cstring>
#include <string>
#include <vector>
#include "doctest/doctest.h"
#include "lua/luacryptoapi.h"

extern "C"  {
  #include "lauxlib.h"
}

using namespace Scarab :: Engine :: Lua;

/** A configured key is 32 bytes as 64 hex characters (see LuaCryptoApi's GetContentKey). */
#define __KEY_HEX_LENGTH  64

static const char * const  PLAIN_TEXT = "<map version=\"1.10\"> the plain bytes of a packed file </map>";

/**
 * @brief Whether this build was configured with a content key.
 * @return true when SCARAB_CONTENT_KEY holds a full key;
 */
static bool HasContentKey( void )  {

    return std :: strlen( SCARAB_CONTENT_KEY ) == __KEY_HEX_LENGTH;
}

/** crypto_encrypt_data( PLAIN_TEXT ) through Lua, as tools/pack.lua calls it; false if it failed. */
static bool Encrypt( std :: vector<unsigned char>& blob, std :: string& strError )  {

    lua_State*  L = luaL_newstate();

    LuaCryptoApi :: Register( L );
    lua_getglobal( L, "crypto_encrypt_data" );
    lua_pushstring( L, PLAIN_TEXT );
    lua_call( L, 1, 2 );

    bool  bOk = lua_isstring( L, -2 ) != 0;

    if( bOk )  {
        size_t       nLength = 0;
        const char*  pData   = lua_tolstring( L, -2, &nLength );

        blob.assign( pData, pData + nLength );
    }
    else
        strError = lua_tostring( L, -1 );

    lua_close( L );

    return bOk;
}


TEST_CASE( "Plain, unencrypted bytes never pass the read filter" )  {

    std :: vector<unsigned char>  plain( PLAIN_TEXT, PLAIN_TEXT + std :: strlen( PLAIN_TEXT ) );
    std :: vector<unsigned char>  out;

    CHECK_FALSE( LuaCryptoApi :: TryDecryptBytes( plain, out ) );
    CHECK_FALSE( LuaCryptoApi :: TryDecryptBytes( std :: vector<unsigned char>(), out ) );
}

TEST_CASE( "Encrypted bytes decrypt back exactly; tampered or cut ones are refused" )  {

    std :: vector<unsigned char>  blob;
    std :: vector<unsigned char>  out;
    std :: string                 strError;

    if( !HasContentKey() )  {
        CHECK_FALSE( Encrypt( blob, strError ) );
        CHECK( strError == "this build has no content key (SCARAB_CONTENT_KEY is not configured)" );
        return;
    }

    REQUIRE( Encrypt( blob, strError ) );
    REQUIRE( blob.size() > std :: strlen( PLAIN_TEXT ) );         // a nonce and a MAC in front

    CHECK( LuaCryptoApi :: TryDecryptBytes( blob, out ) );
    CHECK( std :: string( out.begin(), out.end() ) == PLAIN_TEXT );

    std :: vector<unsigned char>  tampered = blob;

    tampered.back() ^= 0x01;
    CHECK_FALSE( LuaCryptoApi :: TryDecryptBytes( tampered, out ) );

    std :: vector<unsigned char>  cut( blob.begin(), blob.begin() + blob.size() / 2 );

    CHECK_FALSE( LuaCryptoApi :: TryDecryptBytes( cut, out ) );

    std :: vector<unsigned char>  second;

    REQUIRE( Encrypt( second, strError ) );
    CHECK( second != blob );                                       // a fresh nonce every time
}
