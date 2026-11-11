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

extern "C"  {
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
             * an object, and never `0`; when the pool for that type is
             * full it returns `nil, "message"` - check for it. A handle
             * from a released slot doesn't silently alias a new sprite
             * that reuses the same slot — every operation on a stale
             * handle fails with `nil, "invalid sprite handle"` instead of
             * touching the wrong sprite, since each slot's internal
             * generation counter is bumped on release.
             *
             * ## The full lifecycle
             *
             * ```lua
             * -- 1. Register a pool for this sprite type, once, with a fixed capacity
             * pool_register_type("enemy_satellite", 50)
             *
             * -- 2. Acquire a handle
             * local handle = sprite_acquire("enemy_satellite")
             * if not handle then
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
             * -- 5. Release it: it is taken off every layer it is on, so it stops
             * --    drawing and colliding, and the slot is free for another acquire
             * sprite_release(handle)
             * ```
             *
             * ## Error reporting
             *
             * Every sprite function reports a failure as `nil, "message"` and
             * never raises a Lua error: `local ok, err = sprite_set_pos( h, x, y )`.
             * A function with nothing else to return returns `true` on success.
             * Arguments are checked strictly: `"invalid sprite handle"` (never
             * acquired, or already released), `"argument 'x' must be an integer"`
             * (a string, a missing argument, or a fractional number like `3.5` —
             * round it with `math.floor` first; `3.0` is fine), `"argument 'visible'
             * must be a boolean"` (not `nil`, not `0`/`1`).
             */
            class LuaSpriteApi  {

                static int RegisterType( lua_State* pLuaState );
                static int Acquire( lua_State* pLuaState );
                static int Release( lua_State* pLuaState );
                static int ConfigureTexture( lua_State* pLuaState );
                static int SetActiveSequence( lua_State* pLuaState );
                static int GetActiveSequence( lua_State* pLuaState );
                static int SetAnimationMode( lua_State* pLuaState );
                static int SetVisible( lua_State* pLuaState );
                static int GetVisible( lua_State* pLuaState );
                static int GetPos( lua_State* pLuaState );
                static int SetWorldSpace( lua_State* pLuaState );
                static int GetWorldSpace( lua_State* pLuaState );
                static int SetBlendMode( lua_State* pLuaState );
                static int ClearBlendMode( lua_State* pLuaState );
                static int GetBlendMode( lua_State* pLuaState );
                static int SetOpacity( lua_State* pLuaState );
                static int GetOpacity( lua_State* pLuaState );
                static int SetTint( lua_State* pLuaState );
                static int GetTint( lua_State* pLuaState );
                static int SetZOrder( lua_State* pLuaState );
                static int GetZOrder( lua_State* pLuaState );
                static int SetPos( lua_State* pLuaState );
                static int GetSize( lua_State* pLuaState );
                static int SetCollisionInset( lua_State* pLuaState );
                static int AddToLayer( lua_State* pLuaState );
                static int RemoveFromLayer( lua_State* pLuaState );

                static void RegisterEnums( lua_State* pLuaState );

                public:

                // The key LuaEngine::Init keeps the engine's SpritePool under (LuaEngineUtil::GetEnginePointer: out of every script's reach).
                static constexpr const char*  SPRITE_POOL_KEY = "spritePoolPtr";

                static void Register( lua_State* pLuaState );
            };
        }
    }
}

#endif /* __LUASPRITEAPI_H__ */
