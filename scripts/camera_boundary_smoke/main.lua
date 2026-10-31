--[[
    Camera boundary smoke test - run by scripts/camera_boundary_smoke.py (and ci.yml, every
    platform):

        scarab --headless --fast --max-frames 200 scripts/camera_boundary_smoke/project.json

    Pins how camera_move_* / view_move_camera_* behave at the edges of the map (sunlight's
    Camera::MoveUp/Down/Left/Right, plans/MASTER_PLAN.md item V1):

      * up/left scroll towards the map's far edge and stop EXACTLY on it, even when the
        distance is not a whole number of scroll steps - the last step is partial;
      * down/right scroll back and land EXACTLY on the origin (0), never a step short.

    The scroll step (7 px) deliberately does not divide any of the distances below, so a
    camera that refuses the partial last step (sunlight up to v0.38.0) stops short and fails.

    The far edge itself is -ceil(viewport size / zoom - map size), taken from sunlight's own
    Camera::FarCameraLimit (src/renderer/camera/camera.cpp; negated because the position Lua
    reads is the map point at the view's top-left, Camera::GetPosition's -m_Position) - it is
    checked here exactly, together with the formula-free part (the travel is NOT a multiple
    of the step).

    Two views of resources/tilemap/test.tmx (20x20 tiles of 32 px = 640x640):
      * the default view, Scarab's defaults: viewport 1240x900 at zoom 3.8125,
        driven both by the old globals (camera_move_*) and by view_move_camera_*(0);
      * an extra view, 300x200 at zoom 1.0, driven by view_move_camera_*.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local MAP_SIZE      = 640     -- test.tmx: 20 tiles x 32 px, both ways
local SCROLL_STEP   = 7       -- divides none of the distances to the far limits below
local MAX_MOVES     = 1000    -- far more than any edge needs; a camera still moving after it is a failure
local EXTRA_VIEW    = { x = 0, y = 0, w = 300, h = 200 }

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- the far edge as Lua reads it: sunlight's Camera::FarCameraLimit, negated (see the header)
local function far_edge( viewportSize, zoom )
    return -math.ceil( viewportSize / zoom - MAP_SIZE )
end

-- call move() until the chosen camera coordinate stops changing; returns that coordinate
-- and how many calls actually moved it
local function move_until_stopped( move, coord )
    local last  = coord()
    local moves = 0

    for _ = 1, MAX_MOVES do
        move()
        local now = coord()
        if now == last then return now, moves end
        last  = now
        moves = moves + 1
    end

    return nil, moves
end

-- one axis of one view: towards the far edge, then back to the origin
local function check_axis( label, toFar, toOrigin, coord, edge )
    check( edge > 0 and edge % SCROLL_STEP ~= 0,
           label .. ": test setup - far edge " .. edge .. " must be positive and not a multiple of " .. SCROLL_STEP )

    local far, farMoves = move_until_stopped( toFar, coord )
    check( far ~= nil, label .. ": camera never stopped moving towards the far edge" )
    check( far == edge, label .. ": stopped at " .. tostring( far ) .. ", want the map's far edge " .. edge )
    check( far == nil or far % SCROLL_STEP ~= 0,
           label .. ": travel " .. tostring( far ) .. " is a whole number of steps - the partial last step was refused" )
    check( farMoves == math.ceil( edge / SCROLL_STEP ),
           label .. ": " .. farMoves .. " moves to the far edge, want " .. math.ceil( edge / SCROLL_STEP ) )

    local back, backMoves = move_until_stopped( toOrigin, coord )
    check( back == 0, label .. ": returned to " .. tostring( back ) .. ", want exactly the origin 0" )
    check( backMoves == math.ceil( edge / SCROLL_STEP ),
           label .. ": " .. backMoves .. " moves back to the origin, want " .. math.ceil( edge / SCROLL_STEP ) )
end

local function check_view( label, view, viewportW, viewportH, zoom, moves )
    local function cam_x() local x = view_get_camera_position( view ) return x end
    local function cam_y() local _, y = view_get_camera_position( view ) return y end

    check( view_get_zoom( view ) == zoom, label .. ": zoom is " .. tostring( view_get_zoom( view ) ) .. ", want " .. zoom )
    check( view_set_scroll_step( view, SCROLL_STEP, SCROLL_STEP ) == true, label .. ": view_set_scroll_step failed" )

    check( view_set_camera_position( view, 0, 0 ) == true, label .. ": view_set_camera_position failed" )
    check_axis( label .. " left/right", moves.left, moves.right, cam_x, far_edge( viewportW, zoom ) )
    check( cam_y() == 0, label .. ": moving left/right changed y" )

    check( view_set_camera_position( view, 0, 0 ) == true, label .. ": view_set_camera_position failed" )
    check_axis( label .. " up/down", moves.up, moves.down, cam_y, far_edge( viewportH, zoom ) )
    check( cam_x() == 0, label .. ": moving up/down changed x" )
end

local function view_moves( view )
    return { up    = function() view_move_camera_up( view ) end,
             down  = function() view_move_camera_down( view ) end,
             left  = function() view_move_camera_left( view ) end,
             right = function() view_move_camera_right( view ) end }
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

local r, err = renderer_create{ title = "camera_boundary_smoke" }
check( r ~= nil, "renderer_create failed: " .. tostring( err ) )

-- the runner's working directory is the repo root
check( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT ), "resources/tilemap/test.tmx failed to load" )

local extra = view_create( r, EXTRA_VIEW.x, EXTRA_VIEW.y, EXTRA_VIEW.w, EXTRA_VIEW.h )
check( extra ~= nil, "view_create failed" )

local _, _, defaultW, defaultH = viewport_get_dimension()
local defaultZoom              = viewport_get_zoom_factor()

check_view( "default view, camera_move_*", 0, defaultW, defaultH, defaultZoom,
            { up = camera_move_up, down = camera_move_down, left = camera_move_left, right = camera_move_right } )
check_view( "default view, view_move_camera_*", 0, defaultW, defaultH, defaultZoom, view_moves( 0 ) )

if extra then
    check_view( "extra view, view_move_camera_*", extra, EXTRA_VIEW.w, EXTRA_VIEW.h, 1.0, view_moves( extra ) )
end

local frames = 0
function on_update( dt )
    frames = frames + 1
    if frames == 1 then
        if #failures == 0 then
            print( "CAMERA BOUNDARY SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "CAMERA BOUNDARY SMOKE FAIL: " .. f ) end
        end
    end
end
