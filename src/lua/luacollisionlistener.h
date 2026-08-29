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

#ifndef __LUACOLLISIONLISTENER_H__
#define __LUACOLLISIONLISTENER_H__

#include "collision/icollisionlistener.h"

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Collision listener that decodes packed SpriteHandles from
             * collider data and dispatches to a registered Lua handler. Only
             * processes collisions where BOTH sides are Lua-owned - anything
             * with at least one legacy (non-Lua) side is the legacy collision
             * listener's job instead (it's the one that understands the legacy
             * sprite type well enough to decide response for a mixed collision
             * without a listener-ordering race).
             */
            class LuaCollisionListener : public SunLight :: Collision :: ICollisionListener  {

                lua_State  *m_pLuaState;

                public:

                LuaCollisionListener( lua_State *pLuaState );
                virtual ~LuaCollisionListener( void );

                void OnCollision( SunLight :: Collision :: Collider *pFirst,
                                  SunLight :: Collision :: Collider *pSecond );
                void OnCollision( SunLight :: Collision :: Collider *pFirst,
                                  SunLight :: TileMap :: stTile *pSecond );
            };
        }
    }
}

#endif  /* __LUACOLLISIONLISTENER_H__ */
