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

#ifndef __RENDERERPROVIDER_H__
#define __RENDERERPROVIDER_H__

#include <functional>
#include <memory>
#include <string>
#include "engine/irendererprovider.h"
#include "renderer/rendererconfig.h"
#include "renderer/tilemaprenderer.h"

namespace Scarab  {
    namespace Host  {

        /**
         * @brief What the command line asked of --headless; the provider
         * applies it to every renderer it creates, however that renderer
         * came about (default or renderer_create).
         */
        struct stHeadlessSettings  {
            bool      bEnabled       = false;   // --headless: force the null backend
            bool      bFast          = false;   // --fast: unlimited frame pacing
            unsigned  nMaxFrames     = 0;       // --max-frames N (0 = none given)
        };

        /**
         * @brief Owns the process's one renderer and creates it on demand.
         *
         * The renderer is created either explicitly (Create, from
         * renderer_create) or implicitly with the default configuration the
         * first time a primitive needs a window (GetTileMap/GetDrawSurface)
         * or when the entry script finishes without needing one
         * (EnsureCreated). Either way it is Start()ed immediately, so its
         * window is open by the time Create returns - load-time code like
         * set_font keeps working.
         */
        class RendererProvider : public Scarab :: Engine :: IRendererProvider  {

            public:

            /** @brief How the current renderer came to exist. */
            enum Origin  {
                ORIGIN_NONE = 0,       // no renderer yet
                ORIGIN_CREATED,        // an explicit Create()
                ORIGIN_LAZY_DEFAULT,   // implicitly, by the first call that needed a window
                ORIGIN_LAST
            };

            typedef std :: function<void( SunLight :: Renderer :: TileMapRenderer& )>  CreatedHook;

            private:

            SunLight :: Renderer :: RendererConfig                        m_DefaultConfig;
            SunLight :: Renderer :: RendererConfig                        m_EffectiveConfig;
            stHeadlessSettings                                            m_Headless;
            std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>   m_pRenderer;
            Origin                                                        m_Origin;
            std :: string                                                 m_strTrigger;
            CreatedHook                                                   m_CreatedHook;

            SunLight :: Renderer :: TileMapRenderer* GetOrCreateDefault( const char *szTrigger );

            public:

            RendererProvider( const SunLight :: Renderer :: RendererConfig &defaults,
                              const stHeadlessSettings &headless );
            virtual ~RendererProvider( void );

            void SetCreatedHook( CreatedHook hook );
            bool Create( const SunLight :: Renderer :: RendererConfig &config,
                         Origin origin,
                         const std :: string &strTrigger,
                         std :: string *pError );
            void EnsureCreated( const char *szTrigger );

            SunLight :: Renderer :: TileMapRenderer* GetRenderer( void );
            Origin GetOrigin( void ) const;
            const std :: string& GetTrigger( void ) const;
            const SunLight :: Renderer :: RendererConfig& GetEffectiveConfig( void ) const;
            const stHeadlessSettings& GetHeadlessSettings( void ) const;

            // Scarab::Engine::IRendererProvider
            SunLight :: TileMap :: ITileMap* GetTileMap( const char *szTrigger );
            SunLight :: DrawSurface :: IDrawSurface* GetDrawSurface( const char *szTrigger );
            SunLight :: DrawSurface :: IDrawSurface* PeekDrawSurface( void );
        };
    }
}

#endif  /* __RENDERERPROVIDER_H__ */
