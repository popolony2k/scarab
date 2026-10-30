--[[
    Error-reporting smoke test - run by scripts/error_reporting_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/error_reporting_smoke/project.json

    Pins doc/MASTER_PLAN.md item D3: every Lua function that can fail reports it as
    `nil, "message"` (never a Lua error, never a silent no-op or a "safe" value), a function with
    nothing else to return returns `true`, and arguments are checked strictly (a wrong type, a
    missing argument, a fractional number where an integer is expected, a value out of range or
    not one of its constant group). One check per kind of failure of every module: sprite, pool,
    scripting queue, sound and songs, tilemap, camera, collision, app, input, timer, json,
    crypto, pack and text. It also checks that a call refused for its arguments creates no
    renderer.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local MAP          = "resources/tilemap/test.tmx"
local TEXTURE      = "resources/sprites/sunny_idle_down.png"
local SOUND        = "resources/sounds/stage_theme.wav"
local BAD_JSON     = "scripts/error_reporting_smoke/bad.json"
local GOOD_DIR     = "scripts/error_reporting_smoke"
local MISSING      = "scripts/error_reporting_smoke/does-not-exist"
local FRAMES       = 4      -- texture frames in sunny_idle_down.png
local MAP_LAYER    = 7      -- "monke", a layer of test.tmx
local NO_LAYER     = 200    -- a valid id no layer of test.tmx has
local SOUND_ID     = 7
local NO_SOUND_ID  = 8
local TIMER_ID     = 3
local SLOT_LIMIT   = 65535  -- the pool's total slots (16-bit handles)
local QUEUE_LIMIT  = 65535  -- a queued command's 16-bit parameter

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- calls f( ... ) protected: it must not raise, and must return nil + a message containing `needle`
local function expect_error( label, needle, f, ... )
    local ok, value, message = pcall( f, ... )

    if not ok then
        check( false, label .. ": raised a Lua error: " .. tostring( value ) )
        return
    end

    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- calls f( ... ) protected: it must return exactly true
local function expect_true( label, f, ... )
    local ok, value, message = pcall( f, ... )

    check( ok and value == true, label .. ": expected true, got " .. tostring( value ) .. " " .. tostring( message ) )
end

function on_load_stage( stageId ) return true end

-- the queue: true once queued; only the parameter can fail (0..65535)
expect_true( "sp_wait", sp_wait, 1 )
expect_error( "sp_wait fraction", "argument 'milliseconds' must be an integer", sp_wait, 1.5 )
expect_error( "sp_wait string", "must be an integer", sp_wait, "10" )
expect_error( "sp_wait missing", "must be an integer", sp_wait )
expect_error( "sp_wait too big", "must be between 0 and 65535", sp_wait, QUEUE_LIMIT + 1 )
expect_error( "sp_wait negative", "must be between 0 and 65535", sp_wait, -1 )
expect_error( "sp_load_stage", "argument 'stageId'", sp_load_stage, "first" )
expect_error( "sp_add_label", "argument 'id'", sp_add_label, nil )
expect_error( "sp_play_song", "argument 'id'", sp_play_song, QUEUE_LIMIT + 1 )

-- refused for its arguments before any window: no renderer may exist after these
expect_error( "app_set_name number", "argument 'name' must be a string", app_set_name, 5 )
expect_error( "configure, invalid handle", "invalid sprite handle", sprite_configure_texture, 0, 0, TEXTURE, FRAMES, 0,
              TEXTURE_ANIMATION_MODE_MANUAL )
expect_error( "load_map number", "argument 'path' must be a string", tilemap_load_map, 5 )
check( renderer_get_current() == nil, "a call refused for its arguments created the renderer" )

-- pool
expect_error( "pool tag number", "argument 'typeTag' must be a string", pool_register_type, 5, 1 )
expect_error( "pool capacity 0", "argument 'capacity' must be between 1", pool_register_type, "a", 0 )
expect_true( "pool register", pool_register_type, "a", 1 )
expect_error( "pool duplicate", "is already registered", pool_register_type, "a", 1 )
expect_error( "pool too big", "at most 65535 sprites", pool_register_type, "huge", SLOT_LIMIT )
expect_error( "acquire unknown", "unknown pool type 'nope'", sprite_acquire, "nope" )

local sp = sprite_acquire( "a" )

check( math.type( sp ) == "integer" and sp ~= 0, "sprite_acquire returned " .. tostring( sp ) )
expect_error( "acquire full", "pool type 'a' is full", sprite_acquire, "a" )

-- sprite: handle, then each argument
expect_error( "configure frames 0", "argument 'framesByTexture'", sprite_configure_texture, sp, 0, TEXTURE, 0, 0,
              TEXTURE_ANIMATION_MODE_MANUAL )
expect_error( "configure tile index", "argument 'activeTileIndex' must be between 0 and 3", sprite_configure_texture, sp, 0, TEXTURE,
              FRAMES, FRAMES, TEXTURE_ANIMATION_MODE_MANUAL )
expect_error( "configure mode", "unknown TEXTURE_ANIMATION_MODE_* value 99", sprite_configure_texture, sp, 0, TEXTURE, FRAMES, 0, 99 )
expect_error( "configure delay", "argument 'delayMilli' must be -1", sprite_configure_texture, sp, 0, TEXTURE, FRAMES, 0,
              TEXTURE_ANIMATION_MODE_MANUAL, -2 )
expect_error( "configure missing file", "cannot load texture", sprite_configure_texture, sp, 0, MISSING .. ".png", FRAMES, 0,
              TEXTURE_ANIMATION_MODE_MANUAL )
expect_true( "configure", sprite_configure_texture, sp, 0, TEXTURE, FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 )
expect_error( "active sequence unconfigured", "sequence 5 is not configured", sprite_set_active_sequence, sp, 5 )
expect_true( "active sequence", sprite_set_active_sequence, sp, 0 )
expect_error( "animation mode unconfigured", "sequence 5 is not configured", sprite_set_animation_mode, sp, 5,
              TEXTURE_ANIMATION_MODE_MANUAL )
expect_error( "animation mode bad", "TEXTURE_ANIMATION_MODE_* constant", sprite_set_animation_mode, sp, 0, "manual" )
expect_error( "visible nil", "argument 'visible' must be a boolean", sprite_set_visible, sp, nil )
expect_error( "visible 1", "must be a boolean", sprite_set_visible, sp, 1 )
expect_true( "visible", sprite_set_visible, sp, true )
expect_error( "world space 0", "argument 'world_space' must be a boolean", sprite_set_world_space, sp, 0 )
expect_error( "opacity 255", "argument 'opacity' must be between 0 and 1", sprite_set_opacity, sp, 255 )
expect_error( "opacity string", "argument 'opacity' must be a number", sprite_set_opacity, sp, "1" )
expect_true( "opacity", sprite_set_opacity, sp, 0.5 )
expect_error( "get opacity bad handle", "invalid sprite handle", sprite_get_opacity, 0 )
expect_error( "tint channel", "argument 'g' must be between 0 and 255", sprite_set_tint, sp, 0, 256, 0 )
expect_error( "tint alpha", "argument 'a' must be an integer", sprite_set_tint, sp, 0, 0, 0, 0.5 )
expect_true( "tint", sprite_set_tint, sp, 255, 255, 255 )
expect_error( "get tint bad handle", "invalid sprite handle", sprite_get_tint, 0 )
expect_error( "set_pos fraction", "argument 'x' must be an integer", sprite_set_pos, sp, 3.5, 4 )
expect_error( "set_pos missing y", "argument 'y' must be an integer", sprite_set_pos, sp, 3 )
expect_true( "set_pos whole float", sprite_set_pos, sp, 3.0, 4 )
check( select( 1, sprite_get_pos( sp ) ) == 3, "sprite_set_pos( 3.0 ) did not store 3" )
expect_error( "inset range", "argument 'rightPct' must be between 0 and 1", sprite_set_collision_inset, sp, 0, 0, 1.5, 0 )
expect_true( "inset", sprite_set_collision_inset, sp, 0.1, 0.1, 0.1, 0.1 )
expect_error( "layer out of range", "argument 'layerId' must be between 0 and 254", sprite_add_to_layer, sp, 255 )

for _, getter in ipairs( { sprite_get_visible, sprite_get_world_space, sprite_get_pos, sprite_get_size,
                            sprite_get_active_sequence } ) do
    expect_error( "getter, invalid handle", "invalid sprite handle", getter, 0 )
end

-- sound and songs
expect_error( "sound_load missing", "cannot load sound", sound_load, SOUND_ID, MISSING .. ".wav" )
expect_error( "sound_load id", "argument 'id' must be an integer", sound_load, "seven", SOUND )

-- a machine with no audio device (a CI runner) can't load a sound: then only the not-loaded checks run
local soundLoaded, soundError = sound_load( SOUND_ID, SOUND )

if soundLoaded then
    expect_error( "sound_load twice", "is already loaded", sound_load, SOUND_ID, SOUND )
    expect_error( "volume range", "argument 'volume' must be between 0 and 1", sound_set_volume, SOUND_ID, 1.5 )
    expect_true( "volume", sound_set_volume, SOUND_ID, 0.5 )
else
    check( tostring( soundError ):find( "cannot load sound", 1, true ), "sound_load failed oddly: " .. tostring( soundError ) )
    print( "ERROR REPORTING SMOKE NOTE: no audio here (" .. tostring( soundError ) .. "), loaded-sound checks skipped" )
end

for _, f in ipairs( { sound_play, sound_stop, sound_pause, sound_resume, sound_is_playing, sound_unload, play_song,
                      pause_song, stop_song, resume_song, play_song_looping } ) do
    expect_error( "sound, not loaded", "sound 8 is not loaded", f, NO_SOUND_ID )
end

expect_error( "play_song range", "must be between 0 and 65535", play_song, QUEUE_LIMIT + 1 )
expect_true( "play_song no audio", play_song, 0 )
if soundLoaded then
    expect_true( "sound_unload", sound_unload, SOUND_ID )
end

expect_error( "unloaded", "sound 7 is not loaded", sound_play, SOUND_ID )

-- tilemap and camera, before any map
expect_error( "map info, no map", "no map is loaded", tilemap_get_map_info )
expect_error( "unload, no map", "no map is loaded", tilemap_unload_map )
expect_error( "layer, no map", "no map is loaded", tilemap_get_layer, 1 )
expect_error( "camera up, no map", "no map is loaded", camera_move_up )
expect_error( "camera left, no map", "no map is loaded", camera_move_left )
expect_true( "camera down, no map", camera_move_down )
expect_error( "load_map missing", "cannot load map", tilemap_load_map, MISSING .. ".tmx" )
expect_error( "load_map alignment", "unknown MAP_ALIGNMENT_* value 99", tilemap_load_map, MAP, 99 )
expect_true( "load_map", tilemap_load_map, MAP, MAP_ALIGNMENT_TOP_LEFT )
expect_error( "layer unknown", "no layer 200 in the loaded map", tilemap_get_layer, NO_LAYER )
expect_error( "layer by name unknown", "no layer named 'nope'", tilemap_get_layer_by_name, "nope" )
expect_error( "set_layer visible", "argument 'visible' must be a boolean", tilemap_set_layer, 1, "yes", 1, 0, 0 )
expect_error( "set_layer unknown", "no layer 200", tilemap_set_layer, NO_LAYER, true, 1, 0, 0 )
expect_error( "set_layer opacity", "argument 'opacity' must be between 0 and 1", tilemap_set_layer, 1, true, 255, 0, 0 )
expect_error( "layer tint unknown", "no layer 200", tilemap_get_layer_tint, NO_LAYER )
expect_error( "layer tint by name", "no layer named 'nope'", tilemap_get_layer_tint_by_name, "nope" )
expect_error( "set tint channel", "argument 'b' must be between 0 and 255", tilemap_set_layer_tint, 1, 0, 0, 300, 0 )
expect_error( "set tint by name", "argument 'layerName' must be a string", tilemap_set_layer_tint_by_name, 1, 0, 0, 0, 0 )
expect_error( "tile outside", "no tile at row 999", tilemap_get_tile, 999, 0, 1 )
expect_error( "matrix outside", "is outside the map", tilemap_to_tile_matrix, -100, -100 )
expect_error( "object missing", "no object named 'nope'", tilemap_get_object_by_name, "nope" )
expect_error( "camera fraction", "argument 'x' must be an integer", camera_set_position, 0.5, 0 )
expect_true( "camera_set_position", camera_set_position, 0, 0 )
expect_true( "camera up", camera_move_up )

-- sprite layers, with a map loaded
expect_error( "add_to_layer unknown", "layer 200 is not a layer of the loaded map", sprite_add_to_layer, sp, NO_LAYER )
expect_true( "add_to_layer", sprite_add_to_layer, sp, MAP_LAYER )
expect_error( "add_to_layer twice", "already on layer 7", sprite_add_to_layer, sp, MAP_LAYER )
expect_true( "remove_from_layer", sprite_remove_from_layer, sp, MAP_LAYER )
expect_error( "remove_from_layer twice", "is not on layer 7", sprite_remove_from_layer, sp, MAP_LAYER )
expect_true( "release", sprite_release, sp )
expect_error( "release twice", "invalid sprite handle", sprite_release, sp )

-- collision
expect_error( "rule range", "argument 'layerB' must be between 0 and 254", collision_add_rule, 1, 255 )
expect_true( "rule", collision_add_rule, 1, 2 )
expect_error( "rule twice", "is already declared", collision_add_rule, 1, 2 )
expect_error( "tile rule unknown", "layer 200 is not a layer of the loaded map", collision_add_tile_rule, 1, NO_LAYER )
expect_error( "handler", "argument 'fn' must be a function", collision_set_handler, "f" )
expect_true( "handler", collision_set_handler, function() end )

-- app
expect_error( "fade range", "argument 'alpha' must be between 0 and 1", screen_fade, 1.5 )
expect_true( "fade", screen_fade, 0 )
expect_error( "fullscreen nil", "argument 'fullscreen' must be a boolean", app_set_fullscreen, nil )
expect_error( "fullscreen strategy", "unknown FULLSCREEN_STRATEGY_* value 99", app_set_fullscreen, false, 99 )
expect_error( "fps 0", "argument 'fps' must be between 1", app_set_target_fps, 0 )
expect_error( "fps fraction", "argument 'fps' must be an integer", app_set_target_fps, 30.5 )
expect_error( "draw_fps", "argument 'draw_fps' must be a boolean", app_set_draw_fps, 1 )
expect_error( "rectangle colour", "argument 'a' must be between 0 and 255", draw_filled_rectangle, 0, 0, 1, 1, 0, 0, 0, 256 )
expect_error( "exit key unknown", "unknown KEY_* value 99999", app_set_exit_key, 99999 )
expect_true( "exit key off", app_set_exit_key, KEY_NULL )
expect_true( "exit key back", app_set_exit_key, KEY_ESCAPE )

-- input: a misspelt constant name is nil in Lua
expect_error( "key nil", "argument 'key' must be a KEY_* constant", input_is_key_down, KEY_MISSPELT )
expect_error( "key unknown", "unknown KEY_* value 99999", input_is_key_released, 99999 )
check( input_is_key_down( KEY_A ) == false, "input_is_key_down( KEY_A ) must read false headless" )
expect_error( "gamepad id", "argument 'gamepadId' must be between 0 and 9", input_is_gamepad_button_down, 10, GAMEPAD_BUTTON_LEFT_FACE_UP )
expect_error( "gamepad button", "unknown GAMEPAD_BUTTON_* value 999", input_is_gamepad_button_up, 0, 999 )
expect_error( "gamepad axis", "unknown GAMEPAD_AXIS_* value 999", input_get_gamepad_axis, 0, 999 )
expect_error( "add gamepad", "argument 'gamepadId'", input_add_gamepad, -1 )
expect_true( "add gamepad", input_add_gamepad, 0 )

-- timers
expect_error( "timer interval", "argument 'intervalMilli' must be between 1", set_timer, TIMER_ID, 0, function() end )
expect_error( "timer callback", "argument 'callback' must be a function", set_timer, TIMER_ID, 1000, "f" )
expect_true( "timer", set_timer, TIMER_ID, 100000, function() end )
expect_error( "timer twice", "timer 3 is already set", set_timer, TIMER_ID, 1000, function() end )
expect_true( "reset_timer", reset_timer, TIMER_ID )
expect_error( "reset_timer twice", "timer 3 is not set", reset_timer, TIMER_ID )

-- json, crypto, pack
expect_error( "json missing", "cannot open", load_json, MISSING .. ".json" )
expect_error( "json bad", "cannot parse", load_json, BAD_JSON )
expect_error( "json path", "argument 'path' must be a string", load_json, nil )
expect_error( "encrypt type", "argument 'data' must be a string", crypto_encrypt_data, 5 )
expect_error( "decrypt garbage", "", crypto_decrypt_data, "not encrypted" )
expect_error( "list missing", "is not a directory", pack_list_directory, MISSING )
check( type( pack_list_directory( GOOD_DIR ) ) == "table", "pack_list_directory of a real directory must return a table" )
expect_error( "read missing", "cannot open", pack_read_file, MISSING )
expect_error( "archive bad path", "cannot create archive", pack_create_archive, MISSING .. "/sub/x.zip" )
expect_error( "add_entry handle", "invalid archive handle", pack_add_entry, 9999, "a", "b" )
expect_error( "close handle", "invalid archive handle", pack_close_archive, 9999 )

local frames = 0

function on_update( dt )
    frames = frames + 1

    if frames == 1 then
        -- text, inside a frame
        expect_true( "draw_text", draw_text, "ok", 10, 10, 16, 255, 255, 255, 255 )
        expect_true( "draw_text number", draw_text, 42, 10, 30, 16, 255, 255, 255, 255 )
        expect_error( "draw_text size", "argument 'size' must be between 1", draw_text, "x", 10, 10, 0, 255, 255, 255, 255 )
        expect_error( "draw_text colour", "argument 'g' must be between 0 and 255", draw_text, "x", 10, 10, 16, 0, 300, 0, 0 )
        expect_error( "draw_text text", "argument 'text' must be a string", draw_text, {}, 10, 10, 16, 0, 0, 0, 0 )
        expect_error( "measure size", "argument 'size'", measure_text, "x", 1.5 )
        check( math.type( measure_text( 7, 16 ) ) == "integer", "measure_text of a number must return a width" )
    elseif frames == 2 then
        if #failures == 0 then
            print( "ERROR REPORTING SMOKE OK" )
            app_quit()
        else
            for _, message in ipairs( failures ) do
                print( "ERROR REPORTING SMOKE FAIL: " .. message )
            end
        end
    end
end
