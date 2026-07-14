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
