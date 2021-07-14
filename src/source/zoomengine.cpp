/*
 * zoomengine.cpp
 *
 *  Created on: Jul 13, 2021
 *      Author: popolony2k
 */

#include "zoomengine.h"
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
void ZoomEngine ::InitializeZoomEngine( void )  {

    float   fZoomStep = 0.0;

    /*
     * Fill all zoom factor list.
     */
    for( int nCount = 0; nCount < __MAX_ZOOM_DEPTH; nCount++ )  {
        m_vZoomFactorList.push_back(fZoomStep+=__DEFAULT_MAP_ZOOM_SCALE_STEP );
    }

    m_pProps = &m_props;

    SetPreferredZoom( __DEFAULT_PREFERRED_ZOOM_POS );

    m_ZoomBorderLimits.first     = 0;
    m_ZoomBorderLimits.second    = __MAX_ZOOM_DEPTH;
    m_pProps -> bEnabledUserZoom = __DEFAULT_USER_ZOOM_STATUS;
    m_pProps -> fZoomFactor      = m_vZoomFactorList[m_pProps -> nPreferredZoomPos];
}

/**
 * Constructor. Initialize all class data.
 */
ZoomEngine :: ZoomEngine( void )  {

    InitializeZoomEngine();
}

/**
 * Destructor. Finalize all class data.
 */
ZoomEngine :: ~ZoomEngine( void )  {

}

/**
 * Enable or disable the user zoom mode (false disables user zoom by
 * mouse, keyboard, joystick, touch, ...);
 * @param bEnabled The new user zoom mode;
 */
void ZoomEngine :: SetEnableUserZoom( bool bEnabled )  {

    m_pProps -> bEnabledUserZoom = bEnabled;
}

/**
 * Set the preferred zoom to be used when engine apply
 * reset operations.
 * @param nZoomPos The new preferred zoom;
 */
void ZoomEngine :: SetPreferredZoom( unsigned nZoomPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nZoomPos >= limits.min() ) &&  ( nZoomPos <= limits.max() ) )
        m_pProps -> nPreferredZoomPos = nZoomPos;
}

/**
 * Set the minimum zoom border limit;
 * @param nMinPos The new minimum zoom limit;
 */
void ZoomEngine :: SetMinZoom( unsigned nMinPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nMinPos >= limits.min() ) &&  ( nMinPos <= limits.max() ) )
        m_ZoomBorderLimits.first = nMinPos;
}

/**
 * Set the maximum zoom border limit;
 * @param nMaxPos The new maximum zoom limit;
 */
void ZoomEngine :: SetMaxZoom( unsigned nMaxPos )  {

    std::numeric_limits<unsigned>  limits;

    if( ( nMaxPos >= limits.min() ) &&  ( nMaxPos <= limits.max() ) )
        m_ZoomBorderLimits.second = nMaxPos;
}

/**
 * Set zoom programatically.
 * @param nZoomPos The zoom to be applied;
 */
void ZoomEngine :: SetZoom( unsigned nZoomPos )  {

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
void ZoomEngine :: ResetZoom( void )  {

    m_pProps -> nCurrentZoomPos = m_pProps -> nPreferredZoomPos;
    m_pProps -> fZoomFactor     = m_vZoomFactorList[m_pProps -> nCurrentZoomPos];
}

/**
 * Performs Zoom In effect.
 */
void ZoomEngine :: ZoomIn( void )  {

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
void ZoomEngine :: ZoomOut( void )  {

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
void ZoomEngine :: SetZoomPropertiesPtr( stZoomProperties *pProps )  {

    m_pProps = pProps;
}
