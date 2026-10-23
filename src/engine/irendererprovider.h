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

#include <optional>
#include <string>
#include "renderer/rendererconfig.h"
#include "tilemap/itilemap.h"
#include "drawsurface/idrawsurface.h"

namespace Scarab  {
    namespace Engine  {

        /**
         * @brief What renderer_create asks for beyond sunlight's RendererConfig itself:
         * things a provider applies once the renderer exists, not passive configuration
         * values. Plain values only, so IRendererProvider stays Lua-agnostic and names no
         * sunlight input type.
         */
        struct stRendererCreateExtras  {
            /*
             * Install sunlight's own default camera/zoom key and gamepad bindings (arrow
             * keys, PageUp/Down, Home/End, and the matching gamepad buttons) -
             * renderer_create's use_default_key_handler. Out of RendererConfig because
             * sunlight removed its own equivalent field (doc/MASTER_PLAN.md item D1):
             * installing a preset is an action, not a setting.
             */
            bool                         bUseDefaultKeyHandler = false;

            /*
             * The default view's preferred zoom POSITION - what zoom_reset returns to,
             * renderer_create's preferred_zoom (doc/MASTER_PLAN.md item D2). Empty (left
             * out) keeps sunlight's own default. RendererConfig has a current zoom
             * (nZoomPos) but no preferred one.
             */
            std :: optional<unsigned>    nPreferredZoomPos;
        };

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

            /** @brief How the current renderer came to exist. */
            enum Origin  {
                ORIGIN_NONE = 0,       // no renderer yet
                ORIGIN_CREATED,        // an explicit CreateRenderer (renderer_create)
                ORIGIN_LAZY_DEFAULT,   // implicitly, by the first call that needed a window
                ORIGIN_LAST
            };

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

            /**
             * @brief Explicitly create the process's renderer (renderer_create) and open its
             * window. Fails - with the reason in *pError - if one already exists (naming the
             * primitive that created the default one implicitly, when that's what happened),
             * or if the configuration can't work.
             * @param extras What renderer_create asks for beyond RendererConfig itself, applied
             * once the renderer exists - see stRendererCreateExtras;
             */
            virtual bool CreateRenderer( const SunLight :: Renderer :: RendererConfig &config,
                                         const stRendererCreateExtras &extras, std :: string *pError ) = 0;

            /** @brief How the current renderer came to exist (ORIGIN_NONE if it doesn't yet). */
            virtual Origin GetOrigin( void ) = 0;

            /** @brief Whether the process runs --headless (the null backend is forced). */
            virtual bool IsHeadless( void ) = 0;

            /** @brief The configuration an implicitly created renderer gets (Scarab's defaults). */
            virtual SunLight :: Renderer :: RendererConfig GetDefaultConfig( void ) = 0;

            /**
             * @brief The configuration the current renderer was actually created with (after
             * --headless was applied), with the title kept current across app_set_name.
             */
            virtual SunLight :: Renderer :: RendererConfig GetEffectiveConfig( void ) = 0;

            /** @brief Record a window-title change, so the effective configuration stays live. */
            virtual void NoteWindowTitle( const std :: string &strTitle ) = 0;

            /**
             * @brief What CreateRenderer's extras.bUseDefaultKeyHandler was last given (false if
             * no renderer exists yet, or it was created implicitly - the lazy default never
             * requests this). renderer_get_config's own use_default_key_handler field reads
             * this back, the same "a creation-time value, not changed later" status its other
             * creation-only fields already have (view_control_mode, frame_pacing, max_frames).
             */
            virtual bool GetUseDefaultKeyHandler( void ) = 0;
        };
    }
}

#endif  /* __IRENDERERPROVIDER_H__ */
