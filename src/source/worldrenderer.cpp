/*
 * worldrenderer.cpp
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#include <memory.h>
#include <cstring>
#include <chrono>
#include <cmath>
#include <algorithm>
#include "worldrenderer.h"

/*
 * Engine defaults.
 */
#define __DEFAULT_FPS                   30
#define __DEFAULT_SCROLL_STEP_WIDTH     -1
#define __DEFAULT_SCROLL_STEP_HEIGHT    -1
#define __DEFAULT_SCROLL_STEP           -1
#define __DEFAULT_CLEAR_BACKGROUND      true
#define __DEFAULT_RESIZEABLE_STATUS     false
#define __DEFAULT_DRAW_FPS_STATUS       false
#define __DEFAULT_VIEW_CONTROL_MODE     VIEW_CONTROL_MODE_ACTIVE
#define __DEFAULT_EXIT_KEY              KEY_ESCAPE

/*
 * Engine limits.
 */
#define __MAX_OPACITY_LEVEL            0xFF

bool WorldRenderer :: m_bInitialized = false;

using namespace std :: chrono;


/**
 * TxmLib texture loader callback implementation.
 * @param szFileName Texture file name;
 */
void* WorldRenderer :: TextureLoaderCallback( const char *szFileName )  {

    Texture2D *pTexture = new Texture2D;

    *pTexture = ::LoadTexture( szFileName );

    return pTexture;
}

/**
 * TxmLib texture deallocation callback implementation.
 * @param pTexture Pointer to the texture that will be deallocated;
 */
void WorldRenderer :: TextureFreeCallback( void *pTexture )  {

    Texture2D    *pTexture2D = ( Texture2D * ) pTexture;

    ::UnloadTexture( *pTexture2D );

    delete pTexture2D;
}

/**
 * Get a pointer to a loaded layer based on it's Id.
 * @param nLayerId The Layer Id to retrieve the layer;
 */
tmx_layer* WorldRenderer :: GetLayer( int nLayerId )  {

    return ::tmx_find_layer_by_id( m_pTmxMap, nLayerId );
}

/**
 * Get a pointer to a loaded layer based on it's Id or name.
 * @param szLayerName The Layer name used to retrieve the layer. If the layer
 * name is NULL parameter nLayerId will be used for searching layer;
 */
tmx_layer* WorldRenderer :: GetLayer( const char *szLayerName )  {

    return ::tmx_find_layer_by_name( m_pTmxMap, szLayerName );
}

/**
 * Convert integer color representation to @link Color object;
 */
Color WorldRenderer :: IntToColor( int color ) {

    tmx_col_bytes res = ::tmx_col_to_bytes( color );

    return *( ( Color * ) &res );
}

/**
 * Draw  pixel according the specified position.
 * @param nCoordX The X coordinate to plot pixel;
 * @param nCoordY The Y coordinate to plot pixel;
 * @param color Color of pixel;
 */
void WorldRenderer :: SetPixel( int nCoordX, int nCoordY, Color color )  {

    ::DrawPixel( nCoordX, nCoordY, color );
}

/**
 * Midpoint ellipse drawing algorithm based on implementation found at
 * https://www.geeksforgeeks.org/midpoint-ellipse-drawing-algorithm/
 * @param fCoordX Ellipse X coordinate;
 * @param fCoordY Ellipse Y coordinate;
 * @param fRadiusX X radius;
 * @param fRadiusX Y radius;
 * @param color Ellipse color;
 */
void WorldRenderer :: MidPointEllipse( double fCoordX,
                                       double fCoordY,
                                       double fRadiusX,
                                       double fRadiusY,
                                       Color color ) {
    double dx, dy, d1, d2, x, y;
    x = 0;
    y = fRadiusY;

    // Initial decision parameter of region 1
    d1 = ( fRadiusY * fRadiusY ) -
         ( fRadiusX * fRadiusX * fRadiusY ) +
         ( 0.25 * fRadiusX * fRadiusX );
    dx = ( 2 * fRadiusX * fRadiusY * x );
    dy = ( 2 * fRadiusX * fRadiusX * y );

    // For region 1
    while( dx < dy )  {
        int nXPos = ( x + fCoordX );
        int nYPos = ( y + fCoordY );
        int nXNeg = ( -x + fCoordX );
        int nYNeg = ( -y + fCoordY );

        // Print points based on 4-way symmetry
        if( ( nXPos > m_Viewport.pos.x ) &&
            ( nXPos < m_Viewport.size.nWidth ) &&
            ( nYPos > m_Viewport.pos.y ) &&
            ( nYPos < m_Viewport.size.nHeight ) )  {
            SetPixel( nXPos, nYPos, color );
        }

        if( ( nXNeg > m_Viewport.pos.x ) &&
            ( nXNeg < m_Viewport.size.nWidth ) &&
            ( nYPos > m_Viewport.pos.y ) &&
            ( nYPos < m_Viewport.size.nHeight ) )  {
            SetPixel( nXNeg, nYPos, color );
        }

        if( ( nXPos > m_Viewport.pos.x ) &&
            ( nXPos < m_Viewport.size.nWidth ) &&
            ( nYNeg > m_Viewport.pos.y ) &&
            ( nYNeg < m_Viewport.size.nHeight ) )  {
            SetPixel( nXPos, nYNeg, color );
        }

        if( ( nXNeg > m_Viewport.pos.x ) &&
            ( nXNeg < m_Viewport.size.nWidth ) &&
            ( nYNeg > m_Viewport.pos.y ) &&
            ( nYNeg < m_Viewport.size.nHeight ) )  {
            SetPixel( nXNeg, nYNeg, color );
        }

        // Checking and updating value of
        // decision parameter based on algorithm
        if( d1 < 0 )  {
            x++;
            dx = ( dx + (2 * fRadiusY * fRadiusY ) );
            d1 = ( d1 + dx + ( fRadiusY * fRadiusY ) );
        }
        else  {
            x++;
            y--;
            dx = ( dx + ( 2 * fRadiusY * fRadiusY ) );
            dy = ( dy - ( 2 * fRadiusX * fRadiusX ) );
            d1 = ( d1 + dx - dy + ( fRadiusY * fRadiusY ) );
        }
    }

    // Decision parameter of region 2
    d2 = ( ( fRadiusY * fRadiusY ) * ( ( x + 0.5 ) * ( x + 0.5 ) ) ) +
         ( ( fRadiusX * fRadiusX ) * ( ( y - 1 ) * ( y - 1 ) ) ) -
           ( fRadiusX * fRadiusX * fRadiusY * fRadiusY );

    // Plotting points of region 2
    while( y >= 0 ) {

        int nXPos = ( x + fCoordX );
        int nYPos = ( y + fCoordY );
        int nXNeg = ( -x + fCoordX );
        int nYNeg = ( -y + fCoordY );

        // Print points based on 4-way symmetry
        if( ( nXPos > m_Viewport.pos.x ) &&
            ( nXPos < m_Viewport.size.nWidth ) &&
            ( nYPos > m_Viewport.pos.y ) &&
            ( nYPos < m_Viewport.size.nHeight ) )  {
            SetPixel( nXPos, nYPos, color );
        }

        if( ( nXNeg > m_Viewport.pos.x ) &&
            ( nXNeg < m_Viewport.size.nWidth ) &&
            ( nYPos > m_Viewport.pos.y ) &&
            ( nYPos < m_Viewport.size.nHeight ) )  {
            SetPixel( nXNeg, nYPos, color );
        }

        if( ( nXPos > m_Viewport.pos.x ) &&
            ( nXPos < m_Viewport.size.nWidth ) &&
            ( nYNeg > m_Viewport.pos.y ) &&
            ( nYNeg < m_Viewport.size.nHeight ) )  {
            SetPixel( nXPos, nYNeg, color );
        }

        if( ( nXNeg > m_Viewport.pos.x ) &&
            ( nXNeg < m_Viewport.size.nWidth ) &&
            ( nYNeg > m_Viewport.pos.y ) &&
            ( nYNeg < m_Viewport.size.nHeight ) )  {
            SetPixel( nXNeg, nYNeg, color );
        }

        // Checking and updating parameter
        // value based on algorithm
        if( d2 > 0 ) {
            y--;
            dy = ( dy - ( 2 * fRadiusX * fRadiusX ) );
            d2 = ( d2 + ( fRadiusX * fRadiusX ) - dy );
        }
        else  {
            y--;
            x++;
            dx = ( dx + ( 2 * fRadiusY * fRadiusY ) );
            dy = ( dy - ( 2 * fRadiusX * fRadiusX ) );
            d2 = ( d2 + dx - dy + ( fRadiusX * fRadiusX ) );
        }
    }
}

/**
 * Bresenham line generation algorithm based on implementation found at
 * https://gist.github.com/bert/1085538.
 * @param nX0 Initial X line coordinate;
 * @param nY0 Initial Y line coordinate;
 * @param nX1 Final X line coordinate;
 * @param nY1 Final Y line coordinate;
 * @param color line color;
 */
void WorldRenderer :: LineBresenham( int nX0,
                                     int nY0,
                                     int nX1,
                                     int nY1,
                                     Color color )  {

  int   nDx  = std :: abs( nX1 - nX0 );
  int   nSx  = ( nX0 < nX1 ? 1 : -1 );
  int   nDy  = -std :: abs( nY1 - nY0 );
  int   nSy  = ( nY0 < nY1 ? 1 : -1 );
  int   nErr = nDx + nDy;
  int   nE2; /* error value e_xy */

  while( true )  {

    // Print points based on 4-way symmetry
    if( ( nX0 > m_Viewport.pos.x ) &&
        ( nX0 < m_Viewport.size.nWidth ) &&
        ( nY0 > m_Viewport.pos.y ) &&
        ( nY0 < m_Viewport.size.nHeight ) )  {
        SetPixel( nX0, nY0, color );
    }

    if( ( nX0 == nX1 ) && ( nY0 == nY1 ) )
        break;

    nE2 = ( 2 * nErr );

    if( nE2 >= nDy ) {
        nErr+=nDy;
        nX0+=nSx;
    } /* e_xy+e_x > 0 */

    if( nE2 <= nDx ) {
        nErr+=nDx;
        nY0+=nSy;
    } /* e_xy+e_y < 0 */
  }
}

/**
 * Draw a polygon line (without close the polygon).
 * @param fOffset_x X polygon coordinate;
 * @param fOffset_y Y polygon coordinate;
 * @param points array of points for this polygon;
 * @param points_count Number of items of points array;
 */
void WorldRenderer :: DrawPolyline( double fOffset_x,
                                    double fOffset_y,
                                    double **fPoints,
                                    int nPointsCount,
                                    Color color ) {

    fOffset_x = ( ( fOffset_x + m_CameraPos.x ) *
                  m_pProps -> fZoomFactor ) + m_Viewport.pos.x;
    fOffset_y = ( ( fOffset_y + m_CameraPos.y ) *
                  m_pProps -> fZoomFactor ) + m_Viewport.pos.y;

    for( int i=1; i < nPointsCount; i++ ) {
        LineBresenham( ( fOffset_x + ( fPoints[i-1][0] *
                                       m_pProps -> fZoomFactor ) ),
                       ( fOffset_y + ( fPoints[i-1][1] *
                                       m_pProps -> fZoomFactor ) ) ,
                       ( fOffset_x + ( fPoints[i][0] *
                                       m_pProps -> fZoomFactor ) ) ,
                       ( fOffset_y + ( fPoints[i][1] *
                                       m_pProps -> fZoomFactor ) ),
                       color );
    }
}

/**
 * Draw a polygon line (without close the polygon).
 * @param fOffset_x X polygon coordinate;
 * @param fOffset_y Y polygon coordinate;
 * @param points array of points for this polygon;
 * @param points_count Number of items of points array;
 */
void WorldRenderer :: DrawPolygon( double fOffset_x,
                                   double fOffset_y,
                                   double **fPoints,
                                   int nPointsCount,
                                   Color color ) {

    DrawPolyline( fOffset_x,
                  fOffset_y,
                  fPoints,
                  nPointsCount,
                  color );

    if( nPointsCount > 2 ) {
        fOffset_x = ( ( fOffset_x + m_CameraPos.x ) *
                      m_pProps -> fZoomFactor ) + m_Viewport.pos.x;
        fOffset_y = ( ( fOffset_y + m_CameraPos.y ) *
                      m_pProps -> fZoomFactor ) + m_Viewport.pos.y;

        LineBresenham( ( fOffset_x + ( fPoints[0][0] *
                                       m_pProps -> fZoomFactor ) ),
                       ( fOffset_y + ( fPoints[0][1] *
                                       m_pProps -> fZoomFactor ) ),
                       ( fOffset_x + ( fPoints[nPointsCount-1][0] *
                                       m_pProps -> fZoomFactor ) ),
                       ( fOffset_y + ( fPoints[nPointsCount-1][1] *
                                       m_pProps -> fZoomFactor ) ),
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
void WorldRenderer :: DrawRectangle( double fOffset_x,
                                     double fOffset_y,
                                     double fWidth,
                                     double fHeight,
                                     Color color )  {

    float          fViewStartX;
    float          fViewStartY;
    float          fViewEndX;
    float          fViewEndY;

    fViewStartX = ( ( fOffset_x + m_CameraPos.x ) *
                    m_pProps -> fZoomFactor ) + m_Viewport.pos.x;
    fViewStartY = ( ( fOffset_y + m_CameraPos.y ) *
                    m_pProps -> fZoomFactor ) + m_Viewport.pos.y;
    fViewEndX   = ( ( fOffset_x + fWidth + m_CameraPos.x ) *
                    m_pProps -> fZoomFactor ) + m_Viewport.pos.x;
    fViewEndY   = ( ( fOffset_y + fHeight + m_CameraPos.y ) *
                    m_pProps -> fZoomFactor ) + m_Viewport.pos.y;

    // Top line
    LineBresenham( fViewStartX,
                   fViewStartY,
                   fViewEndX,
                   fViewStartY,
                   color );
    // Bottom line
    LineBresenham( fViewStartX,
                   fViewEndY,
                   fViewEndX,
                   fViewEndY,
                   color );

    // Left line
    LineBresenham( fViewStartX,
                   fViewStartY,
                   fViewStartX,
                   fViewEndY,
                   color );

    // Right line
    LineBresenham( fViewEndX,
                   fViewStartY,
                   fViewEndX,
                   fViewEndY,
                   color );
}

/**
 * Draw an ellipse primitive to specified position on texture.
 * @param fOffset_x X ellipse coordinate;
 * @param fOffset_y Y ellipse coordinate;
 * @param fWidth The ellipse width;
 * @param fHeight The ellipse height;
 * @param color ellipse color;
 */
void WorldRenderer :: DrawEllipse( double fOffset_x,
                                   double fOffset_y,
                                   double fWidth,
                                   double fHeight,
                                   Color color )  {

    float          fViewStartX;
    float          fViewStartY;
    float          fClippedWidth;
    float          fClippedHeight;

    fWidth-=( fWidth / 2.0 );
    fHeight-=( fHeight / 2.0 );
    fOffset_x = ( ( fOffset_x + fWidth + m_CameraPos.x ) *
                  m_pProps -> fZoomFactor ) + m_Viewport.pos.x;
    fOffset_y = ( ( fOffset_y + fHeight + m_CameraPos.y ) *
                  m_pProps -> fZoomFactor ) + m_Viewport.pos.y;

    MidPointEllipse( fOffset_x,
                     fOffset_y,
                     ( fWidth * m_pProps -> fZoomFactor ),
                     ( fHeight * m_pProps -> fZoomFactor ),
                     color );
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
void WorldRenderer :: DrawTile( void *pImage,
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

    nDestX+=m_CameraPos.x;
    nDestY+=m_CameraPos.y;

    if( GetClippedArea( nSourceW, nSourceH,
                        nDestX, nDestY,
                        fViewX, fViewY,
                        fClippedWidth, fClippedHeight ) ) {
        ::DrawTextureTiled( *pTexture,
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
                          m_pProps -> fZoomFactor,
                          ( Color ) { op, op, op, op } );
    }
}

/**
 * Draw objects on canvas;
 * @param pLayer Pointer to layer containing object group to draw;
 */
void WorldRenderer :: DrawObjects( tmx_layer *pLayer ) {

    tmx_object *head = pLayer ->  content.objgr -> head;
    Color      color = IntToColor( pLayer ->  content.objgr -> color );

    while( head ) {
        if( head -> visible ) {
            switch( head -> obj_type )  {
                case OT_SQUARE :
                    DrawRectangle( ( head -> x + pLayer -> offsetx ),
                                   ( head -> y + pLayer -> offsety ),
                                   head -> width,
                                   head -> height,
                                   color );
                    break;

                case OT_POLYGON :
                    DrawPolygon( ( head -> x + pLayer -> offsetx ),
                                 ( head -> y + pLayer -> offsety ),
                                 head -> content.shape -> points,
                                 head -> content.shape -> points_len,
                                 color );
                    break;

                case OT_POLYLINE :
                    DrawPolyline( ( head -> x + pLayer -> offsetx ),
                                  ( head -> y + pLayer -> offsety ),
                                  head -> content.shape -> points,
                                  head -> content.shape -> points_len,
                                  color );
                    break;

                case OT_ELLIPSE :
                    DrawEllipse( ( head -> x + pLayer -> offsetx ),
                                 ( head -> y + pLayer -> offsety ),
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
 * @param pImage Pointer to layer containing image to draw;
 */
void WorldRenderer :: DrawImageLayer( tmx_layer *pLayer ) {

    Texture2D *pTexture = ( Texture2D * ) pLayer -> content.image -> resource_image;

    DrawTexture( *pTexture, 0, 0, WHITE );
}

/**
 * Draw layer on screen canvas;
 * @param pMap Pointer to layer map;
 * @param pLayer Pointer to layer with objects to draw;
 */
void WorldRenderer :: DrawLayer( tmx_map *pMap, tmx_layer *pLayer ) {

    float         fOpacity = pLayer -> opacity;


    for( unsigned long i = 0; i < pMap -> height; i++ ) {
        for( unsigned long j = 0; j < pMap -> width; j++ ) {
            stTile            tile;
            stMatrixPosition  pos   = { ( int ) i, ( int ) j };
            stLayer           layer = { false, 0, {0, 0}, pLayer };

            if( GetTile( pos, layer, tile ) )  {
                tmx_tile       *pTile = tile.pTile;
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

                    if( pAnimInfo -> nMillis <= nMillis )  {
                        unsigned int     nNextFrmGID;
                        _tmx_frame       *pTmxAnimFrm;

                        if( pAnimInfo -> nCounter < pTile -> animation_len )  {
                            pTmxAnimFrm = &pTile -> animation[pAnimInfo -> nCounter];
                            pAnimInfo -> nCounter++;
                        }
                        else  {
                            pAnimInfo -> nCounter = 0;
                            pTmxAnimFrm = &pTile -> animation[0];
                        }

                        nNextFrmGID = ( pMap -> ts_head -> firstgid +
                                        pTmxAnimFrm -> tile_id );
                        pAnimInfo -> pNextTile = pMap -> tiles[nNextFrmGID];
                        pAnimInfo -> nMillis   = ( pTmxAnimFrm -> duration +
                                                   nMillis );
                        pTile = pAnimInfo -> pNextTile;

                        if( !pTile )
                            pTile = pMap -> tiles[tile.nGID];
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
                          ( ( j * pTs -> tile_width ) + pLayer -> offsetx ),
                          ( ( i * pTs -> tile_height ) + pLayer -> offsety ),
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
void WorldRenderer :: DrawAllLayers( tmx_map *pMap, tmx_layer *pLayer ) {

    while( pLayer ) {
        if( pLayer -> visible ) {
            switch( pLayer -> type )  {
                case L_GROUP :
                    DrawAllLayers( pMap, pLayer -> content.group_head ); // recursive call
                    break;
                case L_OBJGR :
                    DrawObjects( pLayer );
                    break;
                case L_IMAGE :
                    DrawImageLayer( pLayer );
                    break;
                case L_LAYER :
                    DrawLayer( pMap, pLayer );
                    break;
            }
        }

        pLayer = pLayer -> next;
    }
}

/**
 * Render all map objects.
 */
void WorldRenderer :: RenderMap( void ) {

    if( m_bClearBackground )
        ClearBackground( IntToColor( m_pTmxMap -> backgroundcolor ) );

    DrawAllLayers( m_pTmxMap, m_pTmxMap -> ly_head );

    if( m_bDrawFPS )
        DrawFPS( 0,  0 );
}

/**
 * Unload a previously loaded map and it's related data (animations, etc...);
 */
bool WorldRenderer :: UnloadMap( void )  {

    if( m_pTmxMap )  {
        ::tmx_map_free( m_pTmxMap );

        /*
         * Release all allocated animations data structure.
         */
        __AnimInfoList :: iterator itItem = m_AnimInfoList.begin();

        while( itItem != m_AnimInfoList.end() )  {
            delete *itItem;
            itItem = m_AnimInfoList.erase( itItem );
        }

        m_pTmxMap    = NULL;
        m_nMapWidth  = 0;
        m_nMapHeight = 0;

        return true;
    }

    return false;
}

/**
 * Initialize controller event handlers.
 */
void WorldRenderer :: InitializeControllerHandlers( void )  {

    for( int nCount = 0; nCount < m_UserEventHandlers.size(); nCount++ )
        m_UserEventHandlers[nCount] = NULL;

    m_UserEventHandlers[KEY_UP]        = &WorldRenderer :: MoveCameraUp;
    m_UserEventHandlers[KEY_DOWN]      = &WorldRenderer :: MoveCameraDown;
    m_UserEventHandlers[KEY_LEFT]      = &WorldRenderer :: MoveCameraLeft;
    m_UserEventHandlers[KEY_RIGHT]     = &WorldRenderer :: MoveCameraRight;
    m_UserEventHandlers[KEY_PAGE_UP]   = &WorldRenderer :: ZoomIn;
    m_UserEventHandlers[KEY_PAGE_DOWN] = &WorldRenderer :: ZoomOut;
    m_UserEventHandlers[KEY_HOME]      = &WorldRenderer :: ResetZoom;
    m_UserEventHandlers[KEY_END]       = &WorldRenderer :: ResetCamera;
}

/**
 * Check user input selected previously by user (mouse,
 * joystick, keyboard, etc...)
 */
void WorldRenderer :: HandleUserInput( void )  {

    int      nKeyPressed = GetKeyPressed();

    if( nKeyPressed < m_UserEventHandlers.size() )  {
        KEY_EVENT_HANDLER pHandler = m_UserEventHandlers[nKeyPressed];

        if( pHandler )  {
            CALL_MEMBER_FN(*this, pHandler )();
        }
    }
}

/**
 * Handle user updates from user listeners;
 */
void WorldRenderer :: HandleUserUpdate( void )  {

    for( IWorldListener* pListener : m_WorldListenerList )  {
        pListener -> OnUpdate( *this );
    }
}

/**
 * Handle user collisions;
 */
void WorldRenderer :: HandleUserCollisions( void )  {

    m_CollisionManager.Update();
}

/**
 * Copy user layer to tmx layer.
 * @param pTmxLayer Pointer to Tmx that data will be copied to;
 * @param layer User struct whose layer data will be copied from;
 */
void WorldRenderer :: CopyLayerToTmx( tmx_layer *pTmxLayer, stLayer& layer )  {

    pTmxLayer -> opacity = ( __MAX_OPACITY_LEVEL - layer.nOpacity );
    pTmxLayer -> offsetx = layer.offset.x;
    pTmxLayer -> offsety = layer.offset.y;
    pTmxLayer -> visible = layer.bVisible;
    layer.pLayer         = pTmxLayer;
}

/**
 * Copy tmx layer to user layer.
 * @param pTmxLayer Reference to user layer that data will be copied to;
 * @param layer Pointer whose tmx layer data will be copied from;
 */
void WorldRenderer :: CopyTmxToLayer( stLayer& layer, tmx_layer *pTmxLayer )  {

    layer.nOpacity = ( pTmxLayer -> opacity + __MAX_OPACITY_LEVEL );
    layer.offset.x = pTmxLayer -> offsetx;
    layer.offset.y = pTmxLayer -> offsety;
    layer.bVisible = pTmxLayer -> visible;
    layer.pLayer         = pTmxLayer;
}

/**
 * Initialize all class data.
 * @param fWidth Screen renderer width;
 * @param fHeight Screen renderer height;
 * @param szTitle Screen renderer title;
 * @param nTargetFps Renderer desired FPS;
 * @param bUseDefaultKEyHandler Inform if the default keyboard and
 * camera handler will be used (at the end this parameter will be remove
 * when user interaction be a little bit more clear.
 */
WorldRenderer :: WorldRenderer( float fWidth,
                                float fHeight,
                                const char *szTitle,
                                int nTargetFps,
                                bool bUseDefaultKEyHandler) :
                                m_CollisionManager( this )  {

    m_nMapWidth             = 0;
    m_nMapHeight            = 0;
    m_Viewport.pos.x        = 0;
    m_Viewport.pos.y        = 0;
    m_Viewport.size.nWidth  = fWidth;
    m_Viewport.size.nHeight = fHeight;
    m_fWindowWidth          = fWidth;
    m_fWindowHeight         = fHeight;
    m_nTargetFps            = nTargetFps;
    m_strTitle              = szTitle;
    m_pTmxMap               = NULL;
    m_bIsStarted            = false;
    m_bWindowResizeable     = __DEFAULT_RESIZEABLE_STATUS;
    m_bClearBackground      = __DEFAULT_CLEAR_BACKGROUND;
    m_bDrawFPS              = __DEFAULT_DRAW_FPS_STATUS;
    m_nScrollStepWidth      = __DEFAULT_SCROLL_STEP_WIDTH;
    m_nScrollStepHeight     = __DEFAULT_SCROLL_STEP_HEIGHT;
    m_ViewControlMode       = __DEFAULT_VIEW_CONTROL_MODE;
    m_strTxMapFile.clear();
    m_WorldListenerList.clear();
    memset( &m_CameraPos, 0, sizeof( m_CameraPos ) );
    ResetZoom();

    if( bUseDefaultKEyHandler )  {
        InitializeControllerHandlers();
    }
    else  {
        memset( &m_UserEventHandlers, 0, sizeof( m_UserEventHandlers ) );
    }

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
WorldRenderer :: ~WorldRenderer( void )  {

    UnloadMap();
    m_WorldListenerList.clear();
}

/**
 * Add a World listener to internal listener list renderer.
 * All listeners will be called for each update step;
 * @param pListener Pointer to the @IWorldListener object to add;
 */
void WorldRenderer :: AddWorldListener( IWorldListener *pListener )  {

    WorldListenerList :: iterator itItem = std :: find( m_WorldListenerList.begin(),
                                                        m_WorldListenerList.end(),
                                                        pListener );

    if( itItem == m_WorldListenerList.end() )
        m_WorldListenerList.push_back( pListener );
}

/**
 * Remove a World listener from internal listener list renderer.
 * @param pListener Pointer to the @IWorldListener object to remove;
 */
void WorldRenderer :: RemoveWorldListener( IWorldListener *pListener )  {

    WorldListenerList :: iterator itItem = std :: find( m_WorldListenerList.begin(),
                                                        m_WorldListenerList.end(),
                                                        pListener );

    if( itItem != m_WorldListenerList.end() )
        m_WorldListenerList.erase( itItem );
}

/**
 * Set exit key to leave the renderer when it is in running state;
 * @param key The key code representing the exit key (check raylib
 * KeyboardKey enum);
 * The default exit is ESC Key;
 */
void WorldRenderer :: SetExitKey( KeyboardKey key )  {

    ::SetExitKey( key );
}

/**
 * Set window resizeable status.
 * @param bResizeable The new resizeable status for window;
 */
void WorldRenderer :: SetWindowResizeable( bool bResizeable )  {

    m_bWindowResizeable = bResizeable;
}

/**
 * Set the status of background cleaning for each
 * rendering cycle;
 * @param bStatus The new background clear status settings;
 */
void WorldRenderer :: SetClearBackground( bool bStatus )  {

    m_bClearBackground = bStatus;
}

/**
 * Enable/disable draw FPS information on top left corner of screen
 * @param bDrawFPS The new draw FPS status;
 */
void WorldRenderer :: SetDrawFPS( bool bDrawFPS )  {

    m_bDrawFPS = bDrawFPS;
}

/**
 * Set the view port control mode;
 * Viewport control mode (active and reactive)
 * Active, the view port reacts to a single key pressing continuously;
 * Reactive, the view port reacts only for each key pressing;
 * @param mode The new view control mode;
 */
void WorldRenderer :: SetViewControlMode( ViewControlMode mode )  {

    m_ViewControlMode = mode;
}

/**
 * Set the default scroll step size.
 * @param nStepWidth The new scroll step size Width
 * (-1 uses the map tile size width);
 * @param nStepHeight The new scroll step size Height
 * (-1 uses the map tile size height);
 */
void WorldRenderer :: SetScrollStepSize( int nStepWidth, int nStepHeight )  {

    m_nScrollStepWidth  = nStepWidth;
    m_nScrollStepHeight = nStepHeight;
}

/**
 * Get last key from user input selected control.
 */
int WorldRenderer :: GetKeyPressed( void )  {

    switch( m_ViewControlMode )  {
        case VIEW_CONTROL_MODE_REACTIVE :
            return ::GetKeyPressed();

        case VIEW_CONTROL_MODE_ACTIVE :
            if( ::IsKeyDown( KEY_UP ) )
                return KEY_UP;
            else
            if( ::IsKeyDown( KEY_DOWN ) )
                return KEY_DOWN;
            else
            if( ::IsKeyDown( KEY_LEFT ) )
                return KEY_LEFT;
            else
            if( ::IsKeyDown( KEY_RIGHT ) )
                return KEY_RIGHT;
            else
            if( ::IsKeyDown( KEY_PAGE_UP ) )
                return KEY_PAGE_UP;
            else
            if( ::IsKeyDown( KEY_PAGE_DOWN ) )
                return KEY_PAGE_DOWN;
            else
            if( ::IsKeyDown( KEY_HOME ) )
                return KEY_HOME;
            else
            if( ::IsKeyDown( KEY_END ) )
                    return KEY_END;
    }

    return KEY_NULL;
}

/**
 * Reset the camera position.
 */
void WorldRenderer :: ResetCamera( void )  {

    m_CameraPos.x = 0;
    m_CameraPos.y = 0;
}

/**
 * Move view camera up.
 */
void WorldRenderer :: MoveCameraUp( void )  {

    m_CameraPos.y-=m_nScrollStepHeight;
}

/**
 * Move view camera down.
 */
void WorldRenderer :: MoveCameraDown( void )  {

    m_CameraPos.y+=m_nScrollStepHeight;
}

/**
 * Move view camera left.
 */
void WorldRenderer :: MoveCameraLeft( void )  {

    m_CameraPos.x-=m_nScrollStepWidth;
}

/**
 * Move view camera right.
 */
void WorldRenderer :: MoveCameraRight( void )  {

    m_CameraPos.x+=m_nScrollStepWidth;
}

/**
 * Set layer parameters.
 * @param nLayerId The layer id to set layer parameters;
 * @param layer reference to layer parameters structure to set;
 */
bool WorldRenderer :: SetLayer( int nLayerId, stLayer &layer )  {

    tmx_layer *pTmxLayer = GetLayer( nLayerId );

    if( pTmxLayer )  {
        CopyLayerToTmx( pTmxLayer, layer );
        return true;
    }

    return false;
}

/**
 * Set layer parameters.
 * @param szLayerName The layer name to set layer parameters;
 * @param layer reference to layer parameters structure to set;
 */
bool WorldRenderer :: SetLayer( const char *szLayerName, stLayer &layer )  {

    tmx_layer *pTmxLayer = GetLayer( szLayerName );

    if( pTmxLayer )  {
        CopyLayerToTmx( pTmxLayer, layer );
        return true;
    }

    return false;
}

/**
 * Get the layer parameters.
 * @param nLayerId The layer id to get layer parameters;
 * @param layer reference to layer parameters structure to get;
 */
bool WorldRenderer :: GetLayer( int nLayerId, stLayer &layer )  {

    tmx_layer *pTmxLayer = GetLayer( nLayerId );

    if( pTmxLayer )  {
        CopyTmxToLayer( layer, pTmxLayer );
        return true;
    }

    return false;
}

/**
 * Get the layer parameters.
 * @param szLayerName The layer name to get layer parameters;
 * @param layer reference to layer parameters structure to get;
 */
bool WorldRenderer :: GetLayer( const char *szLayerName, stLayer &layer )  {

    tmx_layer *pTmxLayer = GetLayer( szLayerName );

    if( pTmxLayer )  {
        CopyTmxToLayer( layer, pTmxLayer );
        return true;
    }

    return false;
}

/**
 * Get a tile based row and column on specified map layer;
 * @param pos The tile position on layer map.
 * @param layer Reference to the layer whose tile will be
 * retrieved;
 * @param tile reference to @link stTile object to receive
 * tile information;
 */
bool WorldRenderer :: GetTile( const stMatrixPosition& pos,
                               const stLayer& layer,
                               stTile& tile ) {

    tile.nGID = layer.pLayer -> content.gids[( pos.nTileRow *
                                               m_pTmxMap -> width ) +
                                               pos.nTileCol] &
                                               TMX_FLIP_BITS_REMOVAL;
    tile.pTile = m_pTmxMap -> tiles[tile.nGID];

    if( !tile.pTile )  {
        tile.nGID  = 0;
        tile.pTile = NULL;

        return false;
    }

    return true;
}

/**
 * Convert world coordinate to tile matrix coordinate.
 * @param coord The World coordinate to translate to tile matrix coordinate;
 * @param pos Reference to struct @link stMatrixPosition to receive the
 * tile matrix position based on world coordinate passed as parameter;
 */
bool WorldRenderer :: WorldToTileMatrix( const stCoordinate2D& coord,
                                         stMatrixPosition& pos )  {

    if( m_pTmxMap )  {
        int     nCoordX = ( coord.x / m_pProps -> fZoomFactor );
        int     nCoordY = ( coord.y / m_pProps -> fZoomFactor );

        if( ( ( nCoordX >= 0 ) && ( nCoordX < m_nMapWidth ) ) &&
            ( ( nCoordY >= 0 ) && ( nCoordY < m_nMapHeight ) ) ) {
            pos.nTileCol = ( coord.x / m_pTmxMap -> tile_width );
            pos.nTileRow = ( coord.y / m_pTmxMap -> tile_height );
            return true;
        }
    }

    return false;
}

/**
 * Set the TMX map file to engine load on start.
 * @param szTmxMapFile Renderer map file;
 */
void WorldRenderer :: SetMapFile( const char *szTmxMapFile )  {

    m_strTxMapFile = szTmxMapFile;

    return;
}

/**
 * Get the current map information data.
 * @param mapInfo Reference to the struct @link stMapInfo that will
 * receive the map information.
 */
bool WorldRenderer :: GetMapInfo( stMapInfo& mapInfo )  {

    if( m_pTmxMap )  {
        mapInfo.mapSize.nWidth   = m_pTmxMap -> width;
        mapInfo.mapSize.nHeight  = m_pTmxMap -> height;
        mapInfo.tileSize.nWidth  = m_pTmxMap -> tile_width;
        mapInfo.tileSize.nHeight = m_pTmxMap -> tile_height;
        mapInfo.pMap = m_pTmxMap;

        return true;
    }

    return false;
}

/**
 * Return the reference to internal renderer collision manager.
 */
CollisionManager& WorldRenderer :: GetCollisionManager( void )  {

    return m_CollisionManager;
}

/**
 * Start engine renderer.
 */
bool WorldRenderer :: Start( void )  {

    if( m_bWindowResizeable )
        SetConfigFlags( FLAG_WINDOW_RESIZABLE );

    InitWindow( ( int ) m_fWindowWidth,
                ( int ) m_fWindowHeight,
                m_strTitle.c_str() );

    if( !IsWindowReady() ) {
        ::tmx_perror( "Cannot create a window" );
        return false;
    }

    SetExitKey( __DEFAULT_EXIT_KEY );
    SetTargetFPS( m_nTargetFps != -1 ? m_nTargetFps : __DEFAULT_FPS );
    m_pTmxMap = ::tmx_load( m_strTxMapFile.c_str() );

    if( !m_pTmxMap ) {
        ::tmx_perror( "Cannot load map" );
        return false;
    }

    m_nMapWidth  = ( m_pTmxMap -> width * m_pTmxMap -> tile_width );
    m_nMapHeight = ( m_pTmxMap -> height * m_pTmxMap -> tile_height );

    /*
     * Set scrolling properties.
     */
    SetScrollStepSize( ( m_nScrollStepWidth < 0 ? m_pTmxMap -> tile_width :
                                                  m_nScrollStepWidth ),
                       ( m_nScrollStepHeight < 0 ? m_pTmxMap -> tile_height :
                                                   m_nScrollStepHeight ) );

    m_bIsStarted = ( m_pTmxMap != NULL );

    return m_bIsStarted;
}

/**
 * Stop renderer freeing all allocated resources.
 */
void WorldRenderer :: Stop( void )  {

    if( m_bIsStarted )  {
        UnloadMap();
        CloseWindow();
        m_bIsStarted = false;
    }
}

/**
 * Run renderer.
 */
bool WorldRenderer :: Run( void )  {

    if( m_bIsStarted )  {
        while ( !WindowShouldClose() ) {
            BeginDrawing();
                RenderMap();
                HandleUserInput();
                HandleUserUpdate();
                HandleUserCollisions();
            EndDrawing();
        }

        return true;
    }

    return false;
}
