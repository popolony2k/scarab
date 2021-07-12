/*
 * drawentity.cpp
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#include "drawentity.h"
#include <cstring>

/**
 * Constructor. Initialize all class data.
 */
DrawEntity :: DrawEntity( void )  {

    m_bVisible   = false;
    m_pDimension = &m_dimension;
    std :: memset( m_pDimension, 0, sizeof( stDimension2D ) );
}

/**
 * Destructor. Finalize all class data.
 */
DrawEntity :: ~DrawEntity( void )  {

}

/**
 * Set the visible status of a drawing entity.
 * @param bVisible The new visible status;
 */
void DrawEntity :: SetVisible( bool bVisible )  {

    m_bVisible = bVisible;
}

/**
 * Get the visible status.
 */
bool DrawEntity :: GetVisible( void )  {

    return m_bVisible;
}

/**
 * Change the internal dimension object to another pointed
 * by pointer passed as parameter;
 * @param pDimension Pointer to the new @link stDimension2D object
 * that will be used by this entity;
 */
void DrawEntity :: SetDimensionPtr( stDimension2D* pDimension )  {

    m_pDimension = pDimension;
}

/**
 * Set the dimension od this draw entity.
 * @param dimension The new dimension of this entity;
 */
void DrawEntity :: SetDimension2D( stDimension2D dimension )  {

    std :: memcpy( m_pDimension, &dimension, sizeof( dimension ) );
}

/**
 * Get the reference of this entity drawing object.
 */
stDimension2D& DrawEntity :: GetDimension2D( void )  {

    return *m_pDimension;
}
