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
#include "worldbasedefs.h"


class DrawEntity : public Viewport  {

    private:

    stDimension2D   m_dimension;

    protected:

    stDimension2D   *m_pDimension;
    stColor         m_color;
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

    /**
     * Must be implemented by children objects to provide
     * it's own draw behavior.
     */
    virtual void Update( void )  {};
};


#endif /* __DRAWENTITY_H__ */
