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
 * @brief FontPool (src/engine/): the fonts Lua loaded with load_font, one shared sunlight font per
 * handle. A stand-in IFont replaces a real font, so no window is needed.
 */

#include <memory>
#include "doctest/doctest.h"
#include "engine/fontpool.h"

using namespace Scarab :: Engine;

// The checks compare raw pointers (.get()): doctest prints both sides of a failed check, and
// GCC/MSVC can't print a std::shared_ptr of an abstract class (Apple clang can).

/** A font that draws nothing: only its identity and lifetime matter to the pool. */
class FakeFont : public SunLight :: Font :: IFont  {

    public:

    bool IsValid( void ) const override  {

        return true;
    }

    int MeasureText( const char*, int ) override  {

        return 0;
    }

    void DrawText( const char*, int, int, int, SunLight :: Base :: stColor ) override  {
    }

    void DrawTextRotated( const char*, int, int, int, float, SunLight :: Base :: stColor ) override  {
    }
};


TEST_CASE( "Add hands out a handle that resolves to the same font" )  {

    FontPool                                      pool;
    std :: shared_ptr<SunLight :: Font :: IFont>  pFont = std :: make_shared<FakeFont>();

    FontHandle  handle = pool.Add( pFont );

    CHECK( handle != INVALID_FONT_HANDLE );
    CHECK( pool.Resolve( handle ).get() == pFont.get() );
    CHECK( pool.Add( nullptr ) == INVALID_FONT_HANDLE );           // no font, no handle

    FontHandle  other = pool.Add( std :: make_shared<FakeFont>() );

    CHECK( other != handle );
    CHECK( pool.Resolve( other ).get() != pFont.get() );
}

TEST_CASE( "Release drops only the pool's reference: a font still used elsewhere stays alive" )  {

    FontPool                                      pool;
    std :: shared_ptr<SunLight :: Font :: IFont>  pFont = std :: make_shared<FakeFont>();
    std :: weak_ptr<SunLight :: Font :: IFont>    weak  = pFont;

    FontHandle  handle = pool.Add( pFont );

    CHECK( pFont.use_count() == 2 );                               // the test and the pool
    CHECK( pool.Release( handle ) );
    CHECK( pFont.use_count() == 1 );                               // the pool let go ...
    CHECK_FALSE( weak.expired() );                                 // ... the other owner keeps it

    pFont.reset();
    CHECK( weak.expired() );                                       // the last owner released it
}

TEST_CASE( "A released font handle never resolves again, even when its slot is reused" )  {

    FontPool    pool;
    FontHandle  handle = pool.Add( std :: make_shared<FakeFont>() );

    REQUIRE( pool.Release( handle ) );
    CHECK( pool.Resolve( handle ).get() == nullptr );
    CHECK_FALSE( pool.Release( handle ) );

    std :: shared_ptr<SunLight :: Font :: IFont>  pNext = std :: make_shared<FakeFont>();
    FontHandle                                    reused = pool.Add( pNext );     // takes the freed slot

    CHECK( reused != handle );
    CHECK( pool.Resolve( reused ).get() == pNext.get() );
    CHECK( pool.Resolve( handle ).get() == nullptr );
    CHECK_FALSE( pool.Release( handle ) );
    CHECK( pool.Resolve( reused ).get() == pNext.get() );                      // the stale handle did not release it
}

TEST_CASE( "Clear releases every font the pool holds" )  {

    FontPool                                      pool;
    std :: weak_ptr<SunLight :: Font :: IFont>    weak;
    FontHandle                                    handle = INVALID_FONT_HANDLE;

    {
        std :: shared_ptr<SunLight :: Font :: IFont>  pFont = std :: make_shared<FakeFont>();

        weak   = pFont;
        handle = pool.Add( pFont );
    }

    REQUIRE_FALSE( weak.expired() );                               // only the pool holds it now
    pool.Clear();
    CHECK( weak.expired() );
    CHECK( pool.Resolve( handle ).get() == nullptr );
    CHECK( pool.Resolve( INVALID_FONT_HANDLE ).get() == nullptr );
}
