/*
 * iviewport.h
 *
 *  Created on: Jul 14, 2021
 *      Author: popolony2k
 */

#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__

#include "worldbasedefs.h"


class Viewport  {

    protected:

    stViewport          m_Viewport;


    public:

    Viewport( void );
    virtual ~Viewport( void );

    void SetViewport( stViewport viewport );
    stViewport GetViewport( void );

    /**
     * Must be implemented to calculate the clipped rectangle
     * area based on camera position and viewport boundaries;
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
    virtual bool GetClippedArea( int32_t nSourceW,
                                 int32_t nSourceH,
                                 int32_t nDestX,
                                 int32_t nDestY,
                                 float& fViewX,
                                 float& fViewY,
                                 float& fViewWidth,
                                 float& fViewHeight ) = 0;
};

#endif /* __VIEWPORT_H__ */
