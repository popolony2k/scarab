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

/*
 * luascriptingapi.cpp
 *
 *  Created on: Jul 11, 2026
 *      Author: popolony2k
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
