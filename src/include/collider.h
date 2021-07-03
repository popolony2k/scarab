/*
 * collider.h
 *
 *  Created on: Jul 2, 2021
 *      Author: popolony2k
 */

#ifndef __COLLIDER_H__
#define __COLLIDER_H__

#include "worldbasedefs.h"


class Collider {

    stDimension2D       *m_pObjectArea;


    bool RectRect( float fRect1X,
                   float fRect1Y,
                   float fRect1Width,
                   float fRect1Height,
                   float fRect2X,
                   float fRect2Y,
                   float fRect2Width,
                   float fRect2Height );

    public:

    Collider( stDimension2D *pObjectArea );
    virtual ~Collider( void );

    bool Hit( stTile &tile );
};

#endif /* __COLLIDER_H__ */
