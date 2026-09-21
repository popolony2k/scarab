--[[
    Renderer API smoke test - run by scripts/renderer_api_smoke.py (and ci.yml, every
    platform) once per scenario, because a process gets exactly ONE renderer:

        RENDERER_SMOKE_SCENARIO=<name> scarab --headless --fast --max-frames 200 \
            scripts/renderer_api_smoke/project.json

    Scenarios: defaults, create, errors, lazy, nowindow, views, viewsmulti, views_norenderer. Each records its own failures and
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

    -- the default view cannot be removed; unknown handles are reported, not raised
    expect_error( "view_destroy default", "the default view cannot be removed", view_destroy( v ) )
    expect_error( "unknown view", "unknown view 7", view_get_zoom( 7 ) )
    expect_error( "view handle type", "expected a view handle", view_get_zoom( "0" ) )

elseif scenario == "viewsmulti" then
    -- real extra views (view_create and everything that configures one). The hidden-map-layer
    -- names/ids are those of resources/tilemap/test.tmx: 1 sky, 2 background_clouds, 3 background,
    -- 4 clouds, 5 houses, 6 smoke, 7 monke, 8 birb.
    local r = renderer_create{ title = "viewsmulti" }
    check( r ~= nil, "renderer_create failed" )
    check( renderer_get_view_count( r ) == 1, "a fresh renderer must have exactly the default view" )

    -- creation: ids >= 1, counted, listed with their own rectangle and the documented defaults
    local a, err = view_create( r, 1000, 20, 240, 240 )
    check( a ~= nil and a >= 1, "view_create failed: " .. tostring( err ) )
    local b = view_create( r, 20, 20, 300, 200 )
    check( b ~= nil and b > a, "the second view's id must be greater than the first's: " .. tostring( b ) )
    check( renderer_get_view_count( r ) == 3, "view count after two view_create: " .. tostring( renderer_get_view_count( r ) ) )
    local x, y, w, h = view_get_dimension( a )
    check( x == 1000 and y == 20 and w == 240 and h == 240, "new view rectangle: " .. table.concat( { x, y, w, h }, "," ) )
    check( view_get_visible( a ) == true, "a new view must start visible" )
    check( view_get_draw_order( a ) == a, "a new view's draw order must be its own id: " .. tostring( view_get_draw_order( a ) ) )
    check( view_get_clear_background( a ) == true, "a new view must start with its backdrop on" )
    check( view_get_zoom( a ) == 1.0, "a new view must start at zoom 1.0: " .. tostring( view_get_zoom( a ) ) )
    local cx, cy = view_get_camera_position( a )
    check( cx == 0 and cy == 0, "a new view's camera must start at the origin" )
    check( view_get_draw_order( 0 ) == 0, "the default view's draw order must be 0" )

    -- rejections: the same rules as view_set_dimension, plus the renderer handle
    expect_error( "view_create unknown renderer", "unknown renderer 5", view_create( 5, 0, 0, 10, 10 ) )
    expect_error( "view_create renderer type", "expected a renderer handle", view_create( "x", 0, 0, 10, 10 ) )
    expect_error( "view_create too wide", "must fit the render area", view_create( r, 1000, 0, 300, 100 ) )
    expect_error( "view_create w zero", "w and h must be at least 1", view_create( r, 0, 0, 0, 10 ) )
    expect_error( "view_create negative", "x and y must be at least 0", view_create( r, -1, 0, 10, 10 ) )
    expect_error( "view_create fraction", "must be an integer", view_create( r, 0, 0, 10.5, 10 ) )
    expect_error( "view_create missing args", "must be an integer", view_create( r, 0, 0 ) )
    check( renderer_get_view_count( r ) == 3, "a rejected view_create must not add a view" )

    -- visible / draw order / backdrop: exact round trips + validation
    check( view_set_visible( a, false ) == true and view_get_visible( a ) == false, "view_set_visible(false) did not stick" )
    check( view_get_visible( b ) == true and view_get_visible( 0 ) == true, "hiding one view changed another" )
    view_set_visible( a, true )
    expect_error( "visible type", "must be a boolean", view_set_visible( a, 1 ) )
    check( view_set_draw_order( a, -5 ) == true and view_get_draw_order( a ) == -5, "view_set_draw_order(-5) did not stick" )
    expect_error( "draw order type", "must be an integer", view_set_draw_order( a, 1.5 ) )
    expect_error( "draw order range", "out of range", view_set_draw_order( a, 1 << 40 ) )
    check( view_set_clear_background( a, false ) == true and view_get_clear_background( a ) == false, "view_set_clear_background(false) did not stick" )
    view_set_clear_background( a, true )
    expect_error( "clear type", "must be a boolean", view_set_clear_background( a, "yes" ) )
    check( view_set_background_color( a, 0, 0, 0 ) == true, "view_set_background_color r,g,b failed" )
    check( view_set_background_color( a, 10, 20, 30, 128 ) == true, "view_set_background_color r,g,b,a failed" )
    expect_error( "colour range", "'g' must be between 0 and 255", view_set_background_color( a, 0, 256, 0 ) )
    expect_error( "colour negative", "'r' must be between 0 and 255", view_set_background_color( a, -1, 0, 0 ) )
    expect_error( "colour alpha", "'a' must be between 0 and 255", view_set_background_color( a, 0, 0, 0, 300 ) )
    expect_error( "colour type", "must be an integer", view_set_background_color( a, 0, 0, "blue" ) )
    check( view_use_map_background_color( a ) == true, "view_use_map_background_color failed" )

    -- each view has its own camera/zoom/limits, and none of it touches the default view
    check( view_set_zoom( a, 2.0 ) == true and view_set_camera_position( a, 11, 13 ) == true, "configuring view a failed" )
    check( view_set_zoom_limits( a, 0.5, 4.0 ) == true, "view_set_zoom_limits on an extra view failed" )
    local lo, hi = view_get_zoom_limits( a )
    check( lo == 0.5 and hi == 4.0, "an extra view's zoom limits: " .. tostring( lo ) .. ", " .. tostring( hi ) )
    lo, hi = view_get_zoom_limits( b )
    check( lo == ZOOM_FACTOR_MIN and hi == ZOOM_FACTOR_MAX, "another view's zoom limits changed: " .. tostring( lo ) .. ", " .. tostring( hi ) )
    check( view_get_zoom( b ) == 1.0, "configuring view a changed view b's zoom" )
    check( viewport_get_zoom_factor() == 3.8125, "configuring view a changed the default view's zoom" )
    local gx, gy = camera_get_position()
    check( gx == 0 and gy == 0, "configuring view a moved the default view's camera" )

    -- layer mask BEFORE a map is loaded: ids work, names need a map
    check( view_show_layer( a, 4, false ) == true, "view_show_layer by id before a map failed" )
    check( view_is_layer_shown( a, 4 ) == false, "layer 4 must be masked out in view a" )
    check( view_is_layer_shown( b, 4 ) == true and view_is_layer_shown( 0, 4 ) == true, "masking a layer in one view changed another view" )
    expect_error( "layer name, no map", "no layer named 'clouds'", view_show_layer( a, "clouds", false ) )
    expect_error( "fit, no map", "no map is loaded", view_fit_to_map( a ) )
    view_show_all_layers( a )
    check( view_is_layer_shown( a, 4 ) == true, "view_show_all_layers did not restore layer 4" )

    check( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT ), "resources/tilemap/test.tmx failed to load" )

    -- layer mask with the map loaded: by name, by id, only-list, all
    check( view_show_layer( a, "clouds", false ) == true, "view_show_layer by name failed" )
    check( view_is_layer_shown( a, 4 ) == false, "hiding 'clouds' by name must mask layer id 4" )
    expect_error( "layer name, unknown", "no layer named 'nope'", view_show_layer( a, "nope", true ) )
    check( view_show_layer( a, "clouds", true ) == true and view_is_layer_shown( a, 4 ) == true, "showing 'clouds' again failed" )
    check( view_show_only_layers( a, { 1, 5 } ) == true, "view_show_only_layers failed" )
    for id = 1, 8 do
        local want = ( id == 1 or id == 5 )
        check( view_is_layer_shown( a, id ) == want, "after show_only {1,5}: layer " .. id .. " shown=" .. tostring( view_is_layer_shown( a, id ) ) )
        check( view_is_layer_shown( b, id ) == true, "show_only in view a changed view b's layer " .. id )
        check( view_is_layer_shown( 0, id ) == true, "show_only in view a changed the default view's layer " .. id )
    end
    check( view_show_only_layers( a, {} ) == true and view_is_layer_shown( a, 1 ) == false, "an empty only-list must hide every layer" )
    view_show_all_layers( a )
    check( view_is_layer_shown( a, 5 ) == true, "view_show_all_layers failed" )
    expect_error( "only-layers type", "must be a table", view_show_only_layers( a, 4 ) )
    expect_error( "only-layers element", "layer_ids[2] must be an integer", view_show_only_layers( a, { 1, "x" } ) )
    expect_error( "layer arg type", "layer id (an integer) or a layer name", view_show_layer( a, true, true ) )
    expect_error( "layer show type", "must be a boolean", view_show_layer( a, 4, 1 ) )
    expect_error( "layer_id type", "must be an integer", view_is_layer_shown( a, "4" ) )

    -- fit to map: whole map inside the 240 x 240 rectangle, camera at the map's top-left
    check( view_fit_to_map( a ) == true, "view_fit_to_map failed with a map loaded" )
    local zoom = view_get_zoom( a )
    cx, cy = view_get_camera_position( a )
    check( zoom >= 0.5 and zoom <= 4.0, "fit zoom outside the view's limits: " .. tostring( zoom ) )
    check( cx == 0 and cy == 0, "view_fit_to_map must put the camera at the map's top-left: " .. tostring( cx ) .. ", " .. tostring( cy ) )
    local mapWidth, mapHeight = tilemap_get_map_info()
    check( mapWidth == 20 and mapHeight == 20, "map info " .. tostring( mapWidth ) .. "x" .. tostring( mapHeight ) )

    -- world-space sprites: off by default, a round trip, applies to sequences configured later,
    -- and a released handle's recycled slot never inherits it
    pool_register_type( "ws", 1 )
    local sp = sprite_acquire( "ws" )
    check( sp ~= 0, "sprite_acquire failed" )
    check( sprite_get_world_space( sp ) == false, "a new sprite must not be world-space" )
    sprite_set_world_space( sp, true )
    check( sprite_get_world_space( sp ) == true, "sprite_set_world_space(true) did not stick" )
    check( sprite_configure_texture( sp, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR ) == true, "sprite_configure_texture failed" )
    check( sprite_get_world_space( sp ) == true, "configuring a texture changed the world-space mode" )
    check( sprite_release( sp ) == true, "sprite_release failed" )
    check( sprite_get_world_space( sp ) == false, "a released handle must read as not world-space" )
    local sp2 = sprite_acquire( "ws" )
    check( sp2 ~= 0 and sp2 ~= sp, "the recycled slot must come back under a new handle" )
    check( sprite_get_world_space( sp2 ) == false, "a recycled slot inherited the previous owner's world-space mode" )
    sprite_set_world_space( 0, true )   -- an invalid handle is a quiet no-op, like every other sprite_set_*
    check( sprite_get_world_space( 0 ) == false, "an invalid handle must read as not world-space" )
    sprite_release( sp2 )

    -- destroy: counted, then unknown; ids are never reused; the default view stays
    check( view_destroy( a ) == true, "view_destroy failed" )
    check( renderer_get_view_count( r ) == 2, "view count after view_destroy: " .. tostring( renderer_get_view_count( r ) ) )
    expect_error( "destroyed view", "unknown view " .. a, view_get_zoom( a ) )
    expect_error( "destroy twice", "unknown view " .. a, view_destroy( a ) )
    expect_error( "destroyed view visible", "unknown view " .. a, view_set_visible( a, true ) )
    local c = view_create( r, 0, 0, 100, 100 )
    check( c ~= nil and c > b, "a view created after a destroy must get a fresh id, not " .. tostring( c ) )
    check( view_destroy( b ) == true and view_destroy( c ) == true, "destroying the remaining extra views failed" )
    check( renderer_get_view_count( r ) == 1, "only the default view must remain" )
    expect_error( "destroy default", "the default view cannot be removed", view_destroy( 0 ) )
    check( renderer_get_view_count( r ) == 1, "the default view was removed" )

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
