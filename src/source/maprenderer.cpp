/*
 * maprenderer.cpp
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#include <memory.h>
#include <chrono>
#include <cmath>
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
#define __DEFAULT_USER_ZOOM_STATUS      true
#define __DEFAULT_RESIZEABLE_STATUS     false
#define __DEFAULT_DRAW_FPS_STATUS       false
#define __DEFAULT_PREFERRED_ZOOM_POS    ( ( unsigned )( ( 1 / __DEFAULT_MAP_ZOOM_SCALE_STEP ) - 1 ) )
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

/**
 * Calculates the clipped rectangle area based on camera
 * position and viewport boundaries;
 * @param nSourceX Source object X coordinate;
 * @param nSourceY Source object Y coordinate;
 * @param nDestX destination X coordinate on texture;
 * @param nDestY destination Y coordinate on texture;
 * @param fViewX Calculated object x coordinate based on
 * viewport boundaries;
 * @param fViewY Calculated object y coordinate based on
 * viewport boundaries;
 * @param fViewWidth Calculated object width based on
 * viewport boundaries;
 * @param fViewHeight Calculated object height based on
 * viewport boundaries;
 * @param bResetViewOnNegative When this parameter is set
 * the width parameters fViewX and fViewY are reset to 0.0
 * coordinate when each related coordinate (x,y) are negative;
 */
bool MapRenderer :: GetClippedArea( int32_t nSourceW,
                                    int32_t nSourceH,
                                    int32_t nDestX,
                                    int32_t nDestY,
                                    float& fViewX,
                                    float& fViewY,
                                    float& fViewWidth,
                                    float& fViewHeight,
                                    bool bResetViewOnNegative )  {

    float       fClippingX;
    float       fClippingY;
    int32_t     nTemp;

    nDestX+=m_CameraPos.x;
    nDestY+=m_CameraPos.y;

    if( ( nDestX > ( m_Viewport.x + m_Viewport.width ) ) ||
        ( nDestY > ( m_Viewport.y + m_Viewport.height ) )||
        ( nDestX < 0.0 ) || ( nDestY < 0.0 ) )  {

        /*
         * Adjust origin coordinates (x,y) and dimension (width, height)
         * when scenarioo is moving to negative positions outside viewport
         * borders (left and top moving).
         */
        if( !bResetViewOnNegative )
            return false;

        if( nDestX < m_Viewport.x )  {
            nTemp = std :: abs( nDestX );
            nSourceW-=( nTemp < nSourceW ? nTemp : nSourceW );
        }

        if( nDestY < m_Viewport.y )  {
            nTemp = std :: abs( nDestY );
            nSourceH-=( nTemp < nSourceH ? nTemp : nSourceH );
        }

        nDestX = ( nDestX < 0.0 ? 0.0 : nDestX );
        nDestY = ( nDestY < 0.0 ? 0.0 : nDestY );
    }

    fViewX      = ( ( nDestX * m_fZoomFactor ) + m_Viewport.x );
    fViewY      = ( ( nDestY * m_fZoomFactor ) + m_Viewport.y );
    fViewWidth  = ( nSourceW * m_fZoomFactor );
    fViewHeight = ( nSourceH * m_fZoomFactor );
    fClippingX  = ( fViewX + fViewWidth );
    fClippingY  = ( fViewY + fViewHeight );

    if( fClippingX > m_Viewport.width )  {
        fClippingX = ( fClippingX - m_Viewport.width );

        if( fClippingX > fViewWidth )
            return false;

        fViewWidth = fViewWidth - fClippingX;
    }

    if( fClippingY > m_Viewport.height )  {
        fClippingY = ( fClippingY - m_Viewport.height );

        if( fClippingY > fViewHeight )
            return false;

        fViewHeight = fViewHeight - fClippingY;
    }

    return true;
}

void MapRenderer :: DrawPolyline( double offset_x,
                                  double offset_y,
                                  double **points,
                                  int points_count,
                                  Color color ) {

    offset_x+=m_CameraPos.x;
    offset_y+=m_CameraPos.y;

    for( int i=1; i < points_count; i++ ) {
        DrawLineEx( ( Vector2 ) { ( float ) ( offset_x + points[i-1][0] ),
                                  ( float ) ( offset_y + points[i-1][1] ) },
                    ( Vector2 ) { ( float ) ( offset_x + points[i][0] ),
                                  ( float ) ( offset_y + points[i][1] ) },
                    m_fLineThickness, color );
    }
}

void MapRenderer :: DrawPolygon( double offset_x,
                                 double offset_y,
                                 double **points,
                                 int points_count,
                                 Color color ) {
    offset_x+=m_CameraPos.x;
    offset_y+=m_CameraPos.y;

    DrawPolyline( offset_x,
                  offset_y,
                  points,
                  points_count,
                  color );

    if( points_count > 2 ) {
        DrawLineEx( ( Vector2 ) { ( float ) ( offset_x + points[0][0] ),
                                  ( float ) ( offset_y + points[0][1] ) },
                    ( Vector2 ) { ( float ) ( offset_x + points[points_count-1][0] ),
                                  ( float ) ( offset_y + points[points_count-1][1] ) },
                    m_fLineThickness,
                    color );
    }
}

/**
 * Draw a square primitive to specified position on texture.
 * @param fOffset_x X square coordinate;
 * @param fOffset_y Y square coordinate;
 * @param fWidth The square width;
 * @param fHeight The square height;
 * @param color Rectangle color;
 */
void MapRenderer :: DrawRectangle( double fOffset_x,
                                   double fOffset_y,
                                   double fWidth,
                                   double fHeight,
                                   Color color )  {

    float          fViewStartX;
    float          fViewStartY;
    float          fClippedWidth;
    float          fClippedHeight;

    if( GetClippedArea( fWidth, fHeight,
                        fOffset_x, fOffset_y,
                        fViewStartX, fViewStartY,
                        fClippedWidth, fClippedHeight, true ) ) {
        float fViewEndX = ( float ) ( fViewStartX + fClippedWidth );
        float fViewEndY = ( float ) ( fViewStartY + fClippedHeight );

        if( ( fClippedWidth > 0.0 ) && ( fClippedHeight > 0.0 ) )  {
            // Top line
            DrawLineEx( ( Vector2 ) { fViewStartX,
                                      fViewStartY },
                        ( Vector2 ) { fViewEndX,
                                      fViewStartY },
                        m_fLineThickness,
                        color );

            // Bottom line
            DrawLineEx( ( Vector2 ) { fViewStartX,
                                      fViewEndY },
                        ( Vector2 ) { fViewEndX,
                                      fViewEndY },
                        m_fLineThickness,
                        color );

            // Left line
            DrawLineEx( ( Vector2 ) { fViewStartX,
                                      fViewStartY },
                        ( Vector2 ) { fViewStartX,
                                      fViewEndY },
                        m_fLineThickness,
                        color );

            // Right line
            DrawLineEx( ( Vector2 ) { fViewEndX,
                                      fViewStartY },
                        ( Vector2 ) { fViewEndX,
                                      fViewEndY },
                        m_fLineThickness,
                        color );
        }
    }
}

/**
 * Draw an ellipse primitive to specified position on texture.
 * @param fOffset_x X square coordinate;
 * @param fOffset_y Y square coordinate;
 * @param fWidth The square width;
 * @param fHeight The square height;
 * @param color Rectangle color;
 */
void MapRenderer :: DrawEllipse( double fOffset_x,
                                 double fOffset_y,
                                 double fWidth,
                                 double fHeight,
                                 Color color )  {

    float          fViewStartX;
    float          fViewStartY;
    float          fClippedWidth;
    float          fClippedHeight;

    // FIXME: Ellipses don't work well with rectangular clipping area
    fWidth-=( fWidth / 2.0 );
    fHeight-=( fHeight / 2.0 );
    fOffset_x+=fWidth;
    fOffset_y+=fHeight;

    if( GetClippedArea( fWidth, fHeight,
                        fOffset_x, fOffset_y,
                        fViewStartX, fViewStartY,
                        fClippedWidth, fClippedHeight, true ) ) {
        float fViewEndX = ( float ) ( fViewStartX + fClippedWidth );
        float fViewEndY = ( float ) ( fViewStartY + fClippedHeight );

        if( ( fClippedWidth > 0.0 ) && ( fClippedHeight > 0.0 ) )  {

            DrawEllipseLines( fViewStartX,
                              fViewStartY,
                              fClippedWidth,
                              fClippedHeight,
                              color );
        }
    }
}

/**
 * Draw a tile to specified position on texture.
 * @param pImage Pointer to a @link Texture2D object used as renderer.
 * @param uSourceX Source tile X coordinate;
 * @param uSourceY Source tile Y coordinate;
 * @param uSourceW Source tile width;
 * @param uSourceH Source tile height;
 * @param uDestX destination X coordinate on texture;
 * @param uDestY destination Y coordinate on texture;
 * @param opacity opacity level to be applied on texture;
 */
void MapRenderer :: DrawTile( void *pImage,
                              int32_t nSourceX,
                              int32_t nSourceY,
                              int32_t nSourceW,
                              int32_t nSourceH,
                              int32_t nDestX,
                              int32_t nDestY,
                              float fOpacity ) {

    Texture2D      *pTexture   = ( Texture2D * ) pImage;
    unsigned char  op          = ( 0xFF * fOpacity );
    float          fViewX;
    float          fViewY;
    float          fClippedWidth;
    float          fClippedHeight;

    if( GetClippedArea( nSourceW, nSourceH,
                        nDestX, nDestY,
                        fViewX, fViewY,
                        fClippedWidth, fClippedHeight ) ) {
        DrawTextureTiled( *pTexture,
                        ( Rectangle ) { ( float ) nSourceX,
                                        ( float ) nSourceY,
                                        ( float ) nSourceW,
                                        ( float ) nSourceH },
                        ( Rectangle ) { fViewX,
                                        fViewY,
                                        fClippedWidth,
                                        fClippedHeight },
                        ( Vector2 ) { 0, 0 },
                        0.0f,
                        m_fZoomFactor,
                        ( Color ) { op, op, op, op } );
    }
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
            switch( head -> obj_type )  {
                case OT_SQUARE :
                    DrawRectangle( head -> x,
                                   head -> y,
                                   head -> width,
                                   head -> height,
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
                    DrawEllipse( head -> x,
                                 head -> y,
                                 head -> width,
                                 head -> height,
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

    float         fOpacity;

    fOpacity = pLayer -> opacity;

    for( unsigned long i = 0; i < pMap -> height; i++ ) {
        for( unsigned long j = 0; j < pMap -> width; j++ ) {
            tmx_tile      *pTile;
            uint32_t       nLayerGID = pLayer -> content.gids[( i * pMap -> width ) + j];
            uint32_t       nGID      = nLayerGID & TMX_FLIP_BITS_REMOVAL;

            pTile = pMap -> tiles[nGID];

            if( pTile != NULL ) {
                tmx_image      *pIm   = pTile -> image;
                tmx_tileset    *pTs;
                void           *pImage;


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

                if( pIm ) {
                    pImage = pIm -> resource_image;
                }
                else {
                    pImage = pTs -> image -> resource_image;
                }

                DrawTile( pImage,
                          pTile -> ul_x,
                          pTile -> ul_y,
                          pTs -> tile_width,
                          pTs -> tile_height,
                          ( ( j * pTs -> tile_width ) + pTs -> x_offset ),
                          ( ( i * pTs -> tile_height ) + pTs -> y_offset ),
                          fOpacity );
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

    if( m_bDrawFPS )
        DrawFPS( 0,  0 );
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

    m_nCurrentZoomPos = m_nPreferredZoomPos;
    m_fZoomFactor     = m_vZoomFactorList[m_nCurrentZoomPos];
}

/**
 * Performs Zoom In effect.
 */
void MapRenderer :: ZoomIn( void )  {

    if( ( m_nCurrentZoomPos < m_ZoomBorderLimits.second ) && m_bEnabledUserZoom )  {
        m_nCurrentZoomPos++;

        if( m_nCurrentZoomPos == m_ZoomBorderLimits.second )
            m_nCurrentZoomPos--;

        m_fZoomFactor = m_vZoomFactorList[m_nCurrentZoomPos];
    }
}

/**
 * Performs Zoom Out effect.
 */
void MapRenderer :: ZoomOut( void )  {

    if( ( m_nCurrentZoomPos > m_ZoomBorderLimits.first ) && m_bEnabledUserZoom )  {
        m_nCurrentZoomPos--;
        m_fZoomFactor = m_vZoomFactorList[m_nCurrentZoomPos];
    }
}

/**
 * Initialize the zoom engine.
 */
void MapRenderer :: InitializeZoomEngine( void )  {

    float   fZoomStep = 0.0;

    /*
     * Fill all zoom factor list.
     */
    for( int nCount = 0; nCount < __MAX_ZOOM_DEPTH; nCount++ )  {
        m_vZoomFactorList.push_back(fZoomStep+=__DEFAULT_MAP_ZOOM_SCALE_STEP );
    }

    SetPreferredZoom( __DEFAULT_PREFERRED_ZOOM_POS );

    m_ZoomBorderLimits.first  = 0;
    m_ZoomBorderLimits.second = __MAX_ZOOM_DEPTH;
    m_bEnabledUserZoom        = __DEFAULT_USER_ZOOM_STATUS;
    m_fZoomFactor             = m_vZoomFactorList[m_nPreferredZoomPos];
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
                    ZoomIn();
                    break;
                case KEY_PAGE_DOWN :
                    ZoomOut();
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
            if( ::IsKeyDown( KEY_PAGE_UP ) )
                ZoomIn();
            else
            if( ::IsKeyDown( KEY_PAGE_DOWN ) )
                ZoomOut();
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
 * @param fWidth Screen renderer width;
 * @param fHeight Screen renderer height;
 * @param szTitle Screen renderer title;
 * @param nTargetFps Renderer desired FPS;
 */
MapRenderer :: MapRenderer( float fWidth,
                            float fHeight,
                            const char *szTitle,
                            int nTargetFps )  {

    m_Viewport.x        = 0;
    m_Viewport.y        = 0;
    m_Viewport.width    = fWidth;
    m_Viewport.height   = fHeight;
    m_fWidth            = fWidth;
    m_fHeight           = fHeight;
    m_nTargetFps        = nTargetFps;
    m_strTitle          = szTitle;
    m_pTmxMap           = NULL;
    m_bIsStarted        = false;
    m_bWindowResizeable = __DEFAULT_RESIZEABLE_STATUS;
    m_bClearBackground  = __DEFAULT_CLEAR_BACKGROUND;
    m_bDrawFPS          = __DEFAULT_DRAW_FPS_STATUS;
    m_fLineThickness    = __DEFAULT_LINE_THICKNESS;
    m_nScrollStepWidth  = __DEFAULT_SCROLL_STEP_WIDTH;
    m_nScrollStepHeight = __DEFAULT_SCROLL_STEP_HEIGHT;
    m_ViewControlMode   = __DEFAULT_VIEW_CONTROL_MODE;
    m_strTxMapFile.clear();
    memset( &m_CameraPos, 0, sizeof( m_CameraPos ) );
    InitializeZoomEngine();
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
 * Enable/disable draw FPS information on top left corner of screen
 * @param bDrawFPS The new draw FPS status;
 */
void MapRenderer :: SetDrawFPS( bool bDrawFPS )  {

    m_bDrawFPS = bDrawFPS;
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
 * Set window resizeable status.
 * @param bResizeable The new resizeable status for window;
 */
void MapRenderer :: SetWindowResizeable( bool bResizeable )  {

    m_bWindowResizeable = bResizeable;
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
 * Set the new renderer viewport;
 * @param viewport The new viewport rectangle;
 */
void MapRenderer :: SetViewport( Viewport viewport )  {

    m_Viewport = viewport;
}

/**
 * Enable or disable the user zoom mode (false disables user zoom by
 * mouse, keyboard, joystick, touch, ...);
 * @param bEnabled The new user zoom mode;
 */
void MapRenderer :: SetEnableUserZoom( bool bEnabled )  {

    m_bEnabledUserZoom = bEnabled;
}

/**
 * Set the minimum zoom border limit;
 * @param nMinPos The new minimum zoom limit;
 */
void MapRenderer :: SetMinZoom( unsigned nMinPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nMinPos >= limits.min() ) &&  ( nMinPos <= limits.max() ) )
        m_ZoomBorderLimits.first = nMinPos;
}

/**
 * Set the maximum zoom border limit;
 * @param nMaxPos The new maximum zoom limit;
 */
void MapRenderer :: SetMaxZoom( unsigned nMaxPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nMaxPos >= limits.min() ) &&  ( nMaxPos <= limits.max() ) )
        m_ZoomBorderLimits.second = nMaxPos;
}

/**
 * Set the preferred zoom to be used when engine apply
 * reset operations.
 * @param nZoomPos The new preferred zoom;
 */
void MapRenderer :: SetPreferredZoom( unsigned nZoomPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nZoomPos >= limits.min() ) &&  ( nZoomPos <= limits.max() ) )
        m_nPreferredZoomPos = nZoomPos;
}

/**
 * Set zoom programatically.
 * @param nZoomPos The zoom to be applied;
 */
void MapRenderer :: SetZoom( unsigned nZoomPos )  {

    bool  bEnabledUserZoom = m_bEnabledUserZoom;

    m_bEnabledUserZoom = false;

    if( nZoomPos > m_nCurrentZoomPos )
       ZoomIn();
    else
        if( nZoomPos < m_nCurrentZoomPos )
            ZoomOut();

    m_bEnabledUserZoom = bEnabledUserZoom;
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

    if( m_bWindowResizeable )
        SetConfigFlags( FLAG_WINDOW_RESIZABLE );

    InitWindow( ( int ) m_fWidth, ( int ) m_fHeight, m_strTitle.c_str() );

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
