--[[
 Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
]]

--[[
    Sprite opacity/tint smoke test - run by scripts/sprite_tint_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/sprite_tint_smoke/project.json

    Pins plans/MASTER_PLAN.md item P8 (sunlight I10, BREAKAGES 59): sprite_set/get_opacity and
    sprite_set/get_tint - the defaults of a fresh sprite, exact round trips (0.3 reads 0.3, the
    tint's alpha kept and defaulting to 255), every rejection leaving the value as it was, that
    two sprites showing the SAME texture keep their own values, that reconfiguring a sprite's
    texture or switching its sequence keeps them, that a recycled slot starts opaque and
    untinted (SpritePool::Release), that none of it creates the renderer, and that tinted and
    faded sprites draw for a few frames without trouble. What they LOOK like is checked by eye
    with samples/sprite (keys O and T).

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __TEXTURE      <const> = "resources/sprites/sunny_idle_down.png"
local __FRAMES       <const> = 4
local __SPRITE_LAYER <const> = 1
local __DRAW_FRAMES  <const> = 5      -- frames drawn with tinted/faded sprites before quitting
local __OPACITIES    <const> = { 0, 0.1, 0.25, 0.3, 0.5, 0.69, 0.75, 1 }   -- each must read back exactly

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function tint_is( label, expected, r, g, b, a )
    check( r == expected[1] and g == expected[2] and b == expected[3] and a == expected[4],
           string.format( "%s: tint %s,%s,%s,%s - expected %d,%d,%d,%d", label, tostring( r ), tostring( g ), tostring( b ),
                          tostring( a ), expected[1], expected[2], expected[3], expected[4] ) )
end

local function opacity_is( label, expected, value )
    check( value == expected, string.format( "%s: opacity %s - expected %s", label, tostring( value ), tostring( expected ) ) )
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

local __WHITE <const> = { 255, 255, 255, 255 }

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- part 1, no renderer: handles, defaults, round trips, rejections, recycling
check( pool_register_type( "tint_probe", 1 ) == true, "pool_register_type tint_probe failed" )
local probe = sprite_acquire( "tint_probe" )
check( probe ~= nil, "sprite_acquire tint_probe failed" )

opacity_is( "fresh sprite", 1, sprite_get_opacity( probe ) )
tint_is( "fresh sprite", __WHITE, sprite_get_tint( probe ) )

for _, opacity in ipairs( __OPACITIES ) do
    check( sprite_set_opacity( probe, opacity ) == true, "sprite_set_opacity( " .. opacity .. " ) failed" )
    opacity_is( "set then get " .. opacity, opacity, sprite_get_opacity( probe ) )
end
check( sprite_set_opacity( probe, sprite_get_opacity( probe ) ) == true, "opacity write-back failed" )
opacity_is( "write-back", 1, sprite_get_opacity( probe ) )

check( sprite_set_tint( probe, 10, 20, 30, 40 ) == true, "sprite_set_tint with alpha failed" )
tint_is( "set with alpha", { 10, 20, 30, 40 }, sprite_get_tint( probe ) )
check( sprite_set_tint( probe, 255, 80, 80 ) == true, "sprite_set_tint without alpha failed" )
tint_is( "alpha left out = 255", { 255, 80, 80, 255 }, sprite_get_tint( probe ) )
check( sprite_set_tint( probe, 0, 0, 0, 0 ) == true and sprite_set_tint( probe, sprite_get_tint( probe ) ) == true,
       "tint write-back failed" )
tint_is( "write-back", { 0, 0, 0, 0 }, sprite_get_tint( probe ) )

-- every rejection: nil + the reason, and the values stay as they were
check( sprite_set_opacity( probe, 0.5 ) == true and sprite_set_tint( probe, 1, 2, 3, 4 ) == true, "setting the baseline failed" )
expect_error( "opacity above 1", "argument 'opacity' must be between 0 and 1", sprite_set_opacity( probe, 1.5 ) )
expect_error( "opacity 255", "argument 'opacity' must be between 0 and 1", sprite_set_opacity( probe, 255 ) )
expect_error( "negative opacity", "argument 'opacity' must be between 0 and 1", sprite_set_opacity( probe, -0.1 ) )
expect_error( "NaN opacity", "argument 'opacity' must be between 0 and 1", sprite_set_opacity( probe, 0 / 0 ) )
expect_error( "opacity as a string", "argument 'opacity' must be a number", sprite_set_opacity( probe, "0.5" ) )
expect_error( "missing opacity", "argument 'opacity' must be a number", sprite_set_opacity( probe ) )
opacity_is( "after the rejections", 0.5, sprite_get_opacity( probe ) )

expect_error( "r above 255", "argument 'r'", sprite_set_tint( probe, 256, 0, 0 ) )
expect_error( "negative g", "argument 'g'", sprite_set_tint( probe, 0, -1, 0 ) )
expect_error( "fractional b", "argument 'b' must be an integer", sprite_set_tint( probe, 0, 0, 3.5 ) )
expect_error( "missing b", "argument 'b'", sprite_set_tint( probe, 0, 0 ) )
expect_error( "a above 255", "argument 'a'", sprite_set_tint( probe, 0, 0, 0, 300 ) )
expect_error( "a as a string", "argument 'a'", sprite_set_tint( probe, 0, 0, 0, "x" ) )
tint_is( "after the rejections", { 1, 2, 3, 4 }, sprite_get_tint( probe ) )

for _, fn in ipairs( { { "set_opacity", sprite_set_opacity, 0.5 }, { "get_opacity", sprite_get_opacity },
                       { "set_tint", sprite_set_tint, 1, 2, 3 }, { "get_tint", sprite_get_tint } } ) do
    expect_error( fn[1] .. ", bad handle", "invalid sprite handle", fn[2]( 123456, fn[3], fn[4], fn[5] ) )
    expect_error( fn[1] .. ", handle as a string", "invalid sprite handle", fn[2]( "1", fn[3], fn[4], fn[5] ) )
end

-- a recycled slot starts opaque and untinted (SpritePool::Release)
check( sprite_release( probe ) == true, "sprite_release failed" )
expect_error( "get_opacity, released handle", "invalid sprite handle", sprite_get_opacity( probe ) )
expect_error( "get_tint, released handle", "invalid sprite handle", sprite_get_tint( probe ) )
local recycled = sprite_acquire( "tint_probe" )   -- capacity 1: the same slot, a new generation
check( recycled ~= nil and recycled ~= probe, "the recycled handle must be new: " .. tostring( recycled ) )
opacity_is( "recycled sprite", 1, sprite_get_opacity( recycled ) )
tint_is( "recycled sprite", __WHITE, sprite_get_tint( recycled ) )

-- none of the above needs a window, so none of it may create the renderer
check( renderer_get_current() == nil, "a sprite opacity/tint call created the renderer" )

-- part 2, real textures: two sprites showing the SAME texture keep their own values
check( pool_register_type( "sunny", 2 ) == true, "pool_register_type sunny failed" )
local a = sprite_acquire( "sunny" )
local b = sprite_acquire( "sunny" )
check( a ~= nil and b ~= nil, "sprite_acquire sunny failed" )

for _, h in ipairs( { a, b } ) do
    check( sprite_configure_texture( h, 0, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 ) == true,
           "sprite_configure_texture failed" )
    check( sprite_configure_texture( h, 1, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true,
           "sprite_configure_texture of sequence 1 failed" )
    check( sprite_set_active_sequence( h, 0 ) == true, "sprite_set_active_sequence failed" )
    check( sprite_add_to_layer( h, __SPRITE_LAYER ) == true, "sprite_add_to_layer failed" )
    check( sprite_set_visible( h, true ) == true, "sprite_set_visible failed" )
end
check( sprite_set_pos( a, 10, 10 ) == true and sprite_set_pos( b, 60, 10 ) == true, "sprite_set_pos failed" )

check( sprite_set_opacity( a, 0.25 ) == true and sprite_set_tint( a, 255, 80, 80 ) == true, "setting a failed" )
opacity_is( "a, after setting a", 0.25, sprite_get_opacity( a ) )
tint_is( "a, after setting a", { 255, 80, 80, 255 }, sprite_get_tint( a ) )
opacity_is( "b, after setting a (same texture)", 1, sprite_get_opacity( b ) )
tint_is( "b, after setting a (same texture)", __WHITE, sprite_get_tint( b ) )

-- reconfiguring the texture and switching the sequence keep them (they are the sprite's, not a frame's)
check( sprite_configure_texture( a, 0, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 200 ) == true,
       "reconfiguring a failed" )
check( sprite_set_active_sequence( a, 1 ) == true and sprite_set_active_sequence( a, 0 ) == true, "switching a's sequence failed" )
opacity_is( "a, after reconfigure + sequence switch", 0.25, sprite_get_opacity( a ) )
tint_is( "a, after reconfigure + sequence switch", { 255, 80, 80, 255 }, sprite_get_tint( a ) )

check( sprite_set_opacity( b, 0 ) == true and sprite_set_tint( b, 120, 160, 255 ) == true, "setting b failed" )

local frames = 0
function on_update( dt )
    frames = frames + 1

    -- the values survive real frames of drawing
    if frames == __DRAW_FRAMES then
        opacity_is( "a, after drawing", 0.25, sprite_get_opacity( a ) )
        tint_is( "a, after drawing", { 255, 80, 80, 255 }, sprite_get_tint( a ) )
        opacity_is( "b, after drawing", 0, sprite_get_opacity( b ) )
        tint_is( "b, after drawing", { 120, 160, 255, 255 }, sprite_get_tint( b ) )

        if #failures == 0 then
            print( "SPRITE TINT SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "SPRITE TINT SMOKE FAIL: " .. f ) end
        end
    end
end
