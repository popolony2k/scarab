/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "lua/luascriptingapi.h"
#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Implement the script processing wait routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_wait(milliseconds)}
             * @luadoc
             * Pause queue processing for `milliseconds` before the next
             * queued command runs. Purely queue-side — rendering,
             * physics, and every other per-frame system keep running.
             * @luaexample
             * sp_wait(2000)  -- 2 second pause, non-blocking
             */
            int LuaScriptingApi :: Wait( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: WAIT_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing clear routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_clear()}
             * @luadoc
             * Empty the entire command queue immediately, discarding
             * everything not yet processed. Used once at startup
             * (`main.lua`) before the first `sp_load_stage`, to
             * guarantee a clean queue regardless of what earlier
             * `dofile`s may have queued.
             * @luaexample
             * sp_clear()
             */
            int LuaScriptingApi :: Clear( lua_State *pLuaState )  {

                LuaEngineUtil :: GetScriptProcessor( pLuaState ) -> Clear();

                return 0;
            }

            /**
             * @brief Implement the script processing Wait for queue empty routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_wait_queue_empty()}
             * @luadoc
             * Queue a command that blocks all *later* queued commands
             * until the engine reports the screen is clear of active
             * enemies (`EngineHost::CheckSpritesQueueEmpty`, driven by
             * the game's own `get_active_enemy_count()` — see
             * [callbacks.md](callbacks.md)). The classic "don't spawn wave 2 until wave
             * 1 is dead" gate.
             * @luaexample
             * sp_move_sprites_to_screen(STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM)
             * sp_wait_queue_empty()
             * sp_move_sprites_to_screen(STATE_MOVE_CYLINDER_SHIP_TO_SCREEN_LEFT_SIDE)  -- only starts once wave 1 is gone
             */
            int LuaScriptingApi :: WaitQueueEmpty( lua_State *pLuaState )  {

                LuaEngineUtil :: GetScriptProcessor( pLuaState ) -> AddNoParmCmd( SunLight :: Scripting :: Commands :: WAIT_SPRITES_QUEUE_EMPTY );

                return 0;
            }

            /**
             * @brief Implement the script processing Move sprites to screen routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_move_sprites_to_screen(stateId)}
             * @luadoc
             * Queue a wave-spawn command. `stateId` is an opaque
             * integer — Caravellius defines its own meaningful names for
             * these in `caravellius/src/wavestates.lua`
             * (`STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM`, etc.); the
             * engine itself doesn't know or care what any specific id
             * means; it's forwarded verbatim to the game's own
             * `on_move_sprites_to_screen(stateId)` hook (see
             * [callbacks.md](callbacks.md)).
             * @luaexample
             * sp_move_sprites_to_screen(STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM)
             */
            int LuaScriptingApi :: MoveSpritesToScreen( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: MOVE_SPRITES_TO_SCREEN_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing Add label routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_add_label(id)}
             * @luagroup{labels}
             * @luadoc
             * Mark a position in the queue (`sp_add_label`) and later
             * jump the queue's read position back (or forward) to it
             * (`sp_goto_label`) — the queue's only looping construct,
             * since it's a linear command sequence otherwise. `id` is
             * any integer you choose; it just has to match between the
             * label and the goto.
             * @luaexample
             * sp_add_label(1)
             *
             * sp_move_sprites_to_screen(STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM)
             * sp_wait_queue_empty()
             *
             * sp_goto_label(1)  -- repeats the wave forever
             */
            int LuaScriptingApi :: AddLabel( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: LABEL_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing Goto label routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_goto_label(id)}
             * @luagroup{labels}
             */
            int LuaScriptingApi :: GotoLabel( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: GOTO_LABEL_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing Load stage routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_load_stage(stageId)}
             * @luadoc
             * Queue a stage load. `stageId` is opaque to the engine the
             * same way wave-spawn ids are — Caravellius defines
             * `STAGE_FIRST`/`STAGE_LAST` in
             * `caravellius/src/stageids.lua`. When the queue reaches
             * this command, the engine calls the game's
             * `on_load_stage(stageId)` hook (see [callbacks.md](callbacks.md)), which is
             * expected to load a map and `dofile` a stage script. There
             * is no built-in stage concept beyond this — the engine only
             * ferries the id through.
             * @luaexample
             * sp_load_stage(STAGE_FIRST)
             */
            int LuaScriptingApi :: LoadStage( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: LOAD_STAGE_CMD );

                return 0;
            }

            /**
             * @brief Register the ScriptProcessor queue-control Lua-callable functions.
             */
            void LuaScriptingApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "sp_wait", LuaScriptingApi :: Wait );
                lua_register( pLuaState, "sp_clear", LuaScriptingApi :: Clear );
                lua_register( pLuaState, "sp_wait_queue_empty", LuaScriptingApi :: WaitQueueEmpty );
                lua_register( pLuaState, "sp_move_sprites_to_screen", LuaScriptingApi :: MoveSpritesToScreen );
                lua_register( pLuaState, "sp_add_label", LuaScriptingApi :: AddLabel );
                lua_register( pLuaState, "sp_goto_label", LuaScriptingApi :: GotoLabel );
                lua_register( pLuaState, "sp_load_stage", LuaScriptingApi :: LoadStage );
            }
        }
    }
}
