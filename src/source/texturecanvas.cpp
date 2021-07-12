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

    std :: memset( &texture, 0, sizeof( texture ) );

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

    texture = ::LoadTexture( strTextureFile.c_str() );

    if( texture.id > 0 )  {
        if( ( m_pDimension -> size.nHeight == 0 ) &&
            ( m_pDimension -> size.nHeight == 0 )  ) {
            m_pDimension -> size.nHeight = texture.height;
            m_pDimension -> size.nWidth  = texture.width;
        }

        return true;
    }

    return false;
}

/**
 * Unload a previously loaded texture  by @link Load method.
 */
bool TextureCanvas :: Unload( void )  {

    if( texture.id > 0 )  {
        ::UnloadTexture( texture );
        return true;
    }

    return false;
}

/**
 * Implements the draw update method used to draw a sprite
 * object;
 */
void TextureCanvas :: Update( void )  {

    if( m_bVisible )  {
        ::DrawTextureRec( texture,
                          ( Rectangle ) { 0.0,
                                          0.0,
                                          ( float ) m_pDimension -> size.nWidth,
                                          ( float ) m_pDimension -> size.nHeight },
                          ( Vector2 )   { ( float ) m_pDimension -> pos.x,
                                          ( float ) m_pDimension -> pos.y },
                          ( Color )       { 254, 254, 254, 254 } );
    }
}
