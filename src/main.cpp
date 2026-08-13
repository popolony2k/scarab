/**<main.cpp>
 * Scarab main entry point source file - builds the window/engine host and
 * runs whichever game argv[1] points at (Caravellius today, but nothing
 * here is Caravellius-specific).
 *
 *  Created on: Jun 30, 2021
 *      Author: popolony2k
 */
#include <cstdio>
#include "main.h"
#include "enginehost.h"
#include "renderer/tilemaprenderer.h"


int main( int argc, char **argv ) {

    if( argc < 2 )  {
        fprintf( stderr, "Usage: %s <script.lua|project.json>\n", argv[0] );
        return EXIT_FAILURE;
    }

    SunLight :: Renderer :: TileMapRenderer  renderer( DISPLAY_W,
                                                       DISPLAY_H,
                                                       APP_NAME,
                                                       FRAMES_PER_SECOND,
                                                       false );
    Scarab :: Host :: EngineHost                engineHost( &renderer, argv[1] );
    SunLight :: TileMap :: stDimension2D       viewport;


    renderer.SetScrollStepSize( W_SCROLL_STEP_SIZE, H_SCROLL_STEP_SIZE );
    renderer.SetViewControlMode( SunLight :: Renderer :: ViewControlMode :: VIEW_CONTROL_MODE_ACTIVE );
    renderer.AddTileMapListener( &engineHost );

    viewport.pos.x = VIEWPORT_POS_X;
    viewport.pos.y = VIEWPORT_POS_Y;
    viewport.size.nWidth  = VIEWPORT_WIDTH;
    viewport.size.nHeight = VIEWPORT_HEIGHT;

    renderer.GetViewport().SetZoom( DEFAULT_ZOOM_SCALE_POS );
    renderer.GetViewport().SetDimension2D( viewport );
    renderer.SetDrawFPS( false );
    renderer.SetWindowResizeable( true );
    renderer.Start();
    renderer.Run();
    renderer.Stop();

    return EXIT_SUCCESS;
}
