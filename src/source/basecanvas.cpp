/*
 * basecanvas.cpp
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
 */

#include "basecanvas.h"
#include <stdio.h>


/**
 * Constructor. Initialize all class data.
 */
BaseCanvas :: BaseCanvas( void )  {

    m_pParent   = NULL;
    m_pViewport = &m_Viewport;
}

/**
 * Destructor. Finalize all class data.
 */
BaseCanvas :: ~BaseCanvas( void )  {

}

/**
 * Set parent object.
 * @param pParent The new parent object of this object;
 */
void BaseCanvas :: SetParent( BaseCanvas *pParent )  {

    m_pParent = pParent;
}

/**
 * Return the parent object of this object;
 */
BaseCanvas* BaseCanvas :: GetParent( void )  {

    return m_pParent;
}

/**
 * Set the visible status of this entity.
 * @param bVisible The new visible status;
 */
void BaseCanvas :: SetVisible( bool bVisible )  {

    BaseEntity :: SetVisible( bVisible );
}

/**
 * Get the visible status.
 */
bool BaseCanvas :: GetVisible( void )  {

    return ( m_pParent ? m_pParent -> GetVisible() : BaseEntity :: GetVisible() );
}

/**
 * Set a new viewport object to this canvas.
 * @param pViewport The new viewport object to set;
 */
void BaseCanvas :: SetViewport( Viewport *pViewport )  {

    m_pViewport = pViewport;
}

/**
 * Get the viewport of this object. If this object is a child of
 * another object, the viewport returned is the parent viewport;
 */
Viewport& BaseCanvas :: GetViewport( void )  {

    return ( m_pParent ? m_pParent -> GetViewport() : *m_pViewport );
}
