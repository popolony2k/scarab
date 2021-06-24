#include "maprenderer.h"

#define DISPLAY_H         600
#define DISPLAY_W         800
#define FRAMES_PER_SECOND  60

int main(int argc, char **argv) {

    MapRenderer   renderer( DISPLAY_W,
                            DISPLAY_H,
                            "Sunlight Engine",
                            FRAMES_PER_SECOND );
    std :: string strTmxMapFile = "/home/popolony2k/Projects/C_CPP/tiled/examples/rpg/island.tmx";
    std :: string strTmxMapFile2 = "/home/popolony2k/Projects/C_CPP/tiled/examples/desert.tmx";

    //renderer.SetViewControlMode( VIEW_CONTROL_MODE_REACTIVE );
    renderer.SetMapFile( strTmxMapFile.c_str() );
    renderer.SetViewport( ( Viewport ) { 10.0, 10.0, 510.0, 590.0 } );
    renderer.SetDrawFPS( true );
    renderer.Start();
    renderer.Run();
    renderer.Stop();

	return 0;
}
