/*
 * viewport.cpp
 *
 *  Created on: Jul 13, 2021
 *      Author: popolony2k
 */

#include "viewport.h"
#include <algorithm>


/*
 * Zoom defaults.
 */
#define __DEFAULT_MAP_ZOOM_SCALE_STEP   0.0625f
#define __DEFAULT_PREFERRED_ZOOM_POS    ( ( unsigned )( ( 1 / __DEFAULT_MAP_ZOOM_SCALE_STEP ) - 1 ) )
#define __MAX_ZOOM_DEPTH                256
#define __DEFAULT_USER_ZOOM_STATUS      true


/**
 * Initialize the zoom engine.
 */
void Viewport :: InitializeZoomEngine( void )  {

    float   fZoomStep = 0.0;

    /*
     * Fill all zoom factor list.
     */
    for( int nCount = 0; nCount < __MAX_ZOOM_DEPTH; nCount++ )  {
        m_vZoomFactorList.push_back(fZoomStep+=__DEFAULT_MAP_ZOOM_SCALE_STEP );
    }

    m_pProps = &m_Props;

    SetPreferredZoom( __DEFAULT_PREFERRED_ZOOM_POS );

    m_ZoomBorderLimits.first     = 0;
    m_ZoomBorderLimits.second    = __MAX_ZOOM_DEPTH;
    m_pProps -> bEnabledUserZoom = __DEFAULT_USER_ZOOM_STATUS;
    m_pProps -> fZoomFactor      = m_vZoomFactorList[m_pProps -> nPreferredZoomPos];
    m_Viewport.pos.x             = 0;
    m_Viewport.pos.y             = 0;
    m_Viewport.size.nWidth       = 0;
    m_Viewport.size.nHeight      = 0;

}

/**
 * Constructor. Initialize all class data.
 */
Viewport :: Viewport( void )  {

    InitializeZoomEngine();
}

/**
 * Destructor. Finalize all class data.
 */
Viewport :: ~Viewport( void )  {

}

/**
 * Set the new object viewport;
 * @param viewport The new viewport rectangle;
 */
void Viewport :: SetViewport( stViewport viewport )  {

    m_Viewport = viewport;
}

/**
 * Return the object viewport information structure.
 */
stViewport Viewport :: GetViewport( void )  {

    return m_Viewport;
}

/**
 * Enable or disable the user zoom mode (false disables user zoom by
 * mouse, keyboard, joystick, touch, ...);
 * @param bEnabled The new user zoom mode;
 */
void Viewport :: SetEnableUserZoom( bool bEnabled )  {

    m_pProps -> bEnabledUserZoom = bEnabled;
}

/**
 * Set the preferred zoom to be used when engine apply
 * reset operations.
 * @param nZoomPos The new preferred zoom;
 */
void Viewport :: SetPreferredZoom( unsigned nZoomPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nZoomPos >= limits.min() ) &&  ( nZoomPos <= limits.max() ) )
        m_pProps -> nPreferredZoomPos = nZoomPos;
}

/**
 * Set the minimum zoom border limit;
 * @param nMinPos The new minimum zoom limit;
 */
void Viewport :: SetMinZoom( unsigned nMinPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nMinPos >= limits.min() ) &&  ( nMinPos <= limits.max() ) )
        m_ZoomBorderLimits.first = nMinPos;
}

/**
 * Set the maximum zoom border limit;
 * @param nMaxPos The new maximum zoom limit;
 */
void Viewport :: SetMaxZoom( unsigned nMaxPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nMaxPos >= limits.min() ) &&  ( nMaxPos <= limits.max() ) )
        m_ZoomBorderLimits.second = nMaxPos;
}

/**
 * Set zoom programatically.
 * @param nZoomPos The zoom to be applied;
 */
void Viewport :: SetZoom( unsigned nZoomPos )  {

    bool  bEnabledUserZoom = m_pProps -> bEnabledUserZoom;

    m_pProps -> bEnabledUserZoom = false;

    if( nZoomPos > m_pProps -> nCurrentZoomPos )
       ZoomIn();
    else
        if( nZoomPos < m_pProps -> nCurrentZoomPos )
            ZoomOut();

    m_pProps -> bEnabledUserZoom = bEnabledUserZoom;
}

/**
 * Reset zoom to it's default state.
 */
void Viewport :: ResetZoom( void )  {

    m_pProps -> nCurrentZoomPos = m_pProps -> nPreferredZoomPos;
    m_pProps -> fZoomFactor     = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
}

/**
 * Performs Zoom In effect.
 */
void Viewport :: ZoomIn( void )  {

    if( ( m_pProps -> nCurrentZoomPos < m_ZoomBorderLimits.second ) &&
        m_pProps -> bEnabledUserZoom )  {
        m_pProps -> nCurrentZoomPos++;

        if( m_pProps -> nCurrentZoomPos == m_ZoomBorderLimits.second )
            m_pProps -> nCurrentZoomPos--;

        m_pProps -> fZoomFactor = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
    }
}

/**
 * Performs Zoom Out effect.
 */
void Viewport :: ZoomOut( void )  {

    if( ( m_pProps -> nCurrentZoomPos > m_ZoomBorderLimits.first ) &&
        m_pProps -> bEnabledUserZoom )  {
        m_pProps -> nCurrentZoomPos--;
        m_pProps -> fZoomFactor = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
    }
}

/**
 * Set a new pointer to a new parent zoom properties to be used by this
 * ZoomEngine object;
 * @param pProps Pointer to the parent zoom properties to be set;
 */
void Viewport :: SetZoomPropertiesPtr( stZoomProperties *pProps )  {

    m_pProps = pProps;
}

/**
 * Calculates the clipped rectangle area based on
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
bool Viewport :: GetClippedArea( int32_t nSourceW,
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

    if( ( nDestX > ( m_Viewport.pos.x + m_Viewport.size.nWidth ) ) ||
        ( nDestY > ( m_Viewport.pos.y + m_Viewport.size.nHeight ) )||
        ( nDestX < 0.0 ) || ( nDestY < 0.0 ) )  {

        if( nDestX < m_Viewport.pos.x )  {
            nTemp = std :: abs( nDestX );
            nSourceW-=( nTemp < nSourceW ? nTemp : nSourceW );
        }

        if( nDestY < m_Viewport.pos.y )  {
            nTemp = std :: abs( nDestY );
            nSourceH-=( nTemp < nSourceH ? nTemp : nSourceH );
        }

        nDestX = ( nDestX < 0.0 ? 0.0 : nDestX );
        nDestY = ( nDestY < 0.0 ? 0.0 : nDestY );
    }

    fViewX      = ( ( nDestX * m_pProps -> fZoomFactor ) + m_Viewport.pos.x );
    fViewY      = ( ( nDestY * m_pProps -> fZoomFactor ) + m_Viewport.pos.y );
    fViewWidth  = ( nSourceW * m_pProps -> fZoomFactor );
    fViewHeight = ( nSourceH * m_pProps -> fZoomFactor );
    fClippingX  = ( fViewX + fViewWidth );
    fClippingY  = ( fViewY + fViewHeight );

    if( fClippingX > m_Viewport.size.nWidth )  {
        fClippingX = ( fClippingX - m_Viewport.size.nWidth );

        if( fClippingX > fViewWidth )
            return false;

        fViewWidth = fViewWidth - fClippingX;
    }

    if( fClippingY > m_Viewport.size.nHeight )  {
        fClippingY = ( fClippingY - m_Viewport.size.nHeight );

        if( fClippingY > fViewHeight )
            return false;

        fViewHeight = fViewHeight - fClippingY;
    }

    return true;
}
