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
 * @brief LuaRendererApi::ValidateZoomFactor: the one zoom rule renderer_create and every view_*
 * zoom setter share - a whole multiple of ZOOM_STEP (0.0625) within [ZOOM_FACTOR_MIN,
 * ZOOM_FACTOR_MAX], never rounded, the nearest valid factor named when refused.
 */

#include <cmath>
#include <string>
#include "doctest/doctest.h"
#include "lua/luarendererapi.h"
#include "base/viewport.h"

using namespace Scarab :: Engine :: Lua;


TEST_CASE( "Every whole multiple of the zoom step is valid, and maps to its position" )  {

    unsigned       nZoomPos = 0;
    std :: string  strError;

    CHECK( LuaRendererApi :: ValidateZoomFactor( 0.0625, nZoomPos, strError ) );
    CHECK( nZoomPos == 0 );
    CHECK( LuaRendererApi :: ValidateZoomFactor( 1.0, nZoomPos, strError ) );
    CHECK( nZoomPos == 15 );
    CHECK( LuaRendererApi :: ValidateZoomFactor( 3.8125, nZoomPos, strError ) );     // Scarab's default
    CHECK( nZoomPos == 60 );
    CHECK( LuaRendererApi :: ValidateZoomFactor( 16.0, nZoomPos, strError ) );
    CHECK( nZoomPos == 255 );

    for( unsigned nPos = 0; nPos < SunLight :: Base :: ZOOM_POS_COUNT; nPos++ )  {
        unsigned  nBack = 0;

        REQUIRE( LuaRendererApi :: ValidateZoomFactor( LuaRendererApi :: FactorOfPosition( nPos ), nBack, strError ) );
        CHECK( nBack == nPos );
    }
}

TEST_CASE( "A factor between two steps is refused, naming the nearest valid one - never rounded" )  {

    unsigned       nZoomPos = 99;
    std :: string  strError;

    CHECK_FALSE( LuaRendererApi :: ValidateZoomFactor( 3.8, nZoomPos, strError ) );
    CHECK( strError == "zoom factor 3.8 is not a multiple of 0.0625 - nearest valid is 3.8125" );
    CHECK( nZoomPos == 99 );                          // unchanged on refusal
}

TEST_CASE( "A factor outside the range, or not finite, is refused" )  {

    unsigned       nZoomPos = 0;
    std :: string  strError;

    CHECK_FALSE( LuaRendererApi :: ValidateZoomFactor( 0.0, nZoomPos, strError ) );
    CHECK( strError == "zoom factor 0 is outside [0.0625, 16] - nearest valid is 0.0625" );
    CHECK_FALSE( LuaRendererApi :: ValidateZoomFactor( 16.0625, nZoomPos, strError ) );
    CHECK( strError == "zoom factor 16.0625 is outside [0.0625, 16] - nearest valid is 16" );
    CHECK_FALSE( LuaRendererApi :: ValidateZoomFactor( -1.0, nZoomPos, strError ) );
    CHECK_FALSE( LuaRendererApi :: ValidateZoomFactor( NAN, nZoomPos, strError ) );
    CHECK( strError == "zoom factor must be a finite number" );
    CHECK_FALSE( LuaRendererApi :: ValidateZoomFactor( INFINITY, nZoomPos, strError ) );
}
