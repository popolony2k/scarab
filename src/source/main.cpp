#include "worldrenderer.h"
#include "sprite.h"

#define DISPLAY_H         600
#define DISPLAY_W         800
#define FRAMES_PER_SECOND  60


class MyCollListener : public ICollisionListener  {

    void OnCollision( Collider *pFirst, Collider *pSecond )  {

        printf( "HIT SPRITE TO SPRITE !!!!\n" );
    }

};

class MyListener : public IWorldListener  {

    Sprite            *m_pSprite;
    Sprite            *m_pSprite2;
    Sprite            *m_pSprite3;


    public:

    MyListener( Sprite *pSprite,
                Sprite *pSprite2,
                Sprite *pSprite3) {

        m_pSprite  = pSprite;
        m_pSprite2 = pSprite2;
        m_pSprite3 = pSprite3;
    }

    virtual ~MyListener()  {}

    void OnUpdate( IWorld& world )  {
        stMatrixPosition  tilePos = { 0, 0 };
        stTile            tile;
        stLayer           layer;
        stDimension2D&    spritePos = m_pSprite -> GetDimension2D();

        if( world.GetLayer( 10, layer ) )  {
            if( world.WorldToTileMatrix( spritePos.pos, tilePos ) ) {

                //printf( "LIN [%d], COL [%d]\n", tilePos.nTileRow, tilePos.nTileCol );

                if( world.GetTile(tilePos, layer, tile) &&
                        m_pSprite -> GetCollider().Hit( tile ) ) {
                    printf( "HIT SPRITE TO TILE !!!\n" );
                }
            }
        }

        if( spritePos.pos.y == 100 )
            spritePos.pos.x--;

        if( spritePos.pos.x == 0 )
            spritePos.pos.y++;

        if( spritePos.pos.y == 300 )
            spritePos.pos.x++;

        if( spritePos.pos.x == 300 )
            spritePos.pos.y--;

        //m_pSprite -> GetDimension2D().pos.x--;
        //m_pSprite -> GetDimension2D().pos.y++;
    }
};

void LoadSprite( Sprite& sprite,
                 TextureCanvas& spriteTexture,
                 std :: string strSpriteFile,
                 int interval,
                 int nX = -1,
                 int nY = -1 )  {

    if( spriteTexture.Load( strSpriteFile ) )  {
        sprite.AddSpriteSequence( 1, &spriteTexture, interval );
        sprite.SetVisible( true );
        sprite.SetActiveSequence( 1 );
        sprite.GetDimension2D().pos.x = ( nX == -1 ? 300 : nX );
        sprite.GetDimension2D().pos.y = ( nY == -1 ? 100 : nY );
    }
}

int main(int argc, char **argv) {

    WorldRenderer    renderer( DISPLAY_W,
                               DISPLAY_H,
                               "Sunlight Engine",
                               FRAMES_PER_SECOND,
                               true );
    std :: string    strTmxMapFile  = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/rpg/island.tmx";
    std :: string    strTmxMapFile1 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/desert.tmx";
    std :: string    strTmxMapFile2 = "/home/popolony2k/Projects/C_CPP/tiled-my/examples/rpg/untitled.tmx";
    std :: string    strSpriteFile  = "/home/popolony2k/Projects/C_CPP/game-engine/resources/animations/test_hexagonal_tile_60x60x30.png";
    std :: string    strSpriteFile2 = "/home/popolony2k/Projects/C_CPP/game-engine/resources/animations/test_hexagonal_tile_60x60x30_2.png";
    std :: string    strSpriteFile3 = "/home/popolony2k/Projects/C_CPP/game-engine/resources/animations/test_hexagonal_tile_60x60x30_3.png";
    std :: string    strSpriteFile4 = "/home/popolony2k/Projects/C_CPP/game-engine/resources/animations/test_hexagonal_tile_60x60x30_4.png";
    std :: string    strSpriteFile5 = "/home/popolony2k/Projects/C_CPP/game-engine/resources/animations/chibi-layered.png";
    std :: string    strSpriteFile6 = "/home/popolony2k/Projects/C_CPP/game-engine/resources/animations/sonicwalk.png";


    stDimension2D    viewport;
    stLayer          layer;
    TextureCanvas    spriteTexture;
    TextureCanvas    spriteTexture2;
    TextureCanvas    spriteTexture3;
    TextureCanvas    spriteTexture4;
    TextureCanvas    spriteTexture5;
    TextureCanvas    spriteTexture6;


    Sprite           sprite;
    Sprite           sprite2;
    Sprite           sprite3;
    CollisionManager collisionManager( &renderer );
    MyCollListener   myCollision;
    MyListener       my( &sprite,
                         &sprite2,
                         &sprite3 );


    viewport.pos.x = 10;
    viewport.pos.y = 10;
    viewport.size.nWidth  = 510;
    viewport.size.nHeight = 590;

    renderer.AddWorldListener( &my );

    //renderer.SetViewControlMode( VIEW_CONTROL_MODE_REACTIVE );
    renderer.SetMapFile( strTmxMapFile.c_str() );
    renderer.GetViewport().SetDimension2D( viewport );
    renderer.SetDrawFPS( true );
    renderer.Start();
    renderer.GetLayer( 1, layer );
    layer.nOpacity = 254;
    renderer.SetLayer( 1, layer );

    LoadSprite( sprite, spriteTexture5, strSpriteFile5, 150 );
    spriteTexture5.GetDimension2D().size.nHeight = 16;
    spriteTexture5.GetDimension2D().size.nWidth  = 16;
    spriteTexture5.SetTileSize( 16 );

    LoadSprite( sprite2, spriteTexture, strSpriteFile, 1000 );
    LoadSprite( sprite2, spriteTexture2, strSpriteFile2, 1000 );
    LoadSprite( sprite2, spriteTexture3, strSpriteFile3, 1000 );
    LoadSprite( sprite2, spriteTexture4, strSpriteFile4, 1000 );

    LoadSprite( sprite3, spriteTexture6, strSpriteFile6, 150, 100, 250 );
    spriteTexture6.GetDimension2D().size.nHeight = 80;
    spriteTexture6.GetDimension2D().size.nWidth  = 80;
    spriteTexture6.SetTileSize( 80 );

    renderer.AddSprite( 0, sprite );
    renderer.AddSprite( 1, sprite2 );
    renderer.AddSprite( 3, sprite3 );

    renderer.GetCollisionManager().AddColliderLayerRule( 0, 1 );
    renderer.GetCollisionManager().AddCollisionListener( &myCollision );

    renderer.SetScrollStepSize( 16, 16 );
    renderer.SetViewControlMode( ViewControlMode::VIEW_CONTROL_MODE_REACTIVE );

    renderer.Run();
    sprite3.Unload();
    renderer.Stop();

	return 0;
}
