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
 * Return the reference to internal @link stZoomProperties struct;
 */
stZoomProperties& Viewport :: GetZoomProperties( void )  {

    return *m_pProps;
}

/**
 * Calculates the clipped rectangle area based on
 * position and viewport boundaries;
 * @param src Source coordinates;
 * @param dst Reference to destination clipped area;
 */
bool Viewport :: GetClippedRect( stDimension2D src,
                                 stDimension2D& dst ) {

    float           fClippingX;
    float           fClippingY;
    int32_t         nTemp;
    stDimension2D&  vp = GetDimension2D();

    if( ( src.pos.x > ( vp.pos.x + vp.size.nWidth ) ) ||
        ( src.pos.y > ( vp.pos.y + vp.size.nHeight ) )||
        ( src.pos.x < 0.0 ) || ( src.pos.y < 0.0 ) )  {

        if( src.pos.x < vp.pos.x )  {
            nTemp = std :: abs( src.pos.x );
            src.size.nWidth-=( nTemp < src.size.nWidth ? nTemp :
                               src.size.nWidth );
        }

        if( src.pos.y < vp.pos.y )  {
            nTemp = std :: abs( src.pos.y );
            src.size.nHeight-=( nTemp < src.size.nHeight ? nTemp :
                                src.size.nHeight );
        }

        src.pos.x = ( src.pos.x < 0.0 ? 0.0 : src.pos.x );
        src.pos.y = ( src.pos.y < 0.0 ? 0.0 : src.pos.y );
    }

    dst.pos.x        = ( ( src.pos.x * m_pProps -> fZoomFactor ) + vp.pos.x );
    dst.pos.y        = ( ( src.pos.y * m_pProps -> fZoomFactor ) + vp.pos.y );
    dst.size.nWidth  = ( src.size.nWidth * m_pProps -> fZoomFactor );
    dst.size.nHeight = ( src.size.nHeight * m_pProps -> fZoomFactor );
    fClippingX  = ( dst.pos.x + dst.size.nWidth );
    fClippingY  = ( dst.pos.y + dst.size.nHeight );

    if( fClippingX > vp.size.nWidth )  {
        fClippingX-=vp.size.nWidth;

        if( fClippingX > dst.size.nWidth )
            return false;

        dst.size.nWidth-=fClippingX;
    }

    if( fClippingY > vp.size.nHeight )  {
        fClippingY-=vp.size.nHeight;

        if( fClippingY > dst.size.nHeight )
            return false;

        dst.size.nHeight-=fClippingY;
    }

    return true;
}
