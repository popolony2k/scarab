/*
 * luacollisionlistener.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
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
