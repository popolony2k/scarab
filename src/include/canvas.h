/*
 * canvas.h
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
 */

#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "basecanvas.h"
#include "collider.h"
#include "color.h"


class Canvas : public BaseCanvas {

    Collider        m_Collider;
    stColor         m_Color;


    public:

    Canvas( void );
    virtual ~Canvas( void );

    void SetParent( BaseCanvas *pParent );
    void SetDimension2DPtr( stDimension2D* pDimension );

    void SetColor( stColor color );
    stColor& GetColor( void );

    Collider& GetCollider( void );
};

#endif /* __CANVAS_H__ */
