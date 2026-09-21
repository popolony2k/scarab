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

#include "host/rendererprovider.h"
#include <cstdio>
#include <cstdlib>

namespace Scarab  {
    namespace Host  {

        /**
         * @brief Constructor.
         *
         * @param defaults The configuration used when a renderer has to be
         * created implicitly (today's hardcoded values, see main.cpp);
         * @param headless What the command line asked of --headless;
         */
        RendererProvider :: RendererProvider( const SunLight :: Renderer :: RendererConfig &defaults,
                                              const stHeadlessSettings &headless )  {

            m_DefaultConfig   = defaults;
            m_EffectiveConfig = defaults;
            m_Headless        = headless;
            m_Origin          = ORIGIN_NONE;
        }

        /**
         * @brief Destructor. Destroys the renderer, if one was ever created
         * (its window is normally already closed by then - main() Stop()s it).
         */
        RendererProvider :: ~RendererProvider( void )  {
        }

        /**
         * @brief Set the function called with every renderer right after it
         * is created and before its window opens - where the engine host
         * registers itself as the tile-map listener and the Lua bridge
         * attaches its collision listener.
         */
        void RendererProvider :: SetCreatedHook( CreatedHook hook )  {

            m_CreatedHook = hook;
        }

        /**
         * @brief Create the process's renderer and open its window.
         *
         * Applies --headless on top of the requested configuration: the null
         * backend is forced (asking for another one is an error), and the
         * command line's --fast/--max-frames win over whatever the
         * configuration says. Only one renderer exists per process run.
         *
         * @param config The requested configuration;
         * @param origin How this creation came about;
         * @param strTrigger For an implicit creation, the primitive that needed a window;
         * @param pError Receives the reason on failure (may be null);
         * @return true if the renderer exists now, false (with *pError set) if not;
         */
        bool RendererProvider :: Create( const SunLight :: Renderer :: RendererConfig &config,
                                         Origin origin,
                                         const std :: string &strTrigger,
                                         std :: string *pError )  {

            if( m_pRenderer )  {
                if( pError )
                    *pError = "renderer already created";

                return false;
            }

            SunLight :: Renderer :: RendererConfig  effective = config;

            if( m_Headless.bEnabled )  {
                if( effective.backend == SunLight :: Renderer :: RENDERER_BACKEND_RAYLIB )  {
                    if( pError )
                        *pError = "--headless forces the null backend";

                    return false;
                }

                effective.backend = SunLight :: Renderer :: RENDERER_BACKEND_NULL;

                if( m_Headless.bFast )
                    effective.framePacing = SunLight :: Renderer :: FRAME_PACING_UNLIMITED;

                if( m_Headless.nMaxFrames > 0 )
                    effective.nMaxFrames = m_Headless.nMaxFrames;
            }

            std :: unique_ptr<SunLight :: Renderer :: TileMapRenderer>  pRenderer =
                SunLight :: Renderer :: TileMapRenderer :: Create( effective, pError );

            if( !pRenderer )
                return false;

            m_pRenderer       = std :: move( pRenderer );
            m_EffectiveConfig = effective;
            m_Origin          = origin;
            m_strTrigger      = strTrigger;

            if( m_CreatedHook )
                m_CreatedHook( *m_pRenderer );

            m_pRenderer -> Start();

            return true;
        }

        /**
         * @brief Create the default renderer if none exists yet - called when
         * the entry script has finished, so a game that never asked for a
         * window-needing primitive still gets its one renderer to run on.
         */
        void RendererProvider :: EnsureCreated( const char *szTrigger )  {

            GetOrCreateDefault( szTrigger );
        }

        /**
         * @brief The renderer, creating the default one first if none exists
         * yet. A failure to create even the default one is fatal: nothing can
         * run without a renderer, and callers dereference the result.
         */
        SunLight :: Renderer :: TileMapRenderer* RendererProvider :: GetOrCreateDefault( const char *szTrigger )  {

            if( !m_pRenderer )  {
                std :: string  strError;

                if( !Create( m_DefaultConfig, ORIGIN_LAZY_DEFAULT, szTrigger ? szTrigger : "", &strError ) )  {
                    fprintf( stderr, "[FATAL] - cannot create the default renderer: %s\n", strError.c_str() );
                    exit( EXIT_FAILURE );
                }
            }

            return m_pRenderer.get();
        }

        /** @brief The renderer, or nullptr if none exists yet (never creates one). */
        SunLight :: Renderer :: TileMapRenderer* RendererProvider :: GetRenderer( void )  {

            return m_pRenderer.get();
        }

        /** @brief What the command line asked of --headless. */
        const stHeadlessSettings& RendererProvider :: GetHeadlessSettings( void ) const  {

            return m_Headless;
        }

        SunLight :: TileMap :: ITileMap* RendererProvider :: GetTileMap( const char *szTrigger )  {

            return GetOrCreateDefault( szTrigger );
        }

        SunLight :: DrawSurface :: IDrawSurface* RendererProvider :: GetDrawSurface( const char *szTrigger )  {

            return GetOrCreateDefault( szTrigger );
        }

        SunLight :: DrawSurface :: IDrawSurface* RendererProvider :: PeekDrawSurface( void )  {

            return m_pRenderer.get();
        }

        /**
         * @brief Explicitly create the renderer (renderer_create). The error for "a renderer
         * already exists" says which of the two ways it got there, and - when it was the
         * default, created implicitly - names the primitive that needed a window first, since
         * that call, not this one, is what to move below renderer_create.
         */
        bool RendererProvider :: CreateRenderer( const SunLight :: Renderer :: RendererConfig &config, std :: string *pError )  {

            if( m_pRenderer )  {
                if( pError )  {
                    if( m_Origin == ORIGIN_LAZY_DEFAULT )
                        *pError = "renderer already created: the default renderer was created implicitly by the first "
                                  "window-needing call, '" + m_strTrigger + "' - renderer_create must be the first "
                                  "window-needing call in the entry script";
                    else
                        *pError = "renderer already created - only one renderer per process is supported";
                }

                return false;
            }

            return Create( config, ORIGIN_CREATED, "", pError );
        }

        RendererProvider :: Origin RendererProvider :: GetOrigin( void )  {

            return m_Origin;
        }

        bool RendererProvider :: IsHeadless( void )  {

            return m_Headless.bEnabled;
        }

        SunLight :: Renderer :: RendererConfig RendererProvider :: GetDefaultConfig( void )  {

            return m_DefaultConfig;
        }

        SunLight :: Renderer :: RendererConfig RendererProvider :: GetEffectiveConfig( void )  {

            return m_EffectiveConfig;
        }

        /**
         * @brief Keep the effective configuration's title current: app_set_name changes the
         * live window title, and sunlight has no getter for it to read back.
         */
        void RendererProvider :: NoteWindowTitle( const std :: string &strTitle )  {

            m_EffectiveConfig.strTitle = strTitle;
        }
    }
}
