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

#ifndef __LUASPRITEAPI_H__
#define __LUASPRITEAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Sprite handle pool + rendering Lua primitives.
             *
             * @luacategory{Sprites}
             * @luadoc
             * Backed by `Scarab::Engine::SpritePool` — a fixed-capacity
             * pool of sprite slots, not a dynamically-growing list. Every
             * sprite in the game (enemies, bullets, the player ship) is
             * acquired from this same pool.
             *
             * ## Handles
             *
             * `sprite_acquire` returns an opaque integer **handle**, not
             * an object. `0` always means "invalid"
             * (`INVALID_SPRITE_HANDLE`) — check for it after
             * `sprite_acquire` in case the pool for that type is
             * exhausted. A handle from a released slot doesn't silently
             * alias a new sprite that reuses the same slot — every
             * operation on a stale handle simply fails (returns
             * `false`/`nil`) instead of touching the wrong sprite, since
             * each slot's internal generation counter is bumped on
             * release.
             *
             * ## The full lifecycle
             *
             * ```lua
             * -- 1. Register a pool for this sprite type, once, with a fixed capacity
             * pool_register_type("enemy_satellite", 50)
             *
             * -- 2. Acquire a handle
             * local handle = sprite_acquire("enemy_satellite")
             * if handle == 0 then
             *   return  -- pool exhausted
             * end
             *
             * -- 3. Configure at least one texture sequence (sequence 0 is the convention
             * --    for "main" appearance; sequence 1 is commonly "explosion", but any
             * --    sequence id is valid - it's just an index you choose)
             * sprite_configure_texture(handle, 0, BASE_PATH .. "sprites/satellite-ship.png",
             *                           3, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 150)
             * sprite_set_active_sequence(handle, 0)
             *
             * -- 4. Position it and make it visible on a Tiled layer
             * sprite_set_pos(handle, 100, 0)
             * sprite_add_to_layer(handle, LAYER_ENEMIES_SHIPS)
             *
             * -- ... later, on death/despawn ...
             *
             * -- 5. Remove from the layer BEFORE releasing - the pool has no idea which
             * --    layer(s) a handle was added to, so skipping this step leaves a
             * --    dangling collider that keeps firing collision callbacks against a
             * --    handle that no longer resolves to anything.
             * sprite_remove_from_layer(handle, LAYER_ENEMIES_SHIPS)
             * sprite_release(handle)
             * ```
             */
            class LuaSpriteApi  {

                static int RegisterType( lua_State *pLuaState );
                static int Acquire( lua_State *pLuaState );
                static int Release( lua_State *pLuaState );
                static int ConfigureTexture( lua_State *pLuaState );
                static int SetActiveSequence( lua_State *pLuaState );
                static int GetActiveSequence( lua_State *pLuaState );
                static int SetAnimationMode( lua_State *pLuaState );
                static int SetVisible( lua_State *pLuaState );
                static int GetVisible( lua_State *pLuaState );
                static int GetPos( lua_State *pLuaState );
                static int SetPos( lua_State *pLuaState );
                static int GetSize( lua_State *pLuaState );
                static int SetCollisionInset( lua_State *pLuaState );
                static int AddToLayer( lua_State *pLuaState );
                static int RemoveFromLayer( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUASPRITEAPI_H__ */
