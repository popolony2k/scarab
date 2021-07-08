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

    for( stTexture texture : m_TextureList )  {
        ::UnloadTexture( texture.texture );
    }
}

/**
 * Loads and add a texture to the internal texture map.
 * @param szFileName Texture file name to load;
 */
bool TextureMap :: AddTexture( stTexture texture )  {

    texture.texture = ::LoadTexture( texture.strTextureFile.c_str() );

    if( texture.texture.id > 0 )  {
        m_TextureList.push_back( texture );
        return true;
    }

    return false;
}

/**
 * Get the first texture on list.
 */
bool TextureMap :: First( void )  {

    m_itTexture = m_TextureList.begin();

    return ( m_TextureList.size() > 0 );
}

/**
 * Get the next texture on list.
 */
bool TextureMap :: Next( void )  {

    if( m_TextureList.size() > 0 )  {
        m_itTexture++;

        if( m_itTexture == m_TextureList.end() )
            m_itTexture = m_TextureList.begin();

        return true;
    }

    return false;
}

/**
 * Get the current texture on list.
 */
stTexture& TextureMap :: GetTexture( void )  {

    return *m_itTexture;
}
