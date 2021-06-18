/*
 * renderer.cpp
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#include "renderer.h"
#include <memory.h>
#include <chrono>

/*
 * Engine defaults.
 */
#define __DEFAULT_FPS                30
#define __DEFAULT_LINE_THICKNESS    2.5

bool Renderer :: m_bInitialized = false;

using namespace std :: chrono;


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
                    m_fLineThickness, color );
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
                    m_fLineThickness, color );
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
                                          m_fLineThickness,
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

    Texture2D      *pTexture = ( Texture2D * ) pImage;
    unsigned char  op        = ( 0xFF * opacity );

    DrawTextureRec( *pTexture,
                    ( Rectangle ) { sx, sy, sw, sh },
                    ( Vector2 ) { dx, dy },
                    ( Color ) { op, op, op, op } );
}

void Renderer :: DrawLayer( tmx_map *pMap, tmx_layer *pLayer ) {

    float         opacity;

    opacity = pLayer -> opacity;

    for( unsigned long i = 0; i < pMap -> height; i++ ) {
        for( unsigned long j = 0; j < pMap -> width; j++ ) {
            tmx_tile      *pTile;
            uint32_t       nLayerGID = pLayer -> content.gids[( i * pMap -> width ) + j];
            uint32_t       nGID      = nLayerGID & TMX_FLIP_BITS_REMOVAL;

            pTile = pMap -> tiles[nGID];

            if( pTile != NULL ) {
                uint32_t       nFlags = nLayerGID & ~TMX_FLIP_BITS_REMOVAL;
                tmx_image      *pIm   = pTile -> image;
                tmx_tileset    *pTs;
                void           *pImage;
                unsigned int   x;
                unsigned int   y;
                unsigned int   w;
                unsigned int   h;


                /*
                 * Perform tile animation
                 */
                if( pTile -> animation_len )  {
                    steady_clock :: time_point now     = steady_clock :: now();
                    int64_t                    nMillis = duration_cast<milliseconds>( now.time_since_epoch() ).count();
                    __stTileAnimInfo           *pAnimInfo = ( __stTileAnimInfo * ) pTile -> user_data.pointer;

                    if( !pAnimInfo )  {
                        pAnimInfo = new __stTileAnimInfo;
                        memset( pAnimInfo, 0, sizeof( __stTileAnimInfo ) );
                        pAnimInfo -> nMillis   = nMillis;
                        pAnimInfo -> pNextTile = pTile;
                        pTile -> user_data.pointer = pAnimInfo;
                        m_AnimInfoList.push_back( pAnimInfo );
                    }

                    if( nMillis > pAnimInfo -> nMillis )  {
                        unsigned int     nNextFrmGID;
                        _tmx_frame&      tmxAnimFrm = pTile -> animation[pAnimInfo -> nCounter];

                        if( pAnimInfo -> nCounter < pTile -> animation_len )  {
                            pAnimInfo -> nCounter++;
                            nNextFrmGID = ( pMap -> ts_head -> firstgid +
                                            tmxAnimFrm.tile_id );
                        }
                        else  {
                            nNextFrmGID = nGID;
                            pAnimInfo -> nCounter = 0;
                        }

                        pAnimInfo -> nMillis   = ( tmxAnimFrm.duration + nMillis );
                        pAnimInfo -> pNextTile = pMap -> tiles[nNextFrmGID];
                        pTile = pAnimInfo -> pNextTile;

                        if( !pTile )
                            pTile = pMap -> tiles[nGID];
                    }
                    else  {
                        pTile = ( ( __stTileAnimInfo * ) pTile -> user_data.pointer ) -> pNextTile;
                    }
                }

                pTs = pTile -> tileset;
                x   = pTile -> ul_x;
                y   = pTile -> ul_y;
                w   = pTs -> tile_width;
                h   = pTs -> tile_height;

                if( pIm ) {
                    pImage = pIm -> resource_image;
                }
                else {
                    pImage = pTs -> image -> resource_image;
                }

                DrawTile( pImage, x, y, w, h,
                          ( j * pTs -> tile_width ),
                          ( i * pTs -> tile_height ),
                          opacity, nFlags );
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
                    break;
            }
        }

        pLayers = pLayers -> next;
    }
}

/**
 * Render all map objects.
 */
void Renderer :: RenderMap( void ) {

    // TODO: Check if ClearBackground is really needed;
    //ClearBackground( IntToColor( m_pTmxMap -> backgroundcolor ) );
    DrawAllLayers( m_pTmxMap, m_pTmxMap -> ly_head );
}

/**
 * Release all allocated layer needed data.
 */
void Renderer :: ReleaseLayer( void )  {

    /*
     * Release all allocated animations data structure.
     */
    __AnimInfoList :: iterator itItem = m_AnimInfoList.begin();

    while( itItem != m_AnimInfoList.end() )  {
        delete *itItem;
        itItem = m_AnimInfoList.erase( itItem );
    }
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

    m_nWidth         = nWidth;
    m_nHeight        = nHeight;
    m_nTargetFps     = nTargetFps;
    m_strTitle       = szTitle;
    m_strTmxMapFile  = szTmxMapFile;
    m_pTmxMap        = NULL;
    m_bIsStarted     = false;
    m_fLineThickness = __DEFAULT_LINE_THICKNESS;

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

    ReleaseLayer();
}

/**
 * Set the line thickness for all primitive operations.
 * @param fLineThickness The new line thickness;
 */
void Renderer :: SetLineThickness( float fLineThickness )  {

    m_fLineThickness = fLineThickness;
}

/**
 * Get the line thickness current set to all primitive operations.
 */
float Renderer :: GetLineThickness( void )  {

    return m_fLineThickness;
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
        if( m_pTmxMap )  {
            tmx_map_free( m_pTmxMap );
            ReleaseLayer();
        }

        CloseWindow();
        m_bIsStarted = false;
    }
}

/**
 * Run renderer.
 */
bool Renderer :: Run( void )  {

    if( m_bIsStarted )  {
        while ( !WindowShouldClose() ) {
            BeginDrawing();
            RenderMap();
            EndDrawing();
        }

        return true;
    }

    return false;
}
