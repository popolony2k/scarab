/*
 * luaengine.h
 *
 *  Created on: Dec 13, 2023
 *      Author: popolony2k
 */

#ifndef __LUAENGINE_H__
#define __LUAENGINE_H__

extern "C"
{
  #include "lua.h"
  #include "lualib.h"
  #include "lauxlib.h"
}

#include "scripting/scriptprocessor.h"
#include "lua/luaengineutil.h"
#include <string>
#include <cstdint>

namespace SunLight  {
    namespace TileMap  { class ITileMap; }
    namespace Sound  { class SoundManager; }
}

namespace Scarab  {
    namespace Engine  {
        class SpritePool;

        namespace Lua  { class LuaCollisionListener; }
    }
}

namespace Scarab  {
    namespace Lua {
        /**
         * @brief Implements a C++ abstraction for using
         * Lua engine;
         */
        class LuaEngine  {

            lua_State                                 *m_pLuaState;
            SunLight :: Scripting :: ScriptProcessor  *m_pScriptProcessor;
            Engine :: Lua :: TimerMap                  m_Timers;
            Engine :: Lua :: LuaCollisionListener     *m_pCollisionListener;
            std :: string                              m_strAppDirectory;

            void RegisterCalls( void );

            public:

            LuaEngine( SunLight :: Scripting :: ScriptProcessor *pScriptProcessor );
            virtual ~LuaEngine( void );

            bool RunFile( std :: string strFileName );
            void CallOnUpdate( uint32_t nDeltaMilli );
            bool TryDispatchMoveSpritesToScreen( uint16_t nStateId );
            bool TryDispatchLoadStage( uint16_t nStageId );
            int GetActiveEnemyCount( void );
            const std :: string& GetApplicationDirectory( void ) const;
            void Init( SunLight :: TileMap :: ITileMap *pTileMap,
                      SunLight :: Sound :: SoundManager *pSoundManager,
                      Engine :: SpritePool *pSpritePool );
        };
    }
}
#endif  /* __LUAENGINE_H__ */
