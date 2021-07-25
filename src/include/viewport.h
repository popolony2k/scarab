/*
 * viewport.h
 *
 *  Created on: Jul 13, 2021
 *      Author: popolony2k
 */

#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__

#include "baseentity.h"
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
class Viewport : public BaseEntity {

    typedef std :: vector<float> ZoomFactorList;
    typedef std :: pair<unsigned, unsigned> ZoomBorderLimits;

    stZoomProperties           m_Props;


    void InitializeZoomEngine( void );

    public:

    Viewport( void );
    virtual ~Viewport( void );

    virtual void SetEnableUserZoom( bool bEnabled );
    virtual void SetPreferredZoom( unsigned nZoomPos );
    virtual void SetMinZoom( unsigned nMinPos );
    virtual void SetMaxZoom( unsigned nMaxPos );
    virtual void SetZoom( unsigned nZoomPos );
    virtual void ResetZoom( void );
    virtual void ZoomIn( void );
    virtual void ZoomOut( void );

    void SetZoomPropertiesPtr( stZoomProperties *pProps );
    stZoomProperties& GetZoomProperties( void );

    virtual bool GetClippedArea( int32_t nSourceW,
                                 int32_t nSourceH,
                                 int32_t nDestX,
                                 int32_t nDestY,
                                 float& fViewX,
                                 float& fViewY,
                                 float& fViewWidth,
                                 float& fViewHeight );

    protected:

    ZoomBorderLimits           m_ZoomBorderLimits;
    ZoomFactorList             m_vZoomFactorList;
    stZoomProperties           *m_pProps;
};

#endif /* __VIEWPORT_H__ */
