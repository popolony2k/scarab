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
         *
         * @luacategory{Engine callbacks — hooks the engine calls *into*}
         * @luadoc
         * Every other page in this reference documents functions your
         * Lua script calls *into* the engine. This page is the reverse
         * direction: global Lua functions the engine looks up and calls
         * *from* C++, if you define them. Every one of these is
         * optional — the engine checks whether the global exists and is
         * a function before calling it, and silently no-ops if it isn't
         * defined (except `on_load_stage`, see below).
         *
         * Define these as plain global functions (not table members) —
         * the engine looks them up by exact name via `lua_getglobal`.
         * @luaoutro
         * ## The collision handlers are documented separately
         *
         * `collision_set_handler`/`collision_set_tile_handler` follow
         * the same "engine calls into a Lua global" shape as everything
         * on this page, but since they're registered via an explicit
         * function call rather than just defining a magic global name,
         * they're documented alongside the rest of the collision API in
         * [collision.md](collision.md).
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
                      SunLight :: DrawSurface :: IDrawSurface *pDrawSurface,
                      SunLight :: Sound :: SoundManager *pSoundManager,
                      Engine :: SpritePool *pSpritePool );
        };
    }
}
#endif  /* __LUAENGINE_H__ */
