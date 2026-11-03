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
#include "input/defaultcontrollerbindings.h"
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
        RendererProvider :: RendererProvider( const SunLight :: Renderer :: RendererConfig& defaults,
                                              const stHeadlessSettings& headless )  {

            m_DefaultConfig          = defaults;
            m_EffectiveConfig        = defaults;
            m_bUseDefaultKeyHandler  = false;
            m_Headless               = headless;
            m_Origin                 = ORIGIN_NONE;
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
         * attaches its collision listener and its gamepad hot-plug handler.
         *
         * @param hook The function to call with each new renderer;
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
         * @param extras What renderer_create asks for beyond the configuration, applied once
         * the renderer exists and is Start()ed (stRendererCreateExtras): sunlight's default
         * camera/zoom bindings, and the default view's preferred zoom. Defaults to none of
         * them - what the implicitly-created default renderer always gets, since
         * GetOrCreateDefault never passes this;
         * @return true if the renderer exists now, false (with *pError set) if not;
         */
        bool RendererProvider :: Create( const SunLight :: Renderer :: RendererConfig& config,
                                         Origin origin,
                                         const std :: string& strTrigger,
                                         std :: string* pError,
                                         const Scarab :: Engine :: stRendererCreateExtras& extras )  {

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

            m_pRenderer             = std :: move( pRenderer );
            m_EffectiveConfig       = effective;
            m_bUseDefaultKeyHandler = extras.bUseDefaultKeyHandler;
            m_Origin                = origin;
            m_strTrigger            = strTrigger;

            if( m_CreatedHook )
                m_CreatedHook( *m_pRenderer );

            m_pRenderer -> Start();

            /*
             * Right after Start(), as planned (plans/MASTER_PLAN.md item D1): the renderer and
             * its controller manager both exist by now, and no map is loaded yet at this point
             * in the entry script's own flow (renderer_create must be the first window-needing
             * call, so this always runs before tilemap_load_map) - Install only registers
             * handlers that call ITileMap's camera/zoom methods, it never touches the map
             * itself, so running before one is loaded is safe.
             */
            if( extras.bUseDefaultKeyHandler )
                SunLight :: Input :: DefaultControllerBindings :: Install( *m_pRenderer, m_pRenderer -> GetControllerManager() );

            // Already validated by renderer_create (a valid position, and a fresh renderer's
            // zoom limits are the whole scale), so sunlight's own silent refusal can't happen.
            if( extras.nPreferredZoomPos )
                m_pRenderer -> GetViewport().SetPreferredZoom( *extras.nPreferredZoomPos );

            return true;
        }

        /**
         * @brief Create the default renderer if none exists yet - called when
         * the entry script has finished, so a game that never asked for a
         * window-needing primitive still gets its one renderer to run on.
         *
         * @param szTrigger What needed the renderer, kept for a later renderer_create's error;
         */
        void RendererProvider :: EnsureCreated( const char* szTrigger )  {

            GetOrCreateDefault( szTrigger );
        }

        /**
         * @brief The renderer, creating the default one first if none exists
         * yet. A failure to create even the default one is fatal: nothing can
         * run without a renderer, and callers dereference the result.
         *
         * @param szTrigger The primitive that needs a window (may be null), kept so a later
         * renderer_create can name it;
         * @return The renderer, never null;
         */
        SunLight :: Renderer :: TileMapRenderer* RendererProvider :: GetOrCreateDefault( const char* szTrigger )  {

            if( !m_pRenderer )  {
                std :: string  strError;

                if( !Create( m_DefaultConfig, ORIGIN_LAZY_DEFAULT, szTrigger ? szTrigger : "", &strError ) )  {
                    fprintf( stderr, "[FATAL] - cannot create the default renderer: %s\n", strError.c_str() );
                    exit( EXIT_FAILURE );
                }
            }

            return m_pRenderer.get();
        }

        /**
         * @brief The renderer, if one exists - never creates one.
         *
         * @return The renderer, or nullptr if none exists yet;
         */
        SunLight :: Renderer :: TileMapRenderer* RendererProvider :: GetRenderer( void )  {

            return m_pRenderer.get();
        }

        /**
         * @brief What the command line asked of --headless.
         *
         * @return The --headless, --fast and --max-frames settings;
         */
        const stHeadlessSettings& RendererProvider :: GetHeadlessSettings( void ) const  {

            return m_Headless;
        }

        /**
         * @brief The renderer as a tile map, creating the default renderer (and opening its
         * window) first if none exists yet.
         *
         * @param szTrigger The primitive that needs it, kept for a later renderer_create's error;
         * @return The tile map, never null;
         */
        SunLight :: TileMap :: ITileMap* RendererProvider :: GetTileMap( const char* szTrigger )  {

            return GetOrCreateDefault( szTrigger );
        }

        /**
         * @brief The renderer as a draw surface (screen-space text, rectangles, window state),
         * creating the default renderer first if none exists yet.
         *
         * @param szTrigger The primitive that needs it, kept for a later renderer_create's error;
         * @return The draw surface, never null;
         */
        SunLight :: DrawSurface :: IDrawSurface* RendererProvider :: GetDrawSurface( const char* szTrigger )  {

            return GetOrCreateDefault( szTrigger );
        }

        /**
         * @brief The renderer's controller manager (connected pads, hot-plug, dead zones) - on
         * the concrete renderer only, not on ITileMap - creating the default renderer first if
         * none exists yet.
         *
         * @param szTrigger The primitive that needs it, kept for a later renderer_create's error;
         * @return The controller manager, never null;
         */
        SunLight :: Input :: IControllerManager* RendererProvider :: GetControllerManager( const char* szTrigger )  {

            return &GetOrCreateDefault( szTrigger ) -> GetControllerManager();
        }

        /**
         * @brief The draw surface if a renderer exists - never creates one, for readers that must
         * not open a window as a side effect (the headless os.time/os.clock overrides).
         *
         * @return The draw surface, or nullptr if no renderer exists yet;
         */
        SunLight :: DrawSurface :: IDrawSurface* RendererProvider :: PeekDrawSurface( void )  {

            return m_pRenderer.get();
        }

        /**
         * @brief Explicitly create the renderer (renderer_create). The error for "a renderer
         * already exists" says which of the two ways it got there, and - when it was the
         * default, created implicitly - names the primitive that needed a window first, since
         * that call, not this one, is what to move below renderer_create.
         *
         * @param config The requested configuration;
         * @param extras What renderer_create asks for beyond the configuration (see Create);
         * @param pError Receives the reason on failure (may be null);
         * @return true if the renderer was created;
         */
        bool RendererProvider :: CreateRenderer( const SunLight :: Renderer :: RendererConfig& config,
                                                 const Scarab :: Engine :: stRendererCreateExtras& extras, std :: string* pError )  {

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

            return Create( config, ORIGIN_CREATED, "", pError, extras );
        }

        /**
         * @brief How the current renderer came to exist - renderer_get_current's answer.
         *
         * @return ORIGIN_NONE (no renderer yet), ORIGIN_CREATED (renderer_create) or
         * ORIGIN_LAZY_DEFAULT (the first window-needing call);
         */
        RendererProvider :: Origin RendererProvider :: GetOrigin( void )  {

            return m_Origin;
        }

        /**
         * @brief Whether the process runs --headless, which forces the null backend.
         *
         * @return true under --headless;
         */
        bool RendererProvider :: IsHeadless( void )  {

            return m_Headless.bEnabled;
        }

        /**
         * @brief The configuration a renderer gets when none is asked for (main.h's defaults),
         * the base renderer_create's options are applied to.
         *
         * @return The default configuration;
         */
        SunLight :: Renderer :: RendererConfig RendererProvider :: GetDefaultConfig( void )  {

            return m_DefaultConfig;
        }

        /**
         * @brief The configuration the current renderer was created with, --headless applied, and
         * the title kept current (NoteWindowTitle) - what renderer_get_config reads where
         * sunlight has no live getter.
         *
         * @return The effective configuration (the default one before any renderer exists);
         */
        SunLight :: Renderer :: RendererConfig RendererProvider :: GetEffectiveConfig( void )  {

            return m_EffectiveConfig;
        }

        /**
         * @brief Keep the effective configuration's title current: app_set_name changes the
         * live window title, and sunlight has no getter for it to read back.
         *
         * @param strTitle The title just set;
         */
        void RendererProvider :: NoteWindowTitle( const std :: string& strTitle )  {

            m_EffectiveConfig.strTitle = strTitle;
        }

        /**
         * @brief Whether the renderer got sunlight's default camera/zoom key bindings
         * (renderer_create's use_default_key_handler).
         *
         * @return What CreateRenderer's extras.bUseDefaultKeyHandler was last given;
         */
        bool RendererProvider :: GetUseDefaultKeyHandler( void )  {

            return m_bUseDefaultKeyHandler;
        }

        /**
         * @brief Register a font for a Tiled text-object family and style
         * (TileMapRenderer::RegisterFont, on the concrete renderer only), creating the default
         * renderer first if none exists yet.
         *
         * @param szFamily The font family a map's text objects name;
         * @param pFont The loaded font;
         * @param bBold The bold style slot;
         * @param bItalic The italic style slot;
         * @param szTrigger The primitive that needs it, kept for a later renderer_create's error;
         * @return true if the font was registered;
         */
        bool RendererProvider :: RegisterFont( const char* szFamily, std :: shared_ptr<SunLight :: Font :: IFont> pFont,
                                               bool bBold, bool bItalic, const char* szTrigger )  {

            return GetOrCreateDefault( szTrigger ) -> RegisterFont( szFamily, std :: move( pFont ), bBold, bItalic );
        }

        /**
         * @brief Unregister a font from every family and style slot it fills. Never creates a
         * renderer: with none, there is nothing registered.
         *
         * @param pFont The font to unregister;
         * @return true if it was registered anywhere;
         */
        bool RendererProvider :: UnregisterFont( const std :: shared_ptr<SunLight :: Font :: IFont>& pFont )  {

            return m_pRenderer ? m_pRenderer -> UnregisterFont( pFont ) : false;
        }

        /**
         * @brief Unregister whatever font fills one family and style slot. Never creates a
         * renderer: with none, there is nothing registered.
         *
         * @param szFamily The font family;
         * @param bBold The bold style slot;
         * @param bItalic The italic style slot;
         * @return true if the slot held a font;
         */
        bool RendererProvider :: UnregisterFont( const char* szFamily, bool bBold, bool bItalic )  {

            return m_pRenderer ? m_pRenderer -> UnregisterFont( szFamily, bBold, bItalic ) : false;
        }
    }
}
