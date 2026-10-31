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
 * @brief SpritePool and SpriteHandle (src/engine/): the Lua-agnostic sprite pool every sprite a
 * game creates lives in. No window: a slot's sprite is plain C++ until a texture is loaded.
 */

#include "doctest/doctest.h"
#include "engine/spritepool.h"
#include "engine/spritehandle.h"

using namespace Scarab :: Engine;

/** An opacity and a tint no fresh sprite has, to see a recycled slot reset them. */
#define __CHANGED_OPACITY  0.5f
#define __CHANGED_TINT     SunLight :: Base :: stColor { 1, 2, 3, 4 }
#define __OPAQUE           1.0f
#define __NO_TINT_CHANNEL  255


TEST_CASE( "SpriteHandle packs a 1-based index and a generation" )  {

    SpriteHandle  hSprite = MakeSpriteHandle( 5, 7 );

    CHECK( SpriteHandleIndex( hSprite ) == 5 );
    CHECK( SpriteHandleGeneration( hSprite ) == 7 );
    CHECK( hSprite != INVALID_SPRITE_HANDLE );
    CHECK( MakeSpriteHandle( 0, 0 ) != INVALID_SPRITE_HANDLE );    // index 0 is stored as 1
    CHECK( SpriteHandleIndex( INVALID_SPRITE_HANDLE ) == -1 );

    void*  pPacked = PackSpriteHandleAsPtr( hSprite );

    CHECK( IsPackedSpriteHandle( pPacked ) );
    CHECK( UnpackSpriteHandleFromPtr( pPacked ) == hSprite );
    CHECK_FALSE( IsPackedSpriteHandle( &hSprite ) );                 // a real pointer is not a handle
}

TEST_CASE( "RegisterType reserves fixed slots per type, once" )  {

    SpritePool  pool;

    CHECK_FALSE( pool.HasType( "ship" ) );
    CHECK( pool.RegisterType( "ship", 3 ) );
    CHECK( pool.HasType( "ship" ) );
    CHECK( pool.GetSlotCount() == 3 );

    CHECK_FALSE( pool.RegisterType( "ship", 1 ) );                  // a second registration is refused
    CHECK( pool.GetSlotCount() == 3 );

    CHECK( pool.RegisterType( "bullet", 2 ) );
    CHECK( pool.GetSlotCount() == 5 );

    CHECK_FALSE( pool.RegisterType( "too_many", MAX_SPRITE_SLOTS - 4 ) );   // past MAX_SPRITE_SLOTS in all
    CHECK( pool.RegisterType( "exactly_full", MAX_SPRITE_SLOTS - 5 ) );
    CHECK( pool.GetSlotCount() == MAX_SPRITE_SLOTS );
    CHECK_FALSE( pool.RegisterType( "one_more", 1 ) );
}

TEST_CASE( "Acquire hands out each slot of its type once, until released" )  {

    SpritePool  pool;

    REQUIRE( pool.RegisterType( "ship", 2 ) );
    REQUIRE( pool.RegisterType( "bullet", 1 ) );

    CHECK( pool.Acquire( "unknown" ) == INVALID_SPRITE_HANDLE );

    SpriteHandle  hFirst  = pool.Acquire( "ship" );
    SpriteHandle  hSecond = pool.Acquire( "ship" );

    CHECK( hFirst != INVALID_SPRITE_HANDLE );
    CHECK( hSecond != INVALID_SPRITE_HANDLE );
    CHECK( hFirst != hSecond );
    CHECK( pool.Resolve( hFirst ) != pool.Resolve( hSecond ) );
    CHECK( pool.Acquire( "ship" ) == INVALID_SPRITE_HANDLE );       // the type is exhausted ...
    CHECK( pool.Acquire( "bullet" ) != INVALID_SPRITE_HANDLE );     // ... the other type is not
}

TEST_CASE( "A released handle never resolves again, even when its slot is reused" )  {

    SpritePool  pool;

    REQUIRE( pool.RegisterType( "ship", 1 ) );

    SpriteHandle  hSprite = pool.Acquire( "ship" );

    REQUIRE( pool.Resolve( hSprite ) != nullptr );
    CHECK( pool.Release( hSprite ) );
    CHECK( pool.Resolve( hSprite ) == nullptr );
    CHECK_FALSE( pool.Release( hSprite ) );                          // a second release is refused

    SpriteHandle  hReused = pool.Acquire( "ship" );                  // capacity 1: the same slot

    REQUIRE( hReused != INVALID_SPRITE_HANDLE );
    CHECK( SpriteHandleIndex( hReused ) == SpriteHandleIndex( hSprite ) );
    CHECK( SpriteHandleGeneration( hReused ) == SpriteHandleGeneration( hSprite ) + 1 );
    CHECK( pool.Resolve( hReused ) != nullptr );
    CHECK( pool.Resolve( hSprite ) == nullptr );                     // the stale copy still fails
    CHECK_FALSE( pool.Release( hSprite ) );
    CHECK( pool.Resolve( hReused ) != nullptr );                     // ... and did not release the new owner
}

TEST_CASE( "Handles that were never handed out are refused" )  {

    SpritePool  pool;

    REQUIRE( pool.RegisterType( "ship", 1 ) );

    SpriteHandle  hSprite = pool.Acquire( "ship" );

    CHECK( pool.Resolve( INVALID_SPRITE_HANDLE ) == nullptr );
    CHECK_FALSE( pool.Release( INVALID_SPRITE_HANDLE ) );
    CHECK( pool.Resolve( MakeSpriteHandle( 100, 1 ) ) == nullptr ); // an index past every slot
    CHECK_FALSE( pool.Release( MakeSpriteHandle( 100, 1 ) ) );
    CHECK( pool.Resolve( MakeSpriteHandle( SpriteHandleIndex( hSprite ), SpriteHandleGeneration( hSprite ) + 1 ) ) == nullptr );
}

TEST_CASE( "A recycled slot starts as a fresh sprite: hidden, view space, no blend mode, opaque, untinted" )  {

    SpritePool  pool;

    REQUIRE( pool.RegisterType( "ship", 1 ) );

    SpriteHandle                   hSprite  = pool.Acquire( "ship" );
    SunLight :: Sprite :: Sprite*  pSprite = pool.Resolve( hSprite );

    REQUIRE( pSprite != nullptr );
    pSprite -> SetVisible( true );
    pSprite -> SetWorldSpace( true );
    pSprite -> SetBlendMode( SunLight :: Base :: BLEND_MODE_ADD );
    pSprite -> SetOpacity( __CHANGED_OPACITY );
    pSprite -> SetTint( __CHANGED_TINT );
    REQUIRE( pool.Release( hSprite ) );

    pSprite = pool.Resolve( pool.Acquire( "ship" ) );

    REQUIRE( pSprite != nullptr );
    CHECK_FALSE( pSprite -> GetVisible() );
    CHECK_FALSE( pSprite -> IsWorldSpace() );
    CHECK_FALSE( pSprite -> HasBlendMode() );
    CHECK( pSprite -> GetOpacity() == __OPAQUE );

    SunLight :: Base :: stColor  tint = pSprite -> GetTint();

    CHECK( tint.nRed == __NO_TINT_CHANNEL );
    CHECK( tint.nGreen == __NO_TINT_CHANNEL );
    CHECK( tint.nBlue == __NO_TINT_CHANNEL );
    CHECK( tint.nAlpha == __NO_TINT_CHANNEL );
}

TEST_CASE( "Clear forgets every type, so nothing can be acquired until types are registered again" )  {

    SpritePool  pool;

    REQUIRE( pool.RegisterType( "ship", 2 ) );
    REQUIRE( pool.Acquire( "ship" ) != INVALID_SPRITE_HANDLE );

    pool.Clear();

    CHECK_FALSE( pool.HasType( "ship" ) );
    CHECK( pool.Acquire( "ship" ) == INVALID_SPRITE_HANDLE );
}
