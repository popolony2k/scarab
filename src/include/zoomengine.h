/*
 * zoomengine.h
 *
 *  Created on: Jul 13, 2021
 *      Author: popolony2k
 */

#ifndef __ZOOMENGINE_H__
#define __ZOOMENGINE_H__

#include <vector>

/**
 * Internal zoom properties.
 */
struct stZoomProperties {
    unsigned       nCurrentZoomPos;
    unsigned       nPreferredZoomPos;
    float          fZoomFactor;
    bool           bEnabledUserZoom;
};

/**
 * Abstract class used to manage zoom engine for all
 * interface graphic components.
 */
class ZoomEngine  {

    typedef std :: vector<float> ZoomFactorList;
    typedef std :: pair<unsigned, unsigned> ZoomBorderLimits;

    stZoomProperties     m_props;


    void InitializeZoomEngine( void );

    public:

    ZoomEngine( void );
    virtual ~ZoomEngine( void );

    virtual void SetEnableUserZoom( bool bEnabled );
    virtual void SetPreferredZoom( unsigned nZoomPos );
    virtual void SetMinZoom( unsigned nMinPos );
    virtual void SetMaxZoom( unsigned nMaxPos );
    virtual void SetZoom( unsigned nZoomPos );
    virtual void ResetZoom( void );
    virtual void ZoomIn( void );
    virtual void ZoomOut( void );

    void SetZoomPropertiesPtr( stZoomProperties *pProps );

    protected:

    ZoomBorderLimits           m_ZoomBorderLimits;
    ZoomFactorList             m_vZoomFactorList;
    stZoomProperties           *m_pProps;
};

#endif /* __ZOOMENGINE_H__ */
