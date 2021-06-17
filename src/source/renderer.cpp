/*
 * renderer.cpp
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#include "renderer.h"

#define __DEFAULT_FPS       30
#define __LINE_THICKNESS    2.5



bool Renderer :: m_bInitialized = false;


/**
 * Raylib texture loader callback implementation.
 * @param szPath Texture file path;
 */
void* Renderer :: TextureLoaderCallback( const char *szPath )  {

    Texture2D *pTexture = new Texture2D;

    *pTexture = LoadTexture( szPath );

    return pTexture;
}

/**
 * Raylib texture deallocation callback implementation.
 * @param pTexture Pointer to the texture that will be deallocated;
 */
void Renderer :: TextureFreeCallback( void *pTexture )  {

    Texture2D    *pTexture2D = ( Texture2D * ) pTexture;

    UnloadTexture( *pTexture2D );

    delete pTexture2D;
}


Color Renderer :: IntToColor( int color ) {

    tmx_col_bytes res = tmx_col_to_bytes( color );

    return *( ( Color * ) &res );
}

void Renderer :: DrawPolyline( double offset_x,
                               double offset_y,
                               double **points,
                               int points_count,
                               Color color ) {

    for( int i=1; i < points_count; i++ ) {
        DrawLineEx( ( Vector2 ) { offset_x + points[i-1][0],
                                  offset_y + points[i-1][1] },
                    ( Vector2 ) { offset_x + points[i][0],
                                  offset_y + points[i][1] },
                    __LINE_THICKNESS, color );
    }
}

void Renderer :: DrawPolygon( double offset_x,
                              double offset_y,
                              double **points,
                              int points_count,
                              Color color ) {
    DrawPolyline( offset_x,
                  offset_y,
                  points,
                  points_count,
                  color );

    if( points_count > 2 ) {
        DrawLineEx( ( Vector2 ) { offset_x + points[0][0],
                                  offset_y + points[0][1] },
                    ( Vector2 ) { offset_x + points[points_count-1][0],
                                  offset_y + points[points_count-1][1] },
                    __LINE_THICKNESS, color );
    }
}

void Renderer :: DrawObjects( tmx_object_group *objgr ) {

    tmx_object *head = objgr->head;
    Color      color = IntToColor( objgr -> color );

    while( head ) {
        if( head -> visible ) {
            switch(head -> obj_type)  {
                case OT_SQUARE :
                    DrawRectangleLinesEx( ( Rectangle ){ head->x, head->y,
                                          head -> width,
                                          head -> height },
                                          __LINE_THICKNESS,
                                          color );
                    break;
                case OT_POLYGON :
                    DrawPolygon( head -> x,
                                 head -> y,
                                 head -> content.shape -> points,
                                 head -> content.shape -> points_len,
                                 color );
                    break;

                case OT_POLYLINE :
                    DrawPolyline( head -> x,
                                  head -> y,
                                  head -> content.shape -> points,
                                  head -> content.shape -> points_len,
                                  color );
                    break;

                case OT_ELLIPSE :
                    DrawEllipseLines( head -> x + head -> width / 2.0,
                                      head -> y + head -> height / 2.0,
                                      head -> width / 2.0,
                                      head -> height / 2.0,
                                      color );
                    break;
            }
        }

        head = head -> next;
    }
}

void Renderer :: DrawImageLayer( tmx_image *pImage ) {

    Texture2D *pTexture = ( Texture2D * ) pImage -> resource_image;

    DrawTexture( *pTexture, 0, 0, WHITE );
}

void Renderer :: DrawTile( void *pImage,
                           unsigned int sx,
                           unsigned int sy,
                           unsigned int sw,
                           unsigned int sh,
                           unsigned int dx,
                           unsigned int dy,
                           float opacity,
                           unsigned int flags ) {

    Texture2D *pTexture = (Texture2D*) pImage;
    int       op        = 0xFF * opacity;

    DrawTextureRec( *pTexture,
                    ( Rectangle ) { sx, sy, sw, sh },
                    ( Vector2 ) { dx, dy },
                    ( Color ) { op, op, op, op } );
}

void Renderer :: DrawLayer( tmx_map *pMap, tmx_layer *pLayer ) {

    unsigned long i, j;
    unsigned int  gid, x, y, w, h, flags;
    float         op;
    tmx_tileset   *ts;
    tmx_image     *im;
    void          *image;

    op = pLayer->opacity;

    for (i=0; i < pMap -> height; i++) {
        for (j=0; j < pMap -> width; j++) {
            gid = (pLayer -> content.gids[(i * pMap -> width)+j]) & TMX_FLIP_BITS_REMOVAL;

            if( pMap -> tiles[gid] != NULL ) {
                ts = pMap->tiles[gid]->tileset;
                im = pMap->tiles[gid]->image;
                x  = pMap->tiles[gid]->ul_x;
                y  = pMap->tiles[gid]->ul_y;
                w  = ts->tile_width;
                h  = ts->tile_height;

                if (im) {
                    image = im->resource_image;
                }
                else {
                    image = ts->image->resource_image;
                }

                flags = ( pLayer->content.gids[(i*pMap->width)+j]) & ~TMX_FLIP_BITS_REMOVAL;
                DrawTile( image, x, y, w, h, j*ts->tile_width, i*ts->tile_height, op, flags);
            }
        }
    }
}

void Renderer :: DrawAllLayers( tmx_map *pMap, tmx_layer *pLayers ) {

    while( pLayers ) {
        if( pLayers -> visible ) {
            switch( pLayers->type )  {
                case L_GROUP :
                    DrawAllLayers( pMap, pLayers -> content.group_head ); // recursive call
                    break;
                case L_OBJGR :
                    DrawObjects( pLayers -> content.objgr );
                    break;
                case L_IMAGE :
                    DrawImageLayer( pLayers -> content.image );
                    break;
                case L_LAYER :
                    DrawLayer( pMap, pLayers );
            }
        }

        pLayers = pLayers -> next;
    }
}

void Renderer :: RenderMap( void ) {

    ClearBackground( IntToColor( m_pTmxMap -> backgroundcolor ) );
    DrawAllLayers( m_pTmxMap, m_pTmxMap -> ly_head );
}

/**
 * Initialize all class data.
 * @param nWidth Screen renderer width;
 * @param nHeight Screen renderer height;
 * @param szTitle Screen renderer title;
 * @param szTmxMapFile Renderer map file;
 * @param nTargetFps Renderer desired FPS;
 */
Renderer :: Renderer( int nWidth,
                      int nHeight,
                      const char *szTitle,
                      const char *szTmxMapFile,
                      int nTargetFps )  {

    m_nWidth        = nWidth;
    m_nHeight       = nHeight;
    m_nTargetFps    = nTargetFps;
    m_strTitle      = szTitle;
    m_strTmxMapFile = szTmxMapFile;
    m_pTmxMap       = NULL;
    m_bIsStarted    = false;

    /*
     * Set the raylib callback texture handlers (this call is protected
     * to be called just only one time in whole program execution).
     */
    if( !m_bInitialized )  {
        tmx_img_load_func = TextureLoaderCallback;
        tmx_img_free_func = TextureFreeCallback;
        m_bInitialized    = true;
    }
}

/**
 * Destructor. Finalize all class data.
 */
Renderer :: ~Renderer( void )  {

}

/**
 * Start engine renderer.
 */
bool Renderer :: Start( void )  {

    InitWindow( m_nWidth, m_nHeight, m_strTitle.c_str() );

    if( !IsWindowReady() ) {
        tmx_perror( "Cannot create a window" );
        return false;
    }

    SetTargetFPS( m_nTargetFps != -1 ? m_nTargetFps : __DEFAULT_FPS );

    m_pTmxMap = tmx_load( m_strTmxMapFile.c_str() );

    if( !m_pTmxMap ) {
        tmx_perror( "Cannot load map" );
        return false;
    }

    m_bIsStarted = true;

    return m_bIsStarted;
}

/**
 * Stop renderer freeing all allocated resources.
 */
void Renderer :: Stop( void )  {

    if( m_bIsStarted )  {
        if( m_pTmxMap )
            tmx_map_free( m_pTmxMap );

        CloseWindow();
        m_bIsStarted = false;
    }
}

/**
 * Run renderer.
 */
bool Renderer :: Run( void )  {

    if( m_bIsStarted )  {
        while (!WindowShouldClose()) {
            BeginDrawing();
            RenderMap();
            EndDrawing();
        }

        return true;
    }

    return false;
}
