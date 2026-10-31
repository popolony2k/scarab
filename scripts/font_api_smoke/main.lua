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
    Font API smoke test - run by scripts/font_api_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/font_api_smoke/project.json

    Pins Scarab's handle-based font API (plans/MASTER_PLAN.md item C6, over sunlight I3 "fonts by
    handle", BREAKAGES entry 52): load_font / release_font, set_font's three forms (a path, a
    handle, nil), register_font / unregister_font for the map's text objects, and draw_text /
    measure_text's optional font argument - every round trip and every error (nil + the reason).
    sunlight's null backend loads a font iff its file exists, so success and failure can both be
    checked headless; what a font LOOKS like is checked by eye (samples/text, samples/objects).

    The runner's working directory is the repo root, so font paths are repo-relative.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __REGULAR <const> = "resources/fonts/caravellius8x8.fnt"
local __BOLD    <const> = "resources/fonts/caravellius8x8_bold.fnt"
local __TTF     <const> = "resources/fonts/pressstart2p-regular.ttf"
local __MISSING <const> = "resources/fonts/this-font-does-not-exist.ttf"
local __SIZE    <const> = 20
local __BAD     <const> = 123456          -- a number that is not a live handle

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

-- load_font: a handle per load; every failure is nil + the reason
local regular, err = load_font( __REGULAR )
check( math.type( regular ) == "integer", "load_font( regular ) failed: " .. tostring( err ) )
local bold = load_font( __BOLD )
local ttf  = load_font( __TTF )
check( math.type( bold ) == "integer" and math.type( ttf ) == "integer", "load_font( bold / ttf ) failed" )
check( regular ~= bold and bold ~= ttf and regular ~= ttf, "load_font gave the same handle twice" )
expect_error( "load missing", "cannot load font '" .. __MISSING .. "'", load_font( __MISSING ) )
expect_error( "load number", "load_font expects a path", load_font( 5 ) )
expect_error( "load none", "load_font expects a path", load_font() )

-- set_font: a handle, a path (load + select), nil (built-in); failures keep the active font
check( set_font( regular ) == true, "set_font( handle ) failed" )
check( set_font( __TTF ) == true, "set_font( path ) failed" )
check( set_font( nil ) == true, "set_font( nil ) failed" )
expect_error( "set missing path", "cannot load font '" .. __MISSING .. "'", set_font( __MISSING ) )
expect_error( "set bad handle", "invalid font handle", set_font( __BAD ) )
expect_error( "set none", "set_font expects a path, a font or nil", set_font() )
expect_error( "set boolean", "set_font expects a path, a font or nil", set_font( true ) )

-- draw_text / measure_text: the optional last argument
check( measure_text( "Hello", __SIZE ) > 0, "measure_text with the active font gave no width" )
check( measure_text( "Hello", __SIZE, regular ) > 0, "measure_text with a font gave no width" )
check( measure_text( "", __SIZE, regular ) == 0, "measure_text of an empty string is not 0" )
expect_error( "measure bad font", "invalid font handle", measure_text( "Hello", __SIZE, __BAD ) )
expect_error( "measure font as a string", "invalid font handle", measure_text( "Hello", __SIZE, "x" ) )

-- register_font / unregister_font: by handle and by slot
check( register_font( "Sans", regular ) == true, "register_font( Sans, regular ) failed" )
check( register_font( "Sans", bold, true ) == true, "register_font( Sans, bold, true ) failed" )
check( register_font( "Sans", regular, false, true ) == true, "register_font( Sans italic ) failed" )
check( register_font( "Sans", regular ) == true, "registering the same slot again must replace it" )
expect_error( "register empty family", "non-empty string", register_font( "", regular ) )
expect_error( "register number family", "non-empty string", register_font( 7, regular ) )
expect_error( "register bad font", "invalid font handle", register_font( "Sans", __BAD ) )
expect_error( "register no font", "invalid font handle", register_font( "Sans" ) )
expect_error( "register bold string", "'bold' must be a boolean", register_font( "Sans", regular, "yes" ) )
expect_error( "register italic number", "'italic' must be a boolean", register_font( "Sans", regular, false, 1 ) )

check( unregister_font( "Sans", true ) == true, "unregister_font( Sans bold ) failed" )
expect_error( "unregister slot twice", "not registered", unregister_font( "Sans", true ) )
check( unregister_font( regular ) == true, "unregister_font( regular ) failed (it fills Sans and Sans italic)" )
expect_error( "unregister font twice", "not registered", unregister_font( regular ) )
expect_error( "unregister slot gone", "not registered", unregister_font( "Sans", false, true ) )
expect_error( "unregister bad", "unregister_font expects a font or a family", unregister_font( __BAD ) )
expect_error( "unregister bold string", "'bold' must be a boolean", unregister_font( "Sans", "yes" ) )

-- release_font: drops only the handle - a font still active or registered keeps working
check( register_font( "Mono", ttf ) == true, "register_font( Mono, ttf ) failed" )
check( set_font( ttf ) == true, "set_font( ttf ) failed" )
local widthBefore = measure_text( "Hello", __SIZE )
check( release_font( ttf ) == true, "release_font( ttf ) failed" )
expect_error( "release twice", "invalid font handle", release_font( ttf ) )
expect_error( "released handle in draw_text", "invalid font handle", draw_text( "x", 0, 0, __SIZE, 255, 255, 255, 255, ttf ) )
expect_error( "released handle in set_font", "invalid font handle", set_font( ttf ) )
check( measure_text( "Hello", __SIZE ) == widthBefore, "the active font changed when its handle was released" )
check( unregister_font( "Mono" ) == true, "a released font must still be registered (unregister by family)" )

-- a released slot is reused, but the old handle never resolves to the new font
local reused = load_font( __TTF )
check( math.type( reused ) == "integer" and reused ~= ttf, "a reused slot gave back the released handle " .. tostring( reused ) )
check( measure_text( "Hello", __SIZE, reused ) > 0, "the new handle in a reused slot does not work" )
expect_error( "stale handle after reuse", "invalid font handle", measure_text( "Hello", __SIZE, ttf ) )
expect_error( "release bad", "invalid font handle", release_font( __BAD ) )
expect_error( "release string", "invalid font handle", release_font( "x" ) )

check( release_font( regular ) == true and release_font( bold ) == true and release_font( reused ) == true,
       "releasing the remaining handles failed" )

local frames = 0
function on_update( dt )
    frames = frames + 1
    if frames == 1 then
        -- drawing with a font handle inside a real frame
        local font = load_font( __REGULAR )
        check( font ~= nil and draw_text( "frame", 10, 10, __SIZE, 255, 255, 255, 255, font ) == true,
               "draw_text with a font failed inside a frame" )
        release_font( font )

        if #failures == 0 then
            print( "FONT API SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "FONT API SMOKE FAIL: " .. f ) end
        end
    end
end
