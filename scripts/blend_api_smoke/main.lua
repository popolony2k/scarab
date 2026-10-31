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
    Blend-mode API smoke test - run by scripts/blend_api_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/blend_api_smoke/project.json

    Pins the Lua side of blend modes (plans/MASTER_PLAN.md item B1): the BLEND_MODE_* constants,
    draw_begin_blend_mode / draw_end_blend_mode, and sprite_set / clear / get_blend_mode -
    values, round trips, every error, and that a recycled sprite slot never inherits its
    previous owner's mode. A headless run cannot see colour: what each mode LOOKS like is
    checked by eye with samples/blend.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __FRAMES_WITH_OPEN_BLEND <const> = 5    -- frames run after a begin with no end, before quitting
local __UNKNOWN_MODE           <const> = 9

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- the constants: sunlight's SunLight::Base::BlendMode, one to one, before any window exists
check( BLEND_MODE_NORMAL == 0 and BLEND_MODE_ADD == 1 and BLEND_MODE_MULTIPLY == 2 and BLEND_MODE_SUBTRACT == 3,
       "BLEND_MODE_* values: " .. tostring( BLEND_MODE_NORMAL ) .. " " .. tostring( BLEND_MODE_ADD ) .. " " ..
       tostring( BLEND_MODE_MULTIPLY ) .. " " .. tostring( BLEND_MODE_SUBTRACT ) )

-- a sprite's own mode: needs no renderer
check( pool_register_type( "blend_probe", 1 ), "pool_register_type failed" )
local sprite = sprite_acquire( "blend_probe" )
check( sprite ~= nil, "sprite_acquire failed" )

check( sprite_get_blend_mode( sprite ) == nil, "a fresh sprite must have no blend mode of its own" )
for _, mode in ipairs( { BLEND_MODE_NORMAL, BLEND_MODE_ADD, BLEND_MODE_MULTIPLY, BLEND_MODE_SUBTRACT } ) do
    check( sprite_set_blend_mode( sprite, mode ) == true, "sprite_set_blend_mode( " .. mode .. " ) failed" )
    check( sprite_get_blend_mode( sprite ) == mode, "sprite_get_blend_mode after setting " .. mode .. ": " .. tostring( sprite_get_blend_mode( sprite ) ) )
end
check( sprite_clear_blend_mode( sprite ) == true, "sprite_clear_blend_mode failed" )
check( sprite_get_blend_mode( sprite ) == nil, "sprite_clear_blend_mode left a mode" )

-- every rejection: nil + the reason, and nothing changes
check( sprite_set_blend_mode( sprite, BLEND_MODE_ADD ) == true, "sprite_set_blend_mode( ADD ) failed" )
expect_error( "unknown mode", "unknown blend mode " .. __UNKNOWN_MODE, sprite_set_blend_mode( sprite, __UNKNOWN_MODE ) )
expect_error( "negative mode", "unknown blend mode -1", sprite_set_blend_mode( sprite, -1 ) )
expect_error( "mode as a string", "expected a BLEND_MODE_* constant", sprite_set_blend_mode( sprite, "add" ) )
expect_error( "fractional mode", "expected a BLEND_MODE_* constant", sprite_set_blend_mode( sprite, 1.5 ) )
expect_error( "missing mode", "expected a BLEND_MODE_* constant", sprite_set_blend_mode( sprite ) )
check( sprite_get_blend_mode( sprite ) == BLEND_MODE_ADD, "a rejected sprite_set_blend_mode changed the mode" )
expect_error( "set, bad handle", "invalid sprite handle", sprite_set_blend_mode( 123456, BLEND_MODE_ADD ) )
expect_error( "set, handle as a string", "invalid sprite handle", sprite_set_blend_mode( "1", BLEND_MODE_ADD ) )
expect_error( "clear, bad handle", "invalid sprite handle", sprite_clear_blend_mode( 123456 ) )
expect_error( "get, bad handle", "invalid sprite handle", sprite_get_blend_mode( 123456 ) )

-- a recycled slot never inherits its previous owner's mode (SpritePool::Release clears it)
check( sprite_release( sprite ), "sprite_release failed" )
expect_error( "get, released handle", "invalid sprite handle", sprite_get_blend_mode( sprite ) )
local recycled = sprite_acquire( "blend_probe" )   -- capacity 1: the same slot, a new generation
check( recycled ~= nil and recycled ~= sprite, "the recycled handle must be new: " .. tostring( recycled ) )
check( sprite_get_blend_mode( recycled ) == nil, "a recycled sprite inherited blend mode " .. tostring( sprite_get_blend_mode( recycled ) ) )

-- none of the above needs a window, so none of it may create the renderer
check( renderer_get_current() == nil, "a sprite blend-mode call created the renderer" )

-- the scoped mode: rejections first (they change nothing), then real calls
expect_error( "begin, unknown mode", "unknown blend mode " .. __UNKNOWN_MODE, draw_begin_blend_mode( __UNKNOWN_MODE ) )
expect_error( "begin, mode as a string", "expected a BLEND_MODE_* constant", draw_begin_blend_mode( "add" ) )
check( draw_begin_blend_mode( BLEND_MODE_MULTIPLY ) == true, "draw_begin_blend_mode at load time failed" )
check( draw_end_blend_mode() == true, "draw_end_blend_mode at load time failed" )

local frames = 0
function on_update( dt )
    frames = frames + 1

    if frames == 1 then
        -- every mode around real drawing; the calls do not nest - a second begin replaces the first
        for _, mode in ipairs( { BLEND_MODE_NORMAL, BLEND_MODE_ADD, BLEND_MODE_MULTIPLY, BLEND_MODE_SUBTRACT } ) do
            check( draw_begin_blend_mode( mode ) == true, "draw_begin_blend_mode( " .. mode .. " ) failed" )
            draw_filled_rectangle( 10, 10, 100, 100, 255, 0, 0, 200 )
            draw_text( "blend", 10, 120, 20, 0, 255, 0, 200 )
            check( draw_end_blend_mode() == true, "draw_end_blend_mode after mode " .. mode .. " failed" )
        end
        check( draw_begin_blend_mode( BLEND_MODE_ADD ) == true and draw_begin_blend_mode( BLEND_MODE_SUBTRACT ) == true,
               "a second draw_begin_blend_mode must replace the first, not fail" )
        check( draw_end_blend_mode() == true, "draw_end_blend_mode after a replaced begin failed" )
        check( draw_end_blend_mode() == true, "draw_end_blend_mode with no begin must be harmless" )

        -- a begin with no end: the next frames must still run normally (each frame starts in normal)
        check( draw_begin_blend_mode( BLEND_MODE_ADD ) == true, "the open draw_begin_blend_mode failed" )
    elseif frames == 1 + __FRAMES_WITH_OPEN_BLEND then
        if #failures == 0 then
            print( "BLEND API SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "BLEND API SMOKE FAIL: " .. f ) end
        end
    end
end
