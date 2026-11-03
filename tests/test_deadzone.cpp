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
 * @brief LuaInputApi::ApplyDeadzone: the rule input_get_gamepad_axis applies to a stick
 * (plans/MASTER_PLAN.md item C1a) - sunlight's own: strictly inside the dead zone reads 0, the
 * edge and beyond come back unchanged, never rescaled.
 */

#include "doctest/doctest.h"
#include "lua/luainputapi.h"

using namespace Scarab :: Engine :: Lua;


TEST_CASE( "A stick value strictly inside the dead zone reads 0" )  {

    CHECK( LuaInputApi :: ApplyDeadzone( 0.0f, 0.1f ) == 0.0f );
    CHECK( LuaInputApi :: ApplyDeadzone( 0.04f, 0.1f ) == 0.0f );      // a worn stick at rest
    CHECK( LuaInputApi :: ApplyDeadzone( -0.07f, 0.1f ) == 0.0f );
    CHECK( LuaInputApi :: ApplyDeadzone( 0.0999f, 0.1f ) == 0.0f );
}

TEST_CASE( "The dead zone's edge and everything past it come back unchanged, not rescaled" )  {

    CHECK( LuaInputApi :: ApplyDeadzone( 0.1f, 0.1f ) == 0.1f );       // exactly at the edge counts
    CHECK( LuaInputApi :: ApplyDeadzone( -0.1f, 0.1f ) == -0.1f );
    CHECK( LuaInputApi :: ApplyDeadzone( 0.5f, 0.1f ) == 0.5f );
    CHECK( LuaInputApi :: ApplyDeadzone( -1.0f, 0.1f ) == -1.0f );
    CHECK( LuaInputApi :: ApplyDeadzone( 0.25f, 0.25f ) == 0.25f );
}

TEST_CASE( "A dead zone of 0 changes nothing; a dead zone of 1 keeps only the full travel" )  {

    CHECK( LuaInputApi :: ApplyDeadzone( 0.0001f, 0.0f ) == 0.0001f );
    CHECK( LuaInputApi :: ApplyDeadzone( -0.0001f, 0.0f ) == -0.0001f );
    CHECK( LuaInputApi :: ApplyDeadzone( 0.0f, 0.0f ) == 0.0f );
    CHECK( LuaInputApi :: ApplyDeadzone( 0.99f, 1.0f ) == 0.0f );
    CHECK( LuaInputApi :: ApplyDeadzone( 1.0f, 1.0f ) == 1.0f );
}
