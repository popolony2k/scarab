/*
 * texturecanvas.cpp
 *
 *  Created on: Jul 8, 2021
 *      Author: popolony2k
 */

#include <cstring>
#include "texturecanvas.h"



/**
 * Constructor. Initialize all class data.
 */
TextureCanvas :: TextureCanvas( void )  {

    std :: memset( &m_texture, 0, sizeof( m_texture ) );
    m_nFrameSplitSize = 0;
    m_color = WHITE_COLOR;
    Reset();
}

/**
 * Destructor. Finalize all class data.
 */
TextureCanvas :: ~TextureCanvas( void )  {

    Unload();
}

/**
 * Load a texture specified by parameter.
 * @param strTextureFile The texture filename to load;
 */
bool TextureCanvas :: Load( std :: string strTextureFile )  {

    m_texture = ::LoadTexture( strTextureFile.c_str() );

    if( m_texture.id > 0 )  {
        if( ( m_pDimension -> size.nHeight == 0 ) &&
            ( m_pDimension -> size.nHeight == 0 )  ) {
            m_pDimension -> size.nHeight = m_texture.height;
            m_pDimension -> size.nWidth  = m_texture.width;
        }

        return true;
    }

    return false;
}

/**
 * Unload a previously loaded texture  by @link Load method.
 */
bool TextureCanvas :: Unload( void )  {

    if( m_texture.id > 0 )  {
        ::UnloadTexture( m_texture );
        m_texture.id = 0;
        return true;
    }

    return false;
}

/**
 * Reset canvas object.
 */
void TextureCanvas :: Reset( void )  {

    m_nCurrentFrame = 0;
}

/**
 * Set texture frame split size. Canvas can be used as sequence of animated tiles
 * in conjunction with sprites. In this case user can specify the tile split size
 * in X axis.
 * Each update will increase the step specified by this method.
 * @param nFrameSplitSize The framesplit size to be used by this texture.
 */
void TextureCanvas :: SetFrameSplitSize( unsigned int nFrameSplitSize )  {

    m_nFrameSplitSize = nFrameSplitSize;
}
/**
 * Get texture frame split size.
 */
unsigned int TextureCanvas :: GetFrameSplitSize( void ) {

    return m_nFrameSplitSize;
}

/**
 * Implements the draw update method used to draw a sprite
 * object;
 */
void TextureCanvas :: Update( void )  {

    if( m_bVisible )  {
        m_nCurrentFrame = ( m_nCurrentFrame >= m_texture.width ? 0 :
                            m_nCurrentFrame + m_nFrameSplitSize );

        ::DrawTextureRec( m_texture,
                          ( Rectangle ) { ( float ) m_nCurrentFrame,
                                          0.0,
                                          ( float ) m_pDimension -> size.nWidth,
                                          ( float ) m_pDimension -> size.nHeight },
                          ( Vector2 )   { ( float ) m_pDimension -> pos.x,
                                          ( float ) m_pDimension -> pos.y },
                          ( Color )       { m_color.nRed,
                                            m_color.nGreen,
                                            m_color.nBlue,
                                            m_color.nAlpha } );
    }
}
