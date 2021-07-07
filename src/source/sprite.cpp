/*
 * sprite.cpp
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#include "sprite.h"


/**
 * Constructor. Initialize all class data.
 */
Sprite :: Sprite( void ) : m_Collider( &m_dimension ) {

}

/**
 * Destructor. Finalize all class data.
 */
Sprite :: ~Sprite( void )  {

}

/**
 * Loads and add a texture to the internal texture map object.
 * @param szFileName Texture file name to load;
 */
bool Sprite :: AddTexture( const char *szFileName )  {

    return m_Textures.AddTexture( szFileName );
}

/**
 * Implements the draw update method used to draw a sprite
 * object
 */
void Sprite :: Update( void )  {

}
