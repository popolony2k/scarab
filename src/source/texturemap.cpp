/*
 * texture.cpp
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#include "texturemap.h"


/**
 * Destructor. Finalize all class data.
 */
TextureMap :: TextureMap( void )  {

}

/**
 * Destructor. Finalize all class data.
 */
TextureMap :: ~TextureMap( void )  {

    for( Texture2D texture : m_TextureList )  {
        ::UnloadTexture( texture );
    }
}

/**
 * Loads and add a texture to the internal texture map.
 * @param szFileName Texture file name to load;
 */
bool TextureMap :: AddTexture( const char *szFileName )  {

    Texture2D   texture = ::LoadTexture( szFileName );

    if( texture.id > 0 )  {
        m_TextureList.push_back( texture );
        return true;
    }

    return false;
}

/**
 * Get a texture object based on specified index passed as
 * parameter;
 * @param nIndex Index of texture that will be retrieved;
 * @param texture Reference to the texture that will be retrieved;
 */
bool TextureMap :: GetTexture( int nIndex, Texture2D& texture )  {

    if( nIndex < m_TextureList.size() )  {
        texture = m_TextureList[nIndex];
        return true;
    }

    return false;
}

/**
 * The the number of textures inside internal texture list
 * object;
 */
int TextureMap :: GetTexturesCount( void )  {

    return m_TextureList.size();
}
