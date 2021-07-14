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
    m_nTileSize = 0;
    m_color     = WHITE_COLOR;
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

    m_nCurrentTile = 0;
}

/**
 * Set texture frame split size. Canvas can be used as sequence of animated tiles
 * in conjunction with sprites. In this case user can specify the tile split size
 * in X axis.
 * Each update will increase the step specified by this method.
 * @param nTileSize The tile size to be used to split this texture.
 */
void TextureCanvas :: SetTileSize( unsigned int nTileSize )  {

    m_nTileSize = nTileSize;
}
/**
 * Get texture frame split size.
 */
unsigned int TextureCanvas :: GetTileSize( void ) {

    return m_nTileSize;
}

/**
 * Implements the draw update method used to draw a sprite
 * object;
 */
void TextureCanvas :: Update( void )  {

    if( m_bVisible )  {
        m_nCurrentTile = ( m_nCurrentTile >= m_texture.width ? 0 :
                           m_nCurrentTile + m_nTileSize );

        ::DrawTextureTiled( m_texture,
                            ( Rectangle ) { ( float ) m_nCurrentTile,
                                            0.0,
                                            ( float ) m_texture.width,
                                            ( float ) m_texture.height },
                            ( Rectangle ) { ( float ) m_pDimension -> pos.x,
                                            ( float ) m_pDimension -> pos.y,
                                            ( float ) m_pDimension -> size.nWidth,
                                            ( float ) m_pDimension -> size.nHeight },
                            ( Vector2 )   { 0.0, 0.0 },
                            0.0, // TODO: Rotation
                            m_pProps -> fZoomFactor,
                            ( Color )       { m_color.nRed,
                                              m_color.nGreen,
                                              m_color.nBlue,
                                              m_color.nAlpha } );
    }
}
