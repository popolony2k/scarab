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

    m_itActiveSequence = m_Sequences.begin();
    m_bIsValidSequence = ( m_itActiveSequence != m_Sequences.end() );
}

/**
 * Destructor. Finalize all class data.
 */
Sprite :: ~Sprite( void )  {

    m_Sequences.clear();
}

/**
 * Loads and add a texture to the internal texture map object.
 * @param pTexture Pointer to a @link TextureCanvas to add;
 * @param nDetalyMilli Time in millisecond to be used in texture
 * animation sequence;
 */
void Sprite :: AddSpriteSequence( int nSequence,
                                  TextureCanvas *pTexture,
                                  int64_t nDelayMilli ) {

    TextureSequenceList :: iterator itItem = m_Sequences.find( nSequence );
    stDimension2D    spritePos = GetDimension2D();


    if( ( spritePos.size.nWidth == 0 ) &&
        ( spritePos.size.nHeight == 0 ) )  {
        stDimension2D   texturePos = pTexture -> GetDimension2D();

        m_pDimension -> size.nWidth  = texturePos.size.nWidth;
        m_pDimension -> size.nHeight = texturePos.size.nHeight;
    }

    pTexture -> SetVisible( GetVisible() );
    pTexture -> SetDimensionPtr( m_pDimension );

    if( itItem == m_Sequences.end() )  {
        TextureMap   *pTextureMap = new TextureMap();

        pTextureMap -> AddTexture( pTexture, nDelayMilli );
        m_Sequences.insert( std :: make_pair( nSequence, pTextureMap ) );
    }
    else  {
        itItem -> second -> AddTexture( pTexture, nDelayMilli );
    }
}

/**
 * Set the active sprite sequence animation.
 * @param nSequence The sequence id to activate;
 */
bool Sprite :: SetActiveSequence( int nSequence )  {

    m_itActiveSequence = m_Sequences.find( nSequence );
    m_bIsValidSequence = ( m_itActiveSequence != m_Sequences.end() );

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
        if( itItem -> second -> First() )  {
            do  {
                itItem -> second -> GetTextureData().pTexture -> SetVisible( bVisible );
            } while( itItem -> second -> Next() );
        }
    }
}

/**
 * Implements the draw update method used to draw a sprite
 * object;
 */
void Sprite :: Update( void )  {

    if( m_bVisible && m_bIsValidSequence )  {
        bool          bDisableFrameUpdate = !m_itActiveSequence -> second -> Next();
        TextureCanvas *pTextureCanvas     = m_itActiveSequence -> second -> GetTextureData().pTexture;
        unsigned int  nTileSize;

        if( bDisableFrameUpdate )  {
            nTileSize = pTextureCanvas -> GetTileSize();
            pTextureCanvas -> SetTileSize( 0 );
        }

        pTextureCanvas -> Update();

        if( bDisableFrameUpdate )
            pTextureCanvas -> SetTileSize( nTileSize );

        /*TODO: Check collisions and throw collision listener
          (add collision listener or collision manager) */
    }
}
