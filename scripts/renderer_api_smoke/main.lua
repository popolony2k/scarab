--[[
    Renderer API smoke test - run by scripts/renderer_api_smoke.py (and ci.yml, every
    platform) once per scenario, because a process gets exactly ONE renderer:

        RENDERER_SMOKE_SCENARIO=<name> scarab --headless --fast --max-frames 200 \
            scripts/renderer_api_smoke/project.json

    Scenarios: defaults, create, errors, lazy, nowindow. Each records its own failures and
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
