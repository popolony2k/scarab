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
DrawEntity :: DrawEntity( void ) {

    m_bVisible   = false;
    m_pDimension = &m_Dimension;
    std :: memset( &m_Dimension, 0, sizeof( m_Dimension ) );
    m_Collider.SetDimensionPtr( m_pDimension );
    m_Collider.SetViewportPtr( m_pViewport );
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
 * Set pointer to a new parent dimension object passed as parameter;
 * @param pDimension Pointer to the new @link stDimension2D object
 * that will be used by this entity;
 */
void DrawEntity :: SetDimension2DPtr( stDimension2D* pDimension )  {

    m_pDimension = pDimension;
    m_Collider.SetDimensionPtr( m_pDimension );
}

/**
 * Set the dimension of this draw entity.
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

/**
 * Set pointer to a new parent dimension object passed as parameter;
 * This is a override of parent method to provide passing new viewport to
 * internal dependencies.
 * @param pViewport Pointer to the new @link stViewport object
 * that will be used by this entity;
 */
void DrawEntity :: SetViewportPtr( stViewport *pViewport )  {

    Viewport :: SetViewportPtr( pViewport );
    m_Collider.SetViewportPtr( pViewport );
}

/**
 * Set the entity color.
 * @param color The RGB color to set;
 */
void DrawEntity :: SetColor( stColor color )  {

    std :: memcpy( &m_Color, &color, sizeof( color ) );
}

/**
 * Return the reference to the internal color struct.
 */
stColor& DrawEntity :: GetColor( void )  {

    return m_Color;
}

/**
 * Return the reference to the internal collider object.
 */
Collider& DrawEntity :: GetCollider( void )  {

    return m_Collider;
}
