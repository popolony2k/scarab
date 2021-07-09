/*
 * texture.cpp
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#include <chrono>
#include "texturemap.h"

using namespace std :: chrono;


/**
 * Destructor. Finalize all class data.
 */
TextureMap :: TextureMap( void )  {

}

/**
 * Destructor. Finalize all class data.
 */
TextureMap :: ~TextureMap( void )  {

    for( stTextureData texture : m_TextureList )  {
        texture.texture.Unload();
    }
}

/**
 * Loads and add a texture to the internal texture map.
 * @param strTextureFile Texture file name to load;
 * @param dimension texture dimension;
 * @param nDetalyMilli Time in millisecond to be used in texture
 * animation sequence;
 */
bool TextureMap :: AddTexture( std :: string strTextureFile,
                               stDimension2D dimension,
                               int64_t nDelayMilli )  {

    stTextureData              texture;
    steady_clock :: time_point now = steady_clock :: now();

    texture.nDelayMilli = nDelayMilli;
    texture.nNextTime   = duration_cast<milliseconds>( now.time_since_epoch() ).count();
    texture.nNextTime+=nDelayMilli;

    if( texture.texture.Load( strTextureFile ) )  {
        texture.texture.SetDimension2D( dimension );
        m_TextureList.push_back( texture );

        if( m_TextureList.size() == 1 )
            m_itTexture = m_TextureList.begin();

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
 * This function check if current texture is inside it's
 * animation update window before going to the next texture frame.
 */
bool TextureMap :: Next( void )  {

    if( m_TextureList.size() > 0 )  {
        if(m_itTexture -> nDelayMilli != -1 )  {
            steady_clock :: time_point now = steady_clock :: now();
            uint64_t nTimeMilli = duration_cast<milliseconds>( now.time_since_epoch() ).count();

            if( nTimeMilli < m_itTexture -> nNextTime )  {
                return false;
            }

            m_itTexture -> nNextTime = ( nTimeMilli + m_itTexture -> nDelayMilli );
        }

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
TextureMap :: stTextureData& TextureMap :: GetTextureData( void )  {

    return *m_itTexture;
}
