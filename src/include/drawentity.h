/*
 * Component.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __DRAWENTITY_H__
#define __DRAWENTITY_H__

#include "color.h"
#include "viewport.h"
#include "collider.h"


class DrawEntity : public Viewport  {

    private:

    stDimension2D   m_Dimension;

    protected:

    stDimension2D   *m_pDimension;
    Collider        m_Collider;
    stColor         m_Color;
    bool            m_bVisible;

    public:

    DrawEntity( void );
    virtual ~DrawEntity( void );

    virtual void SetVisible( bool bVisible );
    bool GetVisible( void );

    virtual void SetDimension2D( stDimension2D dimension );
    void SetDimensionPtr( stDimension2D* pDimension );
    stDimension2D& GetDimension2D( void );

    void SetColor( stColor color );
    stColor& GetColor( void );

    Collider& GetCollider( void );

    /**
     * Must be implemented by children objects to provide
     * it's own draw behavior.
     */
    virtual void Update( void )  {};
};

#endif /* __DRAWENTITY_H__ */
