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
Sprite :: Sprite( void ) : m_Collider( m_pDimension ) {

    m_itSequence       = m_Sequences.begin();
    m_bIsValidSequence = ( m_itSequence != m_Sequences.end() );
}

/**
 * Destructor. Finalize all class data.
 */
Sprite :: ~Sprite( void )  {

    m_Sequences.clear();
}

/**
 * Loads and add a texture to the internal texture map object.
 * @param texture Texture to add;
 * @param nDetalyMilli Time in millisecond to be used in texture
 * animation sequence;
 */
void Sprite :: AddSpriteSequence( int nSequence,
                                  TextureCanvas texture,
                                  int64_t nDelayMilli ) {

    TextureSequenceList :: iterator itItem = m_Sequences.find( nSequence );
    stDimension2D    spritePos = GetDimension2D();


    if( ( spritePos.size.nWidth == 0 ) &&
        ( spritePos.size.nHeight == 0 ) )  {
        stDimension2D   texturePos = texture.GetDimension2D();

        m_pDimension -> size.nWidth  = texturePos.size.nWidth;
        m_pDimension -> size.nHeight = texturePos.size.nHeight;
    }

    texture.SetVisible( GetVisible() );
    texture.SetDimensionPtr( m_pDimension );

    if( itItem == m_Sequences.end() )  {
        TextureMap   textureMap;

        textureMap.AddTexture( texture, nDelayMilli );
        m_Sequences.insert( std :: make_pair( nSequence, textureMap ) );
    }
    else  {
        itItem -> second.AddTexture( texture, nDelayMilli );
    }
}

/**
 * Set the active sprite sequence animation.
 * @param nSequence The sequence id to activate;
 */
bool Sprite :: SetActiveSequence( int nSequence )  {

    m_itSequence       = m_Sequences.find( nSequence );
    m_bIsValidSequence = ( m_itSequence != m_Sequences.end() );

    return m_bIsValidSequence;
}

/**
 * Move the sprite based on x,y steps passed a parameter.
 * @param step Reference to a @link stCoordinate2D containing
 * the x,y move steps;
 */
void Sprite :: Move( stCoordinate2D& step )  {


    m_pDimension -> pos.x+=step.x;
    m_pDimension -> pos.y+=step.y;
}

/**
 * Set the visible status of a drawing entity.
 * @param bVisible The new visible status;
 */
void Sprite :: SetVisible( bool bVisible )  {

    TextureSequenceList :: iterator itItem;

    DrawEntity :: SetVisible( bVisible );

    for( itItem = m_Sequences.begin(); itItem != m_Sequences.end(); itItem++ )  {
        if( itItem -> second.First() )  {
            do  {
                itItem -> second.GetTextureData().texture.SetVisible( bVisible );
            } while( itItem -> second.Next() );
        }
    }
}

/**
 * Implements the draw update method used to draw a sprite
 * object;
 */
void Sprite :: Update( void )  {

    if( m_bVisible && m_bIsValidSequence )  {
        m_itSequence -> second.Next();
        m_itSequence -> second.GetTextureData().texture.Update();

        /*TODO: Check collisions and throw collision listener
          (add collision listener or collision manager) */
    }
}
