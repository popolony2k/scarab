#include "renderer.h"

#define DISPLAY_H         600
#define DISPLAY_W         800
#define FRAMES_PER_SECOND  60

int main(int argc, char **argv) {

    Renderer   renderer( DISPLAY_W,
                         DISPLAY_H,
                         "Sunlight Engine",
                         "/home/popolony2k/Projects/C_CPP/tiled/examples/rpg/island.tmx",
                         FRAMES_PER_SECOND );

    renderer.Start();
    renderer.Run();
    renderer.Stop();


	return 0;
}
