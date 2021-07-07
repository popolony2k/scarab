/*
 * Component.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __DRAWENTITY_H__
#define __DRAWENTITY_H__

#include "worldbasedefs.h"


class DrawEntity  {

    protected:

    stDimension2D  m_dimension;
    bool           m_bVisible;

    public:

    DrawEntity( void );
    virtual ~DrawEntity( void );

    void SetVisible( bool bVisible );
    bool GetVisible( void );

    void SetDimension2D( stDimension2D dimension );
    stDimension2D& GetDimension2D( void );

    /**
     * Must be implemented by children objects to provide
     * it's own draw behavior.
     */
    virtual void Update( void )  {};
};


#endif /* __DRAWENTITY_H__ */
