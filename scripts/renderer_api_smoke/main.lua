--[[
    Renderer API smoke test - run by scripts/renderer_api_smoke.py (and ci.yml, every
    platform) once per scenario, because a process gets exactly ONE renderer:

        RENDERER_SMOKE_SCENARIO=<name> scarab --headless --fast --max-frames 200 \
            scripts/renderer_api_smoke/project.json

    Scenarios: defaults, create, errors, lazy, nowindow, views, views_norenderer. Each records its own failures and
    the script quits ITSELF only if there were none (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3 - a failure with
    the details in the log, not a hang.
]]

io.stdout:setvbuf( "line" )

local scenario = os.getenv( "RENDERER_SMOKE_SCENARIO" ) or "?"
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

local function check_config( label, cfg, want )
    for key, expected in pairs( want ) do
        if type( expected ) == "table" then
            for sub, subExpected in pairs( expected ) do
                check( cfg[key] ~= nil and cfg[key][sub] == subExpected,
                       label .. ": " .. key .. "." .. sub .. " = " .. tostring( cfg[key] and cfg[key][sub] ) .. ", want " .. tostring( subExpected ) )
            end
        else
            check( cfg[key] == expected, label .. ": " .. key .. " = " .. tostring( cfg[key] ) .. ", want " .. tostring( expected ) )
        end
    end
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

if scenario == "defaults" then
    -- no options at all: exactly Scarab's defaults (main.h)
    check( renderer_get_current() == nil, "a renderer existed before renderer_create" )
    local r, err = renderer_create()
    check( r ~= nil, "renderer_create() failed: " .. tostring( err ) )
    local cur, origin = renderer_get_current()
    check( cur == r and origin == "created", "renderer_get_current after create: " .. tostring( cur ) .. ", " .. tostring( origin ) )
    check_config( "defaults", renderer_get_config( r ), {
        backend = RENDERER_BACKEND_NULL, width = 1260, height = 920, title = "Scarab", fps = 60,
        resizeable = true, draw_fps = false, zoom = 3.8125, exit_key = KEY_ESCAPE,
        stretch_to_fill = false, view_control_mode = VIEW_CONTROL_MODE_ACTIVE, use_default_key_handler = false,
        viewport = { x = 10, y = 10, w = 1240, h = 900 }, scroll_step = { w = 1, h = 1 } } )

elseif scenario == "create" then
    -- every option set to a NON-default value, then read back: proves the values are applied
    local r, err = renderer_create{
        title = "Smoke", width = 1000, height = 700, fps = 30, resizeable = false, draw_fps = true,
        viewport = { x = 20, y = 30, w = 900, h = 600 }, zoom = 2.0625, scroll_step = { w = 2, h = 3 },
        view_control_mode = VIEW_CONTROL_MODE_REACTIVE, use_default_key_handler = true,
        exit_key = KEY_NULL, stretch_to_fill = true }
    check( r ~= nil, "renderer_create failed: " .. tostring( err ) )
    local cfg = renderer_get_config( r )
    check_config( "create", cfg, {
        backend = RENDERER_BACKEND_NULL, width = 1000, height = 700, title = "Smoke", fps = 30,
        resizeable = false, draw_fps = true, zoom = 2.0625, exit_key = KEY_NULL, stretch_to_fill = true,
        view_control_mode = VIEW_CONTROL_MODE_REACTIVE, use_default_key_handler = true,
        viewport = { x = 20, y = 30, w = 900, h = 600 }, scroll_step = { w = 2, h = 3 } } )
    check( renderer_get_backend( r ) == RENDERER_BACKEND_NULL, "renderer_get_backend is not NULL under --headless" )
    check( renderer_get_default_view( r ) == 0, "default view is not 0" )
    -- live: the title follows app_set_name
    app_set_name( "Renamed" )
    check( renderer_get_config( r ).title == "Renamed", "renderer_get_config title did not follow app_set_name" )
    -- live: the zoom follows zoom_in (one step = 0.0625)
    zoom_in()
    check( renderer_get_config( r ).zoom == 2.125, "renderer_get_config zoom did not follow zoom_in: " .. tostring( renderer_get_config( r ).zoom ) )

elseif scenario == "errors" then
    expect_error( "unknown option", "unknown option 'titel'", renderer_create{ titel = "x" } )
    expect_error( "not a table", "options table", renderer_create( 5 ) )
    expect_error( "width string", "'width' must be an integer", renderer_create{ width = "1260" } )
    expect_error( "width fraction", "'width' must be an integer", renderer_create{ width = 1260.5 } )
    expect_error( "width zero", "'width' must be between", renderer_create{ width = 0 } )
    expect_error( "title number", "'title' must be a string", renderer_create{ title = 5 } )
    expect_error( "resizeable string", "'resizeable' must be a boolean", renderer_create{ resizeable = "no" } )
    expect_error( "zoom string", "'zoom' must be a number", renderer_create{ zoom = "big" } )
    expect_error( "zoom not a multiple", "nearest valid is 3.8125", renderer_create{ zoom = 3.8 } )
    expect_error( "zoom too big", "is outside [0.0625, 16]", renderer_create{ zoom = 20 } )
    expect_error( "zoom zero", "is outside [0.0625, 16]", renderer_create{ zoom = 0 } )
    expect_error( "viewport missing field", "missing field 'h'", renderer_create{ viewport = { x = 0, y = 0, w = 10 } } )
    expect_error( "viewport unknown field", "unknown field 'z'", renderer_create{ viewport = { x = 0, y = 0, w = 10, h = 10, z = 1 } } )
    expect_error( "viewport too big", "must fit the render area", renderer_create{ viewport = { x = 10, y = 10, w = 2000, h = 900 } } )
    expect_error( "viewport w zero", "at least 1", renderer_create{ viewport = { x = 0, y = 0, w = 0, h = 10 } } )
    expect_error( "default viewport vs small area", "pass 'viewport' explicitly", renderer_create{ width = 800, height = 600 } )
    expect_error( "scroll_step zero", "'scroll_step.w' must be between", renderer_create{ scroll_step = { w = 0, h = 1 } } )
    expect_error( "bad view_control_mode", "'view_control_mode' must be between", renderer_create{ view_control_mode = 5 } )
    expect_error( "bad backend value", "'backend' must be between", renderer_create{ backend = 9 } )
    expect_error( "raylib under --headless", "--headless forces the null backend", renderer_create{ backend = RENDERER_BACKEND_RAYLIB } )
    -- handles: nothing exists yet
    expect_error( "config, no renderer", "unknown renderer 1", renderer_get_config( 1 ) )
    expect_error( "backend, bad handle", "expected a renderer handle", renderer_get_backend( "x" ) )
    expect_error( "destroy, no renderer", "unknown renderer 1", renderer_destroy( 1 ) )
    check( renderer_get_current() == nil, "a failed renderer_create must not create a renderer" )
    -- and after all that, a valid create still works, and a second one is refused
    local r, err = renderer_create{ title = "after errors" }
    check( r ~= nil, "renderer_create after failed attempts: " .. tostring( err ) )
    expect_error( "second renderer_create", "only one renderer per process", renderer_create{} )
    expect_error( "wrong handle", "unknown renderer 99", renderer_get_config( 99 ) )

elseif scenario == "lazy" then
    -- a window-needing call FIRST creates the default renderer; renderer_create then names it
    app_set_name( "Implicit" )
    local cur, origin = renderer_get_current()
    check( cur ~= nil and origin == "lazy_default", "after app_set_name: " .. tostring( cur ) .. ", " .. tostring( origin ) )
    local r, err = renderer_create{ title = "too late" }
    expect_error( "renderer_create after lazy", "'app_set_name'", r, err )
    expect_error( "renderer_create after lazy (rule)", "must be the first window-needing call", r, err )
    check( renderer_get_config( cur ).title == "Implicit", "the lazy default renderer lost its title" )

elseif scenario == "nowindow" then
    -- calls that need no window must NOT create a renderer as a side effect
    local platform = app_get_platform()
    local project  = load_json( "scripts/renderer_api_smoke/project.json" )
    local t0       = os.time()
    check( platform ~= nil, "app_get_platform returned nil" )
    check( project ~= nil and project.main_script == "main.lua", "load_json failed before renderer_create" )
    check( renderer_get_current() == nil, "a renderer-free call created a renderer" )
    local r, err = renderer_create{ title = "after renderer-free calls" }
    check( r ~= nil, "renderer_create after renderer-free calls: " .. tostring( err ) )
    local _, origin = renderer_get_current()
    check( origin == "created", "origin after create: " .. tostring( origin ) )

elseif scenario == "views" then
    local r = renderer_create{ title = "views" }
    check( r ~= nil, "renderer_create failed" )
    local v = renderer_get_default_view( r )
    check( v == 0, "default view is not 0" )

    -- zoom: exact round trip; strict validation with the nearest valid factor named
    check( view_set_zoom( v, 2.0625 ) == true, "view_set_zoom 2.0625 failed" )
    check( view_get_zoom( v ) == 2.0625, "view_get_zoom after set: " .. tostring( view_get_zoom( v ) ) )
    check( viewport_get_zoom_factor() == 2.0625, "the global viewport_get_zoom_factor disagrees with the default view" )
    expect_error( "zoom not a multiple", "nearest valid is 3.8125", view_set_zoom( v, 3.8 ) )
    expect_error( "zoom outside the scale", "is outside [0.0625, 16]", view_set_zoom( v, 17 ) )
    expect_error( "zoom string", "must be a number", view_set_zoom( v, "3" ) )
    view_zoom_in( v )
    check( view_get_zoom( v ) == 2.125, "view_zoom_in did not step by ZOOM_STEP: " .. tostring( view_get_zoom( v ) ) )
    view_zoom_out( v ); view_zoom_out( v )
    check( view_get_zoom( v ) == 2.0, "view_zoom_out did not step by ZOOM_STEP: " .. tostring( view_get_zoom( v ) ) )

    -- preferred zoom + reset
    check( view_set_preferred_zoom( v, 1.5 ) == true, "view_set_preferred_zoom failed" )
    check( view_get_preferred_zoom( v ) == 1.5, "view_get_preferred_zoom: " .. tostring( view_get_preferred_zoom( v ) ) )
    view_zoom_reset( v )
    check( view_get_zoom( v ) == 1.5, "view_zoom_reset did not return to the preferred zoom" )

    -- zoom limits: narrow, clamp, refuse out-of-limit zoom, widen again, reject min > max
    check( view_get_zoom_limits( v ) == ZOOM_FACTOR_MIN, "default lower limit is not ZOOM_FACTOR_MIN" )
    check( view_set_zoom_limits( v, 2.0, 4.0 ) == true, "view_set_zoom_limits 2..4 failed" )
    local lo, hi = view_get_zoom_limits( v )
    check( lo == 2.0 and hi == 4.0, "view_get_zoom_limits: " .. tostring( lo ) .. ", " .. tostring( hi ) )
    check( view_get_zoom( v ) == 2.0, "narrowing the limits did not clamp the current zoom: " .. tostring( view_get_zoom( v ) ) )
    check( view_get_preferred_zoom( v ) == 2.0, "narrowing the limits did not clamp the preferred zoom: " .. tostring( view_get_preferred_zoom( v ) ) )
    expect_error( "zoom below the limit", "outside this view's zoom limits [2, 4]", view_set_zoom( v, 1.0 ) )
    expect_error( "min above max", "must not exceed", view_set_zoom_limits( v, 4.0, 2.0 ) )
    check( view_set_zoom_limits( v, 8.0, 16.0 ) == true, "moving the limits wholly above the old max failed" )
    lo, hi = view_get_zoom_limits( v )
    check( lo == 8.0 and hi == 16.0, "limits after moving up: " .. tostring( lo ) .. ", " .. tostring( hi ) )
    check( view_set_zoom_limits( v, ZOOM_FACTOR_MIN, ZOOM_FACTOR_MAX ) == true, "widening the limits again failed" )
    check( view_set_zoom( v, 3.8125 ) == true, "view_set_zoom after widening failed" )

    -- zoom enabled: stepping is locked, an exact zoom still works
    check( view_get_zoom_enabled( v ) == true, "zoom is not enabled by default" )
    view_set_zoom_enabled( v, false )
    check( view_get_zoom_enabled( v ) == false, "view_set_zoom_enabled(false) did not stick" )
    view_zoom_in( v )
    check( view_get_zoom( v ) == 3.8125, "view_zoom_in moved the zoom while zoom was disabled" )
    check( view_set_zoom( v, 3.75 ) == true, "view_set_zoom must still work while zoom stepping is disabled" )
    view_set_zoom_enabled( v, true )
    expect_error( "enabled must be boolean", "must be a boolean", view_set_zoom_enabled( v, 1 ) )

    -- dimension: real width/height, must fit the render area (1260 x 920)
    local x, y, w, h = view_get_dimension( v )
    check( x == 10 and y == 10 and w == 1240 and h == 900, "default view dimension: " .. table.concat( { x, y, w, h }, "," ) )
    check( view_set_dimension( v, 0, 0, 1260, 920 ) == true, "view_set_dimension to the whole render area failed" )
    x, y, w, h = view_get_dimension( v )
    check( x == 0 and y == 0 and w == 1260 and h == 920, "view_get_dimension after set: " .. table.concat( { x, y, w, h }, "," ) )
    local gx, gy, gw, gh = viewport_get_dimension()
    check( gx == 0 and gy == 0 and gw == 1260 and gh == 920, "the global viewport_get_dimension disagrees with the default view" )
    expect_error( "viewport too wide", "must fit the render area", view_set_dimension( v, 10, 10, 1260, 900 ) )
    expect_error( "viewport w zero", "w and h must be at least 1", view_set_dimension( v, 0, 0, 0, 10 ) )
    expect_error( "viewport negative x", "x and y must be at least 0", view_set_dimension( v, -1, 0, 10, 10 ) )
    expect_error( "viewport fraction", "must be an integer", view_set_dimension( v, 0, 0, 10.5, 10 ) )
    check( view_set_dimension( v, 10, 10, 1240, 900 ) == true, "restoring the default viewport failed" )

    -- scroll step + camera
    check( view_set_scroll_step( v, 4, 5 ) == true, "view_set_scroll_step failed" )
    local sw, sh = view_get_scroll_step( v )
    check( sw == 4 and sh == 5, "view_get_scroll_step: " .. tostring( sw ) .. ", " .. tostring( sh ) )
    check( renderer_get_config( r ).scroll_step.w == 4, "renderer_get_config scroll_step is not live" )
    expect_error( "scroll step zero", "w and h must be at least 1", view_set_scroll_step( v, 0, 1 ) )
    check( view_set_camera_position( v, 7, 9 ) == true, "view_set_camera_position failed" )
    local cx, cy = view_get_camera_position( v )
    check( cx == 7 and cy == 9, "view_get_camera_position: " .. tostring( cx ) .. ", " .. tostring( cy ) )
    local gcx, gcy = camera_get_position()
    check( gcx == 7 and gcy == 9, "the global camera_get_position disagrees with the default view" )
    check( view_reset_camera( v ) == true, "view_reset_camera failed" )
    cx, cy = view_get_camera_position( v )
    check( cx == 0 and cy == 0, "view_reset_camera did not return to the origin" )
    -- up/left need a loaded map (their limit is its size); down/right do not. With none loaded the
    -- old globals used to crash the process (sunlight dereferenced the missing map) - now a no-op / an error.
    expect_error( "move up, no map", "no map is loaded", view_move_camera_up( v ) )
    expect_error( "move left, no map", "no map is loaded", view_move_camera_left( v ) )
    check( view_move_camera_down( v ) == true and view_move_camera_right( v ) == true, "view_move_camera_down/right failed with no map" )
    camera_move_up(); camera_move_left()   -- must survive with no map loaded
    -- and with a real map loaded (the runner working directory is the repo root) they work
    check( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER ), "resources/tilemap/test.tmx failed to load" )
    check( view_move_camera_up( v ) == true and view_move_camera_left( v ) == true, "view_move_camera_up/left failed with a map loaded" )

    -- extra views do not exist yet; unknown handles are reported, not raised
    expect_error( "view_create", "only one view supported yet", view_create( r, 0, 0, 100, 100 ) )
    expect_error( "view_destroy default", "the default view cannot be removed", view_destroy( v ) )
    expect_error( "unknown view", "unknown view 7", view_get_zoom( 7 ) )
    expect_error( "view handle type", "expected a view handle", view_get_zoom( "0" ) )

elseif scenario == "views_norenderer" then
    -- a view handle can only exist once a renderer does: asking first is an error and creates nothing
    expect_error( "view before any renderer", "unknown view 0", view_get_zoom( 0 ) )
    check( renderer_get_current() == nil, "a view call created a renderer" )

else
    failures[#failures + 1] = "unknown scenario '" .. scenario .. "'"
end

local frames = 0
function on_update( dt )
    frames = frames + 1
    if frames == 1 then
        if #failures == 0 then
            print( "RENDERER SMOKE OK: " .. scenario )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "RENDERER SMOKE FAIL [" .. scenario .. "]: " .. f ) end
        end
    end
end
