/*
 * maprenderer.cpp
 *
 *  Created on: Jun 17, 2021
 *      Author: popolony2k
 */

#include <memory.h>
#include <cstring>
#include <chrono>
#include <cmath>
#include "maprenderer.h"

/*
 * Engine defaults.
 */
#define __DEFAULT_FPS                   30
#define __DEFAULT_MAP_ZOOM_SCALE_STEP   ( 0.0625f / 2.0 )
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
#define __MAX_OPACITY_LEVEL            0xFF

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
 * Get a pointer to a loaded layer based on it's Id.
 * @param nLayerId The Layer Id to retrieve the layer;
 */
tmx_layer* MapRenderer :: GetLayer( int nLayerId )  {

    return tmx_find_layer_by_id( m_pTmxMap, nLayerId );
}

/**
 * Get a pointer to a loaded layer based on it's Id or name.
 * @param szLayerName The Layer name used to retrieve the layer. If the layer
 * name is NULL parameter nLayerId will be used for searching layer;
 */
tmx_layer* MapRenderer :: GetLayer( const char *szLayerName )  {

    return tmx_find_layer_by_name( m_pTmxMap, szLayerName );
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
 */
bool MapRenderer :: GetClippedArea( int32_t nSourceW,
                                    int32_t nSourceH,
                                    int32_t nDestX,
                                    int32_t nDestY,
                                    float& fViewX,
                                    float& fViewY,
                                    float& fViewWidth,
                                    float& fViewHeight )  {

    float       fClippingX;
    float       fClippingY;
    int32_t     nTemp;

    nDestX+=m_CameraPos.x;
    nDestY+=m_CameraPos.y;

    if( ( nDestX > ( m_Viewport.x + m_Viewport.width ) ) ||
        ( nDestY > ( m_Viewport.y + m_Viewport.height ) )||
        ( nDestX < 0.0 ) || ( nDestY < 0.0 ) )  {

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

/**
 * Draw  pixel according the specified position.
 * @param nCoordX The X coordinate to plot pixel;
 * @param nCoordY The Y coordinate to plot pixel;
 * @param color Color of pixel;
 */
void MapRenderer :: SetPixel( int nCoordX, int nCoordY, Color color )  {

    DrawPixel( nCoordX, nCoordY, color );
}

/**
 * Midpoint ellipse drawing algorithm based on algorithm found at
 * https://www.geeksforgeeks.org/midpoint-ellipse-drawing-algorithm/
 * @param fCoordX Ellipse X coordinate;
 * @param fCoordY Ellipse Y coordinate;
 * @param fRadiusX X radius;
 * @param fRadiusX Y radius;
 * @param color Ellipse color;
 */
void MapRenderer :: MidPointEllipse( double fCoordX,
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
        if( ( nXPos > m_Viewport.x ) && ( nXPos < m_Viewport.width ) &&
            ( nYPos > m_Viewport.y ) && ( nYPos < m_Viewport.height ) )  {
            SetPixel( nXPos, nYPos, color );
        }

        if( ( nXNeg > m_Viewport.x ) && ( nXNeg < m_Viewport.width ) &&
            ( nYPos > m_Viewport.y ) && ( nYPos < m_Viewport.height ) )  {
            SetPixel( nXNeg, nYPos, color );
        }

        if( ( nXPos > m_Viewport.x ) && ( nXPos < m_Viewport.width ) &&
            ( nYNeg > m_Viewport.y ) && ( nYNeg < m_Viewport.height ) )  {
            SetPixel( nXPos, nYNeg, color );
        }

        if( ( nXNeg > m_Viewport.x ) && ( nXNeg < m_Viewport.width ) &&
            ( nYNeg > m_Viewport.y ) && ( nYNeg < m_Viewport.height ) )  {
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
        if( ( nXPos > m_Viewport.x ) && ( nXPos < m_Viewport.width ) &&
            ( nYPos > m_Viewport.y ) && ( nYPos < m_Viewport.height ) )  {
            SetPixel( nXPos, nYPos, color );
        }

        if( ( nXNeg > m_Viewport.x ) && ( nXNeg < m_Viewport.width ) &&
            ( nYPos > m_Viewport.y ) && ( nYPos < m_Viewport.height ) )  {
            SetPixel( nXNeg, nYPos, color );
        }

        if( ( nXPos > m_Viewport.x ) && ( nXPos < m_Viewport.width ) &&
            ( nYNeg > m_Viewport.y ) && ( nYNeg < m_Viewport.height ) )  {
            SetPixel( nXPos, nYNeg, color );
        }

        if( ( nXNeg > m_Viewport.x ) && ( nXNeg < m_Viewport.width ) &&
            ( nYNeg > m_Viewport.y ) && ( nYNeg < m_Viewport.height ) )  {
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
 * Bresenham line generation implementation based on algorithm found at
 * https://gist.github.com/bert/1085538.
 * @param nX0 Initial X line coordinate;
 * @param nY0 Initial Y line coordinate;
 * @param nX1 Final X line coordinate;
 * @param nY1 Final Y line coordinate;
 * @param color line color;
 */
void MapRenderer :: LineBresenham( int nX0,
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
    if( ( nX0 > m_Viewport.x ) && ( nX0 < m_Viewport.width ) &&
        ( nY0 > m_Viewport.y ) && ( nY0 < m_Viewport.height ) )  {
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
void MapRenderer :: DrawPolyline( double fOffset_x,
                                  double fOffset_y,
                                  double **fPoints,
                                  int nPointsCount,
                                  Color color ) {

    fOffset_x = ( ( fOffset_x +
                  m_CameraPos.x ) * m_fZoomFactor ) + m_Viewport.x;
    fOffset_y = ( ( fOffset_y +
                  m_CameraPos.y ) * m_fZoomFactor ) + m_Viewport.y;

    for( int i=1; i < nPointsCount; i++ ) {
        LineBresenham( ( fOffset_x + ( fPoints[i-1][0] * m_fZoomFactor ) ),
                       ( fOffset_y + ( fPoints[i-1][1] * m_fZoomFactor ) ) ,
                       ( fOffset_x + ( fPoints[i][0] * m_fZoomFactor ) ) ,
                       ( fOffset_y + ( fPoints[i][1] * m_fZoomFactor ) ),
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
void MapRenderer :: DrawPolygon( double fOffset_x,
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
        fOffset_x = ( ( fOffset_x +
                      m_CameraPos.x ) * m_fZoomFactor ) + m_Viewport.x;
        fOffset_y = ( ( fOffset_y +
                      m_CameraPos.y ) * m_fZoomFactor ) + m_Viewport.y;

        LineBresenham( ( fOffset_x + ( fPoints[0][0] * m_fZoomFactor ) ),
                       ( fOffset_y + ( fPoints[0][1] * m_fZoomFactor ) ),
                       ( fOffset_x + ( fPoints[nPointsCount-1][0] * m_fZoomFactor ) ),
                       ( fOffset_y + ( fPoints[nPointsCount-1][1] * m_fZoomFactor ) ),
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
    float          fViewEndX;
    float          fViewEndY;

    fViewStartX = ( ( fOffset_x +
                      m_CameraPos.x ) * m_fZoomFactor ) + m_Viewport.x;
    fViewStartY = ( ( fOffset_y +
                      m_CameraPos.y ) * m_fZoomFactor ) + m_Viewport.y;
    fViewEndX   = ( ( fOffset_x +
                      fWidth +
                      m_CameraPos.x ) * m_fZoomFactor ) + m_Viewport.x;
    fViewEndY   = ( ( fOffset_y +
                      fHeight +
                      m_CameraPos.y ) * m_fZoomFactor ) + m_Viewport.y;

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
void MapRenderer :: DrawEllipse( double fOffset_x,
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
    fOffset_x = ( ( fOffset_x +
                    fWidth +
                    m_CameraPos.x ) * m_fZoomFactor ) + m_Viewport.x;
    fOffset_y = ( ( fOffset_y +
                    fHeight +
                    m_CameraPos.y ) * m_fZoomFactor ) + m_Viewport.y;

    MidPointEllipse( fOffset_x,
                     fOffset_y,
                     ( fWidth * m_fZoomFactor ),
                     ( fHeight * m_fZoomFactor ),
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
 * @param pLayer Pointer to layer containing object group to draw;
 */
void MapRenderer :: DrawObjects( tmx_layer *pLayer ) {

    tmx_object *head = pLayer ->  content.objgr -> head;
    Color      color = IntToColor( pLayer ->  content.objgr -> color );

    while( head ) {
        if( head -> visible ) {
            switch( head -> obj_type )  {
                case OT_SQUARE :
                    DrawRectangle( ( head -> x + pLayer -> offsetx ),
                                   ( head -> y + + pLayer -> offsety ),
                                   head -> width,
                                   head -> height,
                                   color );
                    break;

                case OT_POLYGON :
                    DrawPolygon( ( head -> x + + pLayer -> offsetx ),
                                 ( head -> y + + pLayer -> offsety ),
                                 head -> content.shape -> points,
                                 head -> content.shape -> points_len,
                                 color );
                    break;

                case OT_POLYLINE :
                    DrawPolyline( ( head -> x + + pLayer -> offsetx ),
                                  ( head -> y + + pLayer -> offsety ),
                                  head -> content.shape -> points,
                                  head -> content.shape -> points_len,
                                  color );
                    break;

                case OT_ELLIPSE :
                    DrawEllipse( ( head -> x + + pLayer -> offsetx ),
                                 ( head -> y + + pLayer -> offsety ),
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
void MapRenderer :: DrawImageLayer( tmx_layer *pLayer ) {

    Texture2D *pTexture = ( Texture2D * ) pLayer -> content.image -> resource_image;

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
void MapRenderer :: DrawAllLayers( tmx_map *pMap, tmx_layer *pLayer ) {

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
 * Initialize controller event handlers.
 */
void MapRenderer :: InitializeControllerHandlers( void )  {

    for( int nCount = 0; nCount < m_UserEventHandlers.size(); nCount++ )
        m_UserEventHandlers[nCount] = NULL;

    m_UserEventHandlers[KEY_UP]        = &MapRenderer :: MoveCameraUp;
    m_UserEventHandlers[KEY_DOWN]      = &MapRenderer :: MoveCameraDown;
    m_UserEventHandlers[KEY_LEFT]      = &MapRenderer :: MoveCameraLeft;
    m_UserEventHandlers[KEY_RIGHT]     = &MapRenderer :: MoveCameraRight;
    m_UserEventHandlers[KEY_PAGE_UP]   = &MapRenderer :: ZoomIn;
    m_UserEventHandlers[KEY_PAGE_DOWN] = &MapRenderer :: ZoomOut;
    m_UserEventHandlers[KEY_HOME]      = &MapRenderer :: ResetZoom;
    m_UserEventHandlers[KEY_END]       = &MapRenderer :: ResetCamera;
}

/**
 * Get last key from user input selected control.
 */
int MapRenderer :: GetKeyPressed( void )  {

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
 * Check user input selected previously by user (mouse,
 * joystick, keyboard, etc...)
 */
void MapRenderer :: HandleUserInput( void )  {

    int      nKeyPressed = GetKeyPressed();

    if( nKeyPressed < m_UserEventHandlers.size() )  {
        KEY_EVENT_HANDLER pHandler = m_UserEventHandlers[nKeyPressed];

        if( pHandler )  {
            CALL_MEMBER_FN(*this, pHandler )();
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
    m_nScrollStepWidth  = __DEFAULT_SCROLL_STEP_WIDTH;
    m_nScrollStepHeight = __DEFAULT_SCROLL_STEP_HEIGHT;
    m_ViewControlMode   = __DEFAULT_VIEW_CONTROL_MODE;
    m_strTxMapFile.clear();
    memset( &m_CameraPos, 0, sizeof( m_CameraPos ) );
    InitializeZoomEngine();
    InitializeControllerHandlers();
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
 * Reset trhe camera position.
 */
void MapRenderer :: ResetCamera( void )  {

    m_CameraPos.x = 0;
    m_CameraPos.y = 0;
}

/**
 * Move view camera up.
 */
void MapRenderer :: MoveCameraUp( void )  {

    m_CameraPos.y-=m_nScrollStepWidth;
}

/**
 * Move view camera down.
 */
void MapRenderer :: MoveCameraDown( void )  {

    m_CameraPos.y+=m_nScrollStepWidth;
}

/**
 * Move view camera left.
 */
void MapRenderer :: MoveCameraLeft( void )  {

    m_CameraPos.x-=m_nScrollStepWidth;
}

/**
 * Move view camera right.
 */
void MapRenderer :: MoveCameraRight( void )  {

    m_CameraPos.x+=m_nScrollStepWidth;
}

/**
 * Set layer visible status.
 * @param nLayerId The layer id to set visibility status;
 * @param bVisible The new visibility status;
 */
bool MapRenderer :: SetLayerVisible( int nLayerId, bool bVisible )  {

    tmx_layer *pLayer = GetLayer( nLayerId );

    if( pLayer )  {
        pLayer -> visible = bVisible;
        return true;
    }

    return false;
}

/**
 * Set layer visible status.
 * @param szLayerName The layer name to set visibility status;
 * @param bVisible The new visibility status;
 */
bool MapRenderer :: SetLayerVisible( const char *szLayerName, bool bVisible )  {

    tmx_layer *pLayer = GetLayer( szLayerName );

    if( pLayer )  {
        pLayer -> visible = bVisible;
        return true;
    }

    return false;
}

/**
 * Set layer opacity.
 * @param nLayerId The layer id to set opacity;
 * @param nOpacity The new opacity level;
 */
bool MapRenderer :: SetLayerOpacity( int nLayerId, char nOpacity )  {

    tmx_layer *pLayer = GetLayer( nLayerId );

    if( pLayer )  {
        pLayer -> opacity = ( __MAX_OPACITY_LEVEL - nOpacity );
        return true;
    }

    return false;
}

/**
 * Set layer opacity.
 * @param szLayerName The layer name to set visibility status;
 * @param nOpacity The new opacity level;
 */
bool MapRenderer :: SetLayerOpacity( const char *szLayerName, char nOpacity )  {

    tmx_layer *pLayer = GetLayer( szLayerName );

    if( pLayer )  {
        pLayer -> opacity = ( __MAX_OPACITY_LEVEL - nOpacity );
        return true;
    }

    return false;
}

/**
 * Set layer position .
 * @param nLayerId The layer id to set position;
 * @param nPosX new layer X coordinate;
 * @param nPosY new layer Y coordinate;
 */
bool MapRenderer :: SetLayerPosition( int nLayerId,
                                      int nPosX,
                                      int nPosY )  {

    tmx_layer *pLayer = GetLayer( nLayerId );

    if( pLayer )  {
        pLayer -> offsetx = nPosX;
        pLayer -> offsety = nPosY;
        return true;
    }

    return false;
}

/**
 * Set layer position.
 * @param szLayerName The layer name to set layer position;
 * @param nPosX new layer X coordinate;
 * @param nPosY new layer Y coordinate;
 */
bool MapRenderer :: SetLayerPosition( const char *szLayerName,
                                      int nPosX,
                                      int nPosY )  {

    tmx_layer *pLayer = GetLayer( szLayerName );

    if( pLayer )  {
        pLayer -> offsetx = nPosX;
        pLayer -> offsety = nPosY;
        return true;
    }

    return false;
}

/**
 * Get the layer position.
 * @param nLayerId The layer id to get position;
 * @param nPosX Reference variable that will receive X coordinate;
 * @param nPosY Reference variable that will receive Y coordinate;
 */
bool MapRenderer :: GetLayerPosition( int nLayerId, int &nPosX, int &nPosY )  {

    tmx_layer *pLayer = GetLayer( nLayerId );

    if( pLayer )  {
        nPosX = pLayer -> offsetx;
        nPosY = pLayer -> offsety;
        return true;
    }

    return false;

}

/**
 * Get the layer position.
 * @param szLayerName The layer name to set layer position;
 * @param nPosX Reference variable that will receive X coordinate;
 * @param nPosY Reference variable that will receive Y coordinate;
 */
bool MapRenderer :: GetLayerPosition( const char *szLayerName,
                                      int &nPosX,
                                      int &nPosY )  {

    tmx_layer *pLayer = GetLayer( szLayerName );

    if( pLayer )  {
        nPosX = pLayer -> offsetx;
        nPosY = pLayer -> offsety;
        return true;
    }

    return false;
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
