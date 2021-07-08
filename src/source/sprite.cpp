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

    m_itSequence = m_Sequences.begin();
}

/**
 * Destructor. Finalize all class data.
 */
Sprite :: ~Sprite( void )  {

    m_Sequences.clear();
}

/**
 * Loads and add a texture to the internal texture map object.
 * @param szFileName Texture file name to load;
 */
bool Sprite :: AddSpriteSequence( int nSequence, stTexture texture ) {

    TextureSequenceList :: iterator itItem = m_Sequences.find( nSequence );

    if( itItem == m_Sequences.end() )  {
        TextureMap   textureMap;

        if( textureMap.AddTexture( texture ) )  {
            m_Sequences.insert( std :: make_pair( nSequence, textureMap ) );
            return true;
        }
    }
    else  {
        if( itItem -> second.AddTexture( texture ) )  {
            return true;
        }
    }

    return false;
}

/**
 * Set the active sprite sequence animation.
 * @param nSequence The sequence id to activate;
 */
bool Sprite :: SetActiveSequence( int nSequence )  {

    m_itSequence = m_Sequences.find( nSequence );

    return ( m_itSequence != m_Sequences.end() );
}

/**
 * Move the sprite based on x,y steps passed a parameter.
 * @param step Reference to a @link stCoordinate2D containing
 * the x,y move steps;
 */
void Sprite :: Move( stCoordinate2D& step )  {

    m_dimension.pos.x+=step.x;
    m_dimension.pos.y+=step.y;
}

/**
 * Implements the draw update method used to draw a sprite
 * object;
 */
void Sprite :: Update( void )  {

}
