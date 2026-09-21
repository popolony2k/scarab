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

#ifndef __IRENDERERPROVIDER_H__
#define __IRENDERERPROVIDER_H__

namespace SunLight  {
    namespace TileMap  { class ITileMap; }
    namespace DrawSurface  { class IDrawSurface; }
}

namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Where the Lua bridge gets the renderer from.
         *
         * The renderer does not exist yet when the Lua engine is built, or
         * when a game's entry script starts running - the script may create
         * it itself (renderer_create), and if it never does, the default one
         * is created the first time something actually needs a window. So
         * every primitive reaches ITileMap/IDrawSurface through this
         * instead of holding a pointer taken at construction.
         *
         * Lua-agnostic on purpose (like SpritePool): no lua_State here.
         * Implemented by Scarab::Host::RendererProvider.
         */
        class IRendererProvider  {

            public:

            virtual ~IRendererProvider( void ) {}

            /**
             * @brief The tile map, creating the default renderer (and
             * opening its window) first if none exists yet.
             * @param szTrigger Name of the primitive that needs it, kept so
             * a later renderer_create can say which call created the
             * default renderer implicitly;
             */
            virtual SunLight :: TileMap :: ITileMap* GetTileMap( const char *szTrigger ) = 0;

            /**
             * @brief The draw surface, creating the default renderer first
             * if none exists yet - see @link GetTileMap.
             */
            virtual SunLight :: DrawSurface :: IDrawSurface* GetDrawSurface( const char *szTrigger ) = 0;

            /**
             * @brief The draw surface if a renderer already exists, else
             * nullptr - never creates one. For readers that must not open
             * a window as a side effect (eg. os.time's headless override).
             */
            virtual SunLight :: DrawSurface :: IDrawSurface* PeekDrawSurface( void ) = 0;
        };
    }
}

#endif  /* __IRENDERERPROVIDER_H__ */
