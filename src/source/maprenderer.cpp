/*
 * maprenderer.cpp
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#include <memory.h>
#include <chrono>
#include "maprenderer.h"

/*
 * Engine defaults.
 */
#define __DEFAULT_FPS                   30
#define __DEFAULT_LINE_THICKNESS        2.5f
#define __DEFAULT_MAP_ZOOM_SCALE_STEP   0.0625f
#define __DEFAULT_SCROLL_STEP_WIDTH     -1
#define __DEFAULT_SCROLL_STEP_HEIGHT    -1
#define __DEFAULT_SCROLL_STEP           -1
#define __DEFAULT_CLEAR_BACKGROUND      true
#define __DEFAULT_VIEW_CONTROL_MODE     VIEW_CONTROL_MODE_ACTIVE
#define __DEFAULT_EXIT_KEY              KEY_ESCAPE

/*
 * Engine limits.
 */
#define __MAX_ZOOM_DEPTH               256

bool MapRenderer :: m_bInitialized = false;

using namespace std :: chrono;


/**
 * TxmLib texture loader callback implementation.
 * @param szPath Texture file path;
 */
void* MapRenderer :: TextureLoaderCallback( const char *szPath )  {

    Texture2D *pTexture = new Texture2D;

    *pTexture = LoadTexture( szPath );

    return pTexture;
}

/**
 * TxmLib texture deallocation callback implementation.
 * @param pTexture Pointer to the texture that will be deallocated;
 */
void MapRenderer :: TextureFreeCallback( void *pTexture )  {

    Texture2D    *pTexture2D = ( Texture2D * ) pTexture;

    UnloadTexture( *pTexture2D );

    delete pTexture2D;
}

/**
 * Convert integer color representation to @link Color object;
 */
Color MapRenderer :: IntToColor( int color ) {

    tmx_col_bytes res = tmx_col_to_bytes( color );

    return *( ( Color * ) &res );
}

void MapRenderer :: DrawPolyline( double offset_x,
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

void MapRenderer :: DrawPolygon( double offset_x,
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

void MapRenderer :: DrawTile( void *pImage,
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
    float          fZoom     = *m_itCurrentZoom;

    DrawTextureTiled( *pTexture,
                    ( Rectangle ) { sx, sy, sw, sh },
                    ( Rectangle ) { ( dx * fZoom ),
                                    ( dy * fZoom ),
                                    ( sw * fZoom ),
                                    ( sh * fZoom ) },
                    ( Vector2 ) { 0, 0 },
                    0.0f,
                    fZoom,
                    ( Color ) { op, op, op, op } );

    //DrawTextureRec( *pTexture,
    //                ( Rectangle ) { sx, sy, sw, sh },
    //                ( Vector2 ) { dx, dy },
    //                ( Color ) { op, op, op, op } );
}

/**
 * Draw objects on canvas;
 * @param pObjgr Pointer to object group to draw;
 */
void MapRenderer :: DrawObjects( tmx_object_group *pObjgr ) {

    tmx_object *head = pObjgr -> head;
    Color      color = IntToColor( pObjgr -> color );

    while( head ) {
        if( head -> visible ) {
            float    fPosX = ( head -> x + m_CameraPos.x );
            float    fPosY = ( head -> y + m_CameraPos.y );

            switch(head -> obj_type)  {
                case OT_SQUARE :
                    DrawRectangleLinesEx( ( Rectangle ){
                                          fPosX,
                                          fPosY,
                                          ( float ) head -> width,
                                          ( float ) head -> height },
                                          m_fLineThickness,
                                          color );
                    break;
                case OT_POLYGON :
                    DrawPolygon( fPosX,
                                 fPosY,
                                 head -> content.shape -> points,
                                 head -> content.shape -> points_len,
                                 color );
                    break;

                case OT_POLYLINE :
                    DrawPolyline( fPosX,
                                  fPosY,
                                  head -> content.shape -> points,
                                  head -> content.shape -> points_len,
                                  color );
                    break;

                case OT_ELLIPSE :
                    DrawEllipseLines( fPosX + head -> width / 2.0,
                                      fPosY + head -> height / 2.0,
                                      ( float ) head -> width / 2.0,
                                      ( float ) head -> height / 2.0,
                                      color );
                    break;
            }
        }

        head = head -> next;
    }
}

/**
 * Draw image layer on canvas;
 * @param pImage Pointer to image to draw;
 */
void MapRenderer :: DrawImageLayer( tmx_image *pImage ) {

    Texture2D *pTexture = ( Texture2D * ) pImage -> resource_image;

    DrawTexture( *pTexture, 0, 0, WHITE );
}

/**
 * Draw layer on screen canvas;
 * @param pMap Pointer to layer map;
 * @param pLayer Pointer to layer with objects to draw;
 */
void MapRenderer :: DrawLayer( tmx_map *pMap, tmx_layer *pLayer ) {

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

                    if( pAnimInfo -> nMillis < nMillis )  {
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

                DrawTile( pImage,
                          x, y,
                          w, h,
                          ( j * pTs -> tile_width ) + m_CameraPos.x,
                          ( i * pTs -> tile_height ) + m_CameraPos.y,
                          opacity, nFlags );
            }
        }
    }
}

/**
 * Draw all layers on screen canvas;
 * @param pMap Pointer to layers map;
 * @param pLayer Array of layer objects to draw;
 */
void MapRenderer :: DrawAllLayers( tmx_map *pMap, tmx_layer *pLayers ) {

    while( pLayers ) {
        if( pLayers -> visible ) {
            switch( pLayers -> type )  {
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
void MapRenderer :: RenderMap( void ) {

    if( m_bClearBackground )
        ClearBackground( IntToColor( m_pTmxMap -> backgroundcolor ) );

    DrawAllLayers( m_pTmxMap, m_pTmxMap -> ly_head );
}

/**
 * Unload a previously loaded map and it's related data (animations, etc...);
 */
bool MapRenderer :: UnloadMap( void )  {

    if( m_pTmxMap )  {
        tmx_map_free( m_pTmxMap );

        /*
         * Release all allocated animations data structure.
         */
        __AnimInfoList :: iterator itItem = m_AnimInfoList.begin();

        while( itItem != m_AnimInfoList.end() )  {
            delete *itItem;
            itItem = m_AnimInfoList.erase( itItem );
        }

        m_pTmxMap = NULL;

        return true;
    }

    return false;
}

/**
 * Reset zoom to it's default state.
 */
void MapRenderer :: ResetZoom( void )  {

    m_itCurrentZoom = m_vZoomFactors.begin() +
                      ( int )( ( 1 / __DEFAULT_MAP_ZOOM_SCALE_STEP ) - 1 );
}

void MapRenderer :: InitializeZoomFactors( void )  {

    float   fZoomStep = 0.0;

    for( int nCount = 0; nCount < __MAX_ZOOM_DEPTH; nCount++ )  {
        m_vZoomFactors.push_back(fZoomStep+=__DEFAULT_MAP_ZOOM_SCALE_STEP );
    }
}

/**
 * Check user input selected previously by user (mouse,
 * joystick, keyboard, etc...)
 */
void MapRenderer :: HandleUserInput( void )  {

    bool     bKeyHandled = true;

    switch( m_ViewControlMode )  {
        case VIEW_CONTROL_MODE_REACTIVE :
            switch( GetKeyPressed() )  {
                case KEY_UP :
                    m_CameraPos.y-=m_nScrollStepWidth;
                    break;
                case KEY_DOWN :
                    m_CameraPos.y+=m_nScrollStepWidth;
                    break;
                case KEY_LEFT :
                    m_CameraPos.x-=m_nScrollStepWidth;
                    break;
                case KEY_RIGHT :
                    m_CameraPos.x+=m_nScrollStepWidth;
                    break;
                case KEY_PAGE_UP :
                    if( m_itCurrentZoom != m_vZoomFactors.end() )  {
                        m_itCurrentZoom++;

                        if( m_itCurrentZoom == m_vZoomFactors.end() )
                            m_itCurrentZoom--;
                    }
                    break;
                case KEY_PAGE_DOWN :
                    if( m_itCurrentZoom != m_vZoomFactors.begin() )
                        m_itCurrentZoom--;
                    break;
                default :
                    bKeyHandled = false;
            }
            break;
        case VIEW_CONTROL_MODE_ACTIVE :
            if( ::IsKeyDown( KEY_UP ) )
                m_CameraPos.y-=m_nScrollStepWidth;
            else
            if( ::IsKeyDown( KEY_DOWN ) )
                m_CameraPos.y+=m_nScrollStepWidth;
            else
            if( ::IsKeyDown( KEY_LEFT ) )
                m_CameraPos.x-=m_nScrollStepWidth;
            else
            if( ::IsKeyDown( KEY_RIGHT ) )
                m_CameraPos.x+=m_nScrollStepWidth;
            else
            if( ::IsKeyDown( KEY_PAGE_UP ) && ( m_itCurrentZoom != m_vZoomFactors.end() ) )  {
                m_itCurrentZoom++;

                if( m_itCurrentZoom == m_vZoomFactors.end() )
                    m_itCurrentZoom--;
            }
            else
            if( ::IsKeyDown( KEY_PAGE_DOWN ) && ( m_itCurrentZoom != m_vZoomFactors.begin() ) )
                m_itCurrentZoom--;
            else
                bKeyHandled = false;
            break;
    }

    /*
     * Not controller handled keys.
     */
    if( !bKeyHandled )  {
        switch( GetKeyPressed() )  {
            case KEY_HOME :
                ResetZoom();
                break;
        }
    }
}

/**
 * Initialize all class data.
 * @param nWidth Screen renderer width;
 * @param nHeight Screen renderer height;
 * @param szTitle Screen renderer title;
 * @param nTargetFps Renderer desired FPS;
 */
MapRenderer :: MapRenderer( int nWidth,
                            int nHeight,
                            const char *szTitle,
                            int nTargetFps )  {

    m_nWidth            = nWidth;
    m_nHeight           = nHeight;
    m_nTargetFps        = nTargetFps;
    m_strTitle          = szTitle;
    m_pTmxMap           = NULL;
    m_bIsStarted        = false;
    m_bClearBackground  = __DEFAULT_CLEAR_BACKGROUND;
    m_fLineThickness    = __DEFAULT_LINE_THICKNESS;
    m_nScrollStepWidth  = __DEFAULT_SCROLL_STEP_WIDTH;
    m_nScrollStepHeight = __DEFAULT_SCROLL_STEP_HEIGHT;
    m_ViewControlMode   = __DEFAULT_VIEW_CONTROL_MODE;
    m_strTxMapFile.clear();
    memset( &m_CameraPos, 0, sizeof( m_CameraPos ) );
    InitializeZoomFactors();
    ResetZoom();

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
MapRenderer :: ~MapRenderer( void )  {

    UnloadMap();
}

/**
 * Set the line thickness for all primitive operations.
 * @param fLineThickness The new line thickness;
 */
void MapRenderer :: SetLineThickness( float fLineThickness )  {

    m_fLineThickness = fLineThickness;
}

/**
 * Get the line thickness current set to all primitive operations.
 */
float MapRenderer :: GetLineThickness( void )  {

    return m_fLineThickness;
}

/**
 * Set the status of background cleaning for each
 * rendering cycle;
 * @param bStatus The new background clear status settings;
 */
void MapRenderer :: SetClearBackground( bool bStatus )  {

    m_bClearBackground = bStatus;
}

/**
 * Set exit key to leave the renderer when it is in running state;
 * @param key The key code representing the exit key (check raylib
 * KeyboardKey enum);
 * The default exit is ESC Key;
 */
void MapRenderer :: SetExitKey( KeyboardKey key )  {

    ::SetExitKey( key );
}

/**
 * Set the default scroll step size.
 * @param nStepWidth The new scroll step size Width
 * (-1 uses the map tile size width);
 * @param nStepHeight The new scroll step size Height
 * (-1 uses the map tile size height);
 */
void MapRenderer :: SetScrollStepSize( int nStepWidth, int nStepHeight )  {

    m_nScrollStepWidth  = nStepWidth;
    m_nScrollStepHeight = nStepHeight;
}

/**
 * Set the view port control mode;
 * Viewport control mode (active and reactive)
 * Active, the view port reacts to a single key pressing continuously;
 * Reactive, the view port reacts only for each key pressing;
 * @param mode The new view control mode;
 */
void MapRenderer :: SetViewControlMode( ViewControlMode mode )  {

    m_ViewControlMode = mode;
}

/**
 * Set the TMX map file to engine load on start.
 * @param szTmxMapFile Renderer map file;
 */
void MapRenderer :: SetMapFile( const char *szTmxMapFile )  {

    m_strTxMapFile = szTmxMapFile;

    return;
}

/**
 * Start engine renderer.
 */
bool MapRenderer :: Start( void )  {

    InitWindow( m_nWidth, m_nHeight, m_strTitle.c_str() );

    if( !IsWindowReady() ) {
        tmx_perror( "Cannot create a window" );
        return false;
    }

    SetExitKey( __DEFAULT_EXIT_KEY );
    SetTargetFPS( m_nTargetFps != -1 ? m_nTargetFps : __DEFAULT_FPS );
    m_pTmxMap = tmx_load( m_strTxMapFile.c_str() );

    if( !m_pTmxMap ) {
        tmx_perror( "Cannot load map" );
        return false;
    }

    /*
     * Set scrolling properties.
     */
    if( m_nScrollStepWidth < 0 )
        m_nScrollStepWidth = m_pTmxMap -> tile_width;

    if( m_nScrollStepHeight < 0 )
        m_nScrollStepHeight = m_pTmxMap -> tile_height;

    m_bIsStarted = ( m_pTmxMap != NULL );

    return m_bIsStarted;
}

/**
 * Stop renderer freeing all allocated resources.
 */
void MapRenderer :: Stop( void )  {

    if( m_bIsStarted )  {
        UnloadMap();
        CloseWindow();
        m_bIsStarted = false;
    }
}

/**
 * Run renderer.
 */
bool MapRenderer :: Run( void )  {

    if( m_bIsStarted )  {
        while ( !WindowShouldClose() ) {
            BeginDrawing();
            RenderMap();
            HandleUserInput();
            EndDrawing();
        }

        return true;
    }

    return false;
}
