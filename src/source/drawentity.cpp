/*
 * drawentity.cpp
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#include "drawentity.h"


/**
 * Constructor. Initialize all class data.
 */
DrawEntity :: DrawEntity( void )  {

    m_bVisible = false;
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
 * Set the dimension od this draw entity.
 * @param dimension The new dimension of this entity;
 */
void DrawEntity :: SetDimension2D( stDimension2D dimension )  {

    m_dimension = dimension;
}

/**
 * Get the reference of this entity drawing object.
 */
stDimension2D& DrawEntity :: GetDimension2D( void )  {

    return m_dimension;
}
