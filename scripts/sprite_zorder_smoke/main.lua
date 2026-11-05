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
    Sprite z-order smoke test - run by scripts/sprite_zorder_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/sprite_zorder_smoke/project.json

    Pins plans/MASTER_PLAN.md item C3 (sunlight L3, BREAKAGES 37; I15, BREAKAGES 64): sprite_set/get_z_order -
    a fresh sprite's 0, round trips (negative, zero, positive, the C int limits), every rejection leaving the
    value as it was, two sprites keeping their own values, the value kept across a layer move, a texture
    reconfigure and real frames of drawing, a recycled slot starting at 0 (SpritePool::Release), and that none
    of the handle calls creates the renderer. The DRAW ORDER itself can't be seen headless (the null renderer
    draws nothing a script can read): sunlight's own tests pin it, and samples/zorder shows it in a window.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure and never
    quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __TEXTURE      <const> = "resources/sprites/sunny_idle_down.png"
local __FRAMES       <const> = 4
local __FIRST_LAYER  <const> = 1
local __SECOND_LAYER <const> = 2
local __DRAW_FRAMES  <const> = 5               -- frames drawn before the last checks
local __INT_MAX      <const> = 2147483647      -- the range of a C int, what sunlight stores
local __INT_MIN      <const> = -2147483648
local __Z_VALUES     <const> = { -5, 0, 1, 42, __INT_MAX, __INT_MIN }   -- each must read back exactly

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function z_is( label, expected, value )
    check( value == expected and math.type( value ) == "integer",
           string.format( "%s: z-order %s - expected %s", label, tostring( value ), tostring( expected ) ) )
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- part 1, no renderer: defaults, round trips, rejections, recycling
check( pool_register_type( "z_probe", 1 ) == true, "pool_register_type z_probe failed" )
local probe = sprite_acquire( "z_probe" )
check( probe ~= nil, "sprite_acquire z_probe failed" )

z_is( "fresh sprite", 0, sprite_get_z_order( probe ) )

for _, z in ipairs( __Z_VALUES ) do
    check( sprite_set_z_order( probe, z ) == true, "sprite_set_z_order( " .. z .. " ) failed" )
    z_is( "set then get " .. z, z, sprite_get_z_order( probe ) )
end

check( sprite_set_z_order( probe, 3.0 ) == true, "a whole float (3.0) must be accepted" )
z_is( "3.0", 3, sprite_get_z_order( probe ) )

-- every rejection: nil + the reason, and the value stays as it was
expect_error( "fractional z", "argument 'z' must be an integer", sprite_set_z_order( probe, 1.5 ) )
expect_error( "z past INT_MAX", "argument 'z'", sprite_set_z_order( probe, __INT_MAX + 1 ) )
expect_error( "z below INT_MIN", "argument 'z'", sprite_set_z_order( probe, __INT_MIN - 1 ) )
expect_error( "z as a string", "argument 'z'", sprite_set_z_order( probe, "1" ) )
expect_error( "missing z", "argument 'z'", sprite_set_z_order( probe ) )
expect_error( "NaN z", "argument 'z'", sprite_set_z_order( probe, 0 / 0 ) )
z_is( "after the rejections", 3, sprite_get_z_order( probe ) )

expect_error( "set, bad handle", "invalid sprite handle", sprite_set_z_order( 123456, 1 ) )
expect_error( "get, bad handle", "invalid sprite handle", sprite_get_z_order( 123456 ) )
expect_error( "get, handle as a string", "invalid sprite handle", sprite_get_z_order( "1" ) )

-- a recycled slot starts at 0 (SpritePool::Release)
check( sprite_release( probe ) == true, "sprite_release failed" )
expect_error( "get, released handle", "invalid sprite handle", sprite_get_z_order( probe ) )
expect_error( "set, released handle", "invalid sprite handle", sprite_set_z_order( probe, 1 ) )
local recycled = sprite_acquire( "z_probe" )   -- capacity 1: the same slot, a new generation
check( recycled ~= nil and recycled ~= probe, "the recycled handle must be new: " .. tostring( recycled ) )
z_is( "recycled sprite", 0, sprite_get_z_order( recycled ) )

-- none of the above needs a window, so none of it may create the renderer
check( renderer_get_current() == nil, "a sprite z-order call created the renderer" )

-- part 2, real sprites on layers: own values, kept across a layer move and a reconfigure
check( pool_register_type( "sunny", 2 ) == true, "pool_register_type sunny failed" )
local a = sprite_acquire( "sunny" )
local b = sprite_acquire( "sunny" )
check( a ~= nil and b ~= nil, "sprite_acquire sunny failed" )

for _, h in ipairs( { a, b } ) do
    check( sprite_configure_texture( h, 0, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 ) == true,
           "sprite_configure_texture failed" )
    check( sprite_set_active_sequence( h, 0 ) == true, "sprite_set_active_sequence failed" )
    check( sprite_add_to_layer( h, __FIRST_LAYER ) == true, "sprite_add_to_layer failed" )
    check( sprite_set_visible( h, true ) == true, "sprite_set_visible failed" )
end

check( sprite_set_z_order( a, 10 ) == true, "setting a failed" )
z_is( "a", 10, sprite_get_z_order( a ) )
z_is( "b, after setting a", 0, sprite_get_z_order( b ) )

check( sprite_remove_from_layer( a, __FIRST_LAYER ) == true and sprite_add_to_layer( a, __SECOND_LAYER ) == true,
       "moving a to another layer failed" )
z_is( "a, after a layer move", 10, sprite_get_z_order( a ) )
check( sprite_configure_texture( a, 0, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 200 ) == true,
       "reconfiguring a failed" )
z_is( "a, after a reconfigure", 10, sprite_get_z_order( a ) )

check( sprite_set_z_order( b, -3 ) == true, "setting b failed" )

local frames = 0
function on_update( dt )
    frames = frames + 1

    -- the values survive real frames of drawing (and changing one mid-run is fine)
    if frames == 2 then check( sprite_set_z_order( b, 7 ) == true, "changing b between frames failed" ) end

    if frames == __DRAW_FRAMES then
        z_is( "a, after drawing", 10, sprite_get_z_order( a ) )
        z_is( "b, after drawing", 7, sprite_get_z_order( b ) )

        if #failures == 0 then
            print( "SPRITE ZORDER SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "SPRITE ZORDER SMOKE FAIL: " .. f ) end
        end
    end
end
