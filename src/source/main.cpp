#include "worldrenderer.h"
#include "sprite.h"

#define DISPLAY_H         600
#define DISPLAY_W         800
#define FRAMES_PER_SECOND  60


class MyListener : public IWorldListener  {

    stDimension2D     pos;
    Collider          collider;
    Sprite            *m_pSprite;

    public:

    MyListener( Sprite *pSprite ) : collider( &pos )  {

        m_pSprite = pSprite;
        pos.pos.x = 10;
        pos.pos.y = 10;
        pos.size.nWidth  = 16;
        pos.size.nHeight = 16;
    }

    virtual ~MyListener()  {}

    void OnUpdate( IWorld& world )  {
        stMatrixPosition  tilePos = { 0, 0 };
        stTile            tile;
        stLayer           layer;

        if( world.GetLayer( 6, layer ) )  {
            if( world.WorldToTileMatrix( pos.pos, tilePos ) ) {
                if( world.GetTile(tilePos, layer, tile) &&
                    collider.Hit( tile ) ) {
                    //printf( "HIT\n" );
                }
            }
        }

        m_pSprite -> Update();
    }
};

void LoadSprite( Sprite& sprite,
                 TextureCanvas& spriteTexture,
                 std :: string strSpriteFile, int interval )  {

    if( spriteTexture.Load( strSpriteFile ) )  {
        sprite.AddSpriteSequence( 1, &spriteTexture, interval );
        sprite.SetVisible( true );
        sprite.SetActiveSequence( 1 );
        sprite.GetDimension2D().pos.x = 100;
        sprite.GetDimension2D().pos.y = 100;
    }
}

int main(int argc, char **argv) {

    WorldRenderer   renderer( DISPLAY_W,
                              DISPLAY_H,
                              "Sunlight Engine",
                              FRAMES_PER_SECOND,
                              true );
    std :: string  strTmxMapFile  = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/rpg/island.tmx";
    std :: string  strTmxMapFile1 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/desert.tmx";
    std :: string  strTmxMapFile2 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/rpg/untitled.tmx";
    std :: string  strSpriteFile  = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/test_hexagonal_tile_60x60x30.png";
    std :: string  strSpriteFile2 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/test_hexagonal_tile_60x60x30_2.png";
    std :: string  strSpriteFile3 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/test_hexagonal_tile_60x60x30_3.png";
    std :: string  strSpriteFile4 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/test_hexagonal_tile_60x60x30_4.png";
    std :: string  strSpriteFile5 = "/home/popolony2k/Projects/C_CPP/game-engine/resources/animations/sonicwalk.png";


    stLayer        layer;
    TextureCanvas spriteTexture;
    TextureCanvas spriteTexture2;
    TextureCanvas spriteTexture3;
    TextureCanvas spriteTexture4;
    TextureCanvas spriteTexture5;

    Sprite         sprite;
    MyListener      my( &sprite );


    renderer.AddWorldListener( &my );

    //renderer.SetViewControlMode( VIEW_CONTROL_MODE_REACTIVE );
    renderer.SetMapFile( strTmxMapFile.c_str() );
    renderer.SetViewport( ( stViewport ) { 10.0, 10.0, 510.0, 590.0 } );
    renderer.SetDrawFPS( true );
    renderer.Start();
    renderer.GetLayer( 1, layer );
    layer.nOpacity = 254;
    renderer.SetLayer( 1, layer );

    //LoadSprite( sprite, spriteTexture, strSpriteFile, 1000 );
    //LoadSprite( sprite, spriteTexture2, strSpriteFile2, 1000 );
    //LoadSprite( sprite, spriteTexture3, strSpriteFile3, 1000 );
    //LoadSprite( sprite, spriteTexture4, strSpriteFile4, 1000 );

    LoadSprite( sprite, spriteTexture5, strSpriteFile5, 150 );
    spriteTexture5.GetDimension2D().size.nHeight = 80;
    spriteTexture5.GetDimension2D().size.nWidth  = 80;
    spriteTexture5.SetTileSize( 80 );

    renderer.Run();
    renderer.Stop();

	return 0;
}
