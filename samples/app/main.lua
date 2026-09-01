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
 app - window/app-level basics (docs/lua-api/app.html).

 Demonstrates app_get_platform, app_set_draw_fps/app_get_draw_fps,
 app_set_window_resizeable/app_get_window_resizeable, app_set_fullscreen/
 app_get_fullscreen, app_set_stretch_to_fill/app_get_stretch_to_fill,
 app_set_target_fps/app_get_target_fps, screen_fade, and
 draw_filled_rectangle - every primitive in LuaAppApi except app_set_name
 (already shown in hello-world). Also uses input_is_key_released/
 input_is_key_down (LuaInputApi) to drive the toggles - the "input"
 sample later covers that API on it's own.
]]

app_set_name( "Scarab - app sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

app_set_draw_fps( true )

-- Pick a startup fullscreen strategy appropriate to this platform - see
-- app_get_platform's own doc comment (src/lua/luaappapi.cpp) for why
-- FULLSCREEN_STRATEGY_REAL is kept macOS-only project-wide (a real
-- measured Linux performance regression, Windows unaffected but kept
-- consistent with Linux by deliberate choice).
local platform         = app_get_platform()
local platform_name    = "unknown"
local fullscreenStrategy = FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED

if platform == PLATFORM_WINDOWS then
    platform_name = "Windows"
elseif platform == PLATFORM_MACOS then
    platform_name      = "macOS"
    fullscreenStrategy = FULLSCREEN_STRATEGY_REAL
elseif platform == PLATFORM_LINUX then
    platform_name = "Linux"
end

-- Fade in from black over ~1 second at 60fps, the exact pattern from
-- screen_fade's own doc example.
local __FADE_STEP = 1 / 60
local fadeAlpha    = 1.0

local function control_line( key, label, state )
    return key .. " - " .. label .. ": " .. tostring( state )
end

function on_update( dt )

    if fadeAlpha > 0 then
        fadeAlpha = math.max( 0, fadeAlpha - __FADE_STEP )
        screen_fade( fadeAlpha )
    end

    if input_is_key_released( KEY_F1 ) then
        app_set_draw_fps( not app_get_draw_fps() )
    end

    if input_is_key_released( KEY_F2 ) then
        app_set_window_resizeable( not app_get_window_resizeable() )
    end

    if input_is_key_released( KEY_F3 ) then
        app_set_fullscreen( not app_get_fullscreen(), fullscreenStrategy )
    end

    if input_is_key_released( KEY_F4 ) then
        app_set_stretch_to_fill( not app_get_stretch_to_fill() )
    end

    if input_is_key_down( KEY_UP ) then
        app_set_target_fps( app_get_target_fps() + 1 )
    elseif input_is_key_down( KEY_DOWN ) then
        app_set_target_fps( math.max( 1, app_get_target_fps() - 1 ) )
    end

    draw_text( "app sample - platform: " .. platform_name, 20, 20, 24, 255, 255, 255, 255 )
    draw_text( control_line( "F1", "FPS counter", app_get_draw_fps() ), 20, 50, 20, 200, 200, 200, 255 )
    draw_text( control_line( "F2", "Window resizeable", app_get_window_resizeable() ), 20, 74, 20, 200, 200, 200, 255 )
    draw_text( control_line( "F3", "Fullscreen", app_get_fullscreen() ), 20, 98, 20, 200, 200, 200, 255 )
    draw_text( control_line( "F4", "Stretch to fill", app_get_stretch_to_fill() ), 20, 122, 20, 200, 200, 200, 255 )
    draw_text( "UP/DOWN - target FPS: " .. tostring( app_get_target_fps() ), 20, 146, 20, 200, 200, 200, 255 )

    -- A simple HUD "meter" bar, purely to demonstrate draw_filled_rectangle:
    -- a dark background plate, then a colored fill that tracks the startup
    -- fade so there's something visibly animated even after the fade ends.
    draw_filled_rectangle( 20, 184, 300, 24, 40, 40, 40, 255 )
    draw_filled_rectangle( 20, 184, math.floor( 300 * ( 1 - fadeAlpha ) ), 24, 60, 160, 220, 255 )
end
