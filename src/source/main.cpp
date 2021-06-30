#include "worldrenderer.h"

#define DISPLAY_H         600
#define DISPLAY_W         800
#define FRAMES_PER_SECOND  60

int main(int argc, char **argv) {

    WorldRenderer   renderer( DISPLAY_W,
                              DISPLAY_H,
                              "Sunlight Engine",
                              FRAMES_PER_SECOND );
    std :: string strTmxMapFile  = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/rpg/island.tmx";
    std :: string strTmxMapFile2 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/desert.tmx";
    std :: string strTmxMapFile0 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/rpg/untitled.tmx";
    stLayer       layer;

    //renderer.SetViewControlMode( VIEW_CONTROL_MODE_REACTIVE );
    renderer.SetMapFile( strTmxMapFile.c_str() );
    renderer.SetViewport( ( Viewport ) { 10.0, 10.0, 510.0, 590.0 } );
    renderer.SetDrawFPS( true );
    renderer.Start();
    renderer.GetLayer( 1, layer );
    layer.nOpacity = 254;
    renderer.SetLayer( 1, layer );
    renderer.Run();
    renderer.Stop();

	return 0;
}
