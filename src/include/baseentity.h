/*
 * baseentity.h
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
 */

#ifndef __BASEENTITY_H__
#define __BASEENTITY_H__

#include "worldbasedefs.h"


class BaseEntity  {

    stDimension2D    *m_pDimension;
    stDimension2D    m_Dimension;
    bool             m_bVisible;


    public:

    BaseEntity( void );
    virtual ~BaseEntity( void );

    virtual void SetVisible( bool bVisible );
    virtual bool GetVisible( void );

    virtual void SetDimension2DPtr( stDimension2D* pDimension );
    void SetDimension2D( stDimension2D dimension );
    stDimension2D& GetDimension2D( void );
};

#endif /* __BASEENTITY_H__ */
