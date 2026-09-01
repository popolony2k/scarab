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
 scripting - ScriptProcessor's queued sp_* commands
 (docs/lua-api/scripting.html).

 A forever-repeating "wave" cycle built entirely from queued commands:
 sp_add_label/sp_goto_label form the queue's only looping construct,
 sp_wait paces it, sp_move_sprites_to_screen dispatches a wave-spawn
 event to the optional on_move_sprites_to_screen(stateId) hook (see
 docs/lua-api/callbacks.html), and sp_wait_queue_empty demonstrates the
 "don't spawn the next wave until this one's gone" gate - this sample
 has no sprites/enemies at all, so get_active_enemy_count is never
 defined, which means EngineHost::CheckSpritesQueueEmpty treats the
 screen as already clear every ~2s check (see it's own doc comment) -
 sp_wait_queue_empty resolves on it's own shortly after being reached,
 with nothing else needed to unblock it.

 sp_load_stage is deliberately NOT demonstrated here - it requires a
 real on_load_stage(stageId) hook that actually loads a map/script and
 returns true, or the engine logs a fatal-looking error every time the
 queue reaches it (EngineHost::OnCommand's LOAD_STAGE_CMD case) - out
 of scope for a small, focused sample.
]]

app_set_name( "Scarab - scripting sample" )

local waveCount = 0

-- Called when the queue reaches sp_move_sprites_to_screen(1) below.
-- Must return true to "claim" the state id - see docs/lua-api/callbacks.html.
function on_move_sprites_to_screen( stateId )
    waveCount = waveCount + 1

    return true
end

-- The queue itself: label 1, dispatch a wave, pace with sp_wait, gate on
-- "no active enemies" (immediate here, since this sample has none), then
-- loop back to label 1 forever.
sp_add_label( 1 )
sp_move_sprites_to_screen( 1 )
sp_wait( 2000 )
sp_wait_queue_empty()
sp_wait( 500 )
sp_goto_label( 1 )

function on_update( dt )
    draw_text( "scripting sample", 20, 20, 24, 255, 255, 255, 255 )
    draw_text( "on_move_sprites_to_screen fired: " .. waveCount .. " time(s)", 20, 54, 20, 200, 200, 200, 255 )
    draw_text( "(queued sp_wait(2000) -> sp_wait_queue_empty() -> sp_wait(500) -> loop)", 20, 78, 18, 150, 150, 150, 255 )
end
