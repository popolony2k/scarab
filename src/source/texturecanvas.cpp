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
    SetColor( WHITE_COLOR );
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

    stDimension2D& dimension = GetDimension2D();

    m_texture = ::LoadTexture( strTextureFile.c_str() );

    if( m_texture.id > 0 )  {
        if( ( dimension.size.nHeight == 0 ) &&
            ( dimension.size.nHeight == 0 )  ) {
            dimension.size.nHeight = m_texture.height;
            dimension.size.nWidth  = m_texture.width;
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

    if( GetVisible() )  {
        float          fViewX;
        float          fViewY;
        float          fClipW;
        float          fClipH;
        Viewport&      vp   = GetViewport();
        stDimension2D& vpDm = vp.GetDimension2D();
        stDimension2D& dm   = GetDimension2D();


        if( vp.GetClippedArea( dm.size.nWidth, dm.size.nHeight,
                               dm.pos.x, dm.pos.y,
                               fViewX, fViewY,
                               fClipW, fClipH ) ) {

            stColor&  color   = GetColor();
            float fZoomFactor = vp.GetZoomProperties().fZoomFactor;
            int nCutSrcWidth  = ( fViewX == vpDm.pos.x ? std :: abs( fClipW -
                                  ( dm.size.nWidth * fZoomFactor ) ) : 0 );
            int nCutSrcHeight = ( fViewY == vpDm.pos.y ? std :: abs( fClipH -
                                  ( dm.size.nHeight * fZoomFactor ) ) : 0 );

            m_nCurrentTile = ( m_nCurrentTile >= m_texture.width ? 0 :
                               m_nCurrentTile + m_nTileSize );

            ::DrawTextureTiled( m_texture,
                                 Rectangle { ( float ) m_nCurrentTile +
                                                       nCutSrcWidth,
                                             ( float ) nCutSrcHeight,
                                             ( float ) m_texture.width,
                                             ( float ) m_texture.height },
                                 Rectangle { fViewX, fViewY,
                                             fClipW, fClipH },
                                 Vector2    { 0.0, 0.0 },
                                0.0, // TODO: Rotation
                                fZoomFactor,
                                Color       { color.nRed,
                                              color.nGreen,
                                              color.nBlue,
                                              color.nAlpha } );
        }
    }
}
