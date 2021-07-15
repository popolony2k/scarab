/*
 * viewport.cpp
 *
 *  Created on: Jul 14, 2021
 *      Author: popolony2k
 */

#include "viewport.h"


/**
 * Initialize all class data.
 */
Viewport :: Viewport( void )  {

    m_Viewport.pos.x = 0;
    m_Viewport.pos.y = 0;
    m_Viewport.size.nWidth  = 0;
    m_Viewport.size.nHeight = 0;
}

/**
 * Destructor. Finalize all class data.
 */
Viewport :: ~Viewport( void )  {

}

/**
 * Set the new object viewport;
 * @param viewport The new viewport rectangle;
 */
void Viewport :: SetViewport( stViewport viewport )  {

    m_Viewport = viewport;
}

/**
 * Return the object viewport information structure.
 */
stViewport Viewport :: GetViewport( void )  {

    return m_Viewport;
}
