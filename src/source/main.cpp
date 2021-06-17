#include "renderer.h"

#define DISPLAY_H 600
#define DISPLAY_W 800

int main(int argc, char **argv) {

    Renderer   renderer( DISPLAY_W,
                         DISPLAY_H,
                         "raylib example",
                         "/home/popolony2k/Projects/C_CPP/tiled/examples/rpg/island.tmx");

    renderer.Start();
    renderer.Run();
    renderer.Stop();


	return 0;
}
