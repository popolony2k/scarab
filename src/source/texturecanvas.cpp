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
        Viewport&      vp   = GetViewport();
        stDimension2D& vpDm = vp.GetDimension2D();
        stDimension2D& dm   = GetDimension2D();
        stDimension2D  clip;

        if( vp.GetClippedRect( dm, clip ) ) {

            /*
             * All cut operations are calculated considering the
             * base texture that is non-scaled.
             * GetClippedArea fClipW, fClipH results used in cut operation
             * have a zoom factor applied to its results, so "removing" this
             * "noise" is needed by dividing it's results by zoom factor.
             * This is needed because when texture reaches canvas boundaries
             * the texture is cut in a wrong position.
             */
            stColor&  color   = GetColor();
            float fZoomFactor = vp.GetZoomProperties().fZoomFactor;
            int   nCutSrcX    = ( clip.pos.x == vpDm.pos.x ?
                                  std :: abs( ( clip.size.nWidth /
                                                fZoomFactor ) -
                                              dm.size.nWidth ) : 0 );
            int   nCutSrcY    = ( clip.pos.y == vpDm.pos.y ?
                                  std :: abs( ( clip.size.nHeight /
                                                fZoomFactor ) -
                                              dm.size.nHeight ) : 0 );

            m_nCurrentTile = ( m_nCurrentTile >= m_texture.width ? 0 :
                               m_nCurrentTile + m_nTileSize );

            ::DrawTextureTiled( m_texture,
                                 Rectangle { ( float ) m_nCurrentTile +
                                                       nCutSrcX,
                                             ( float ) nCutSrcY,
                                             ( float ) ( m_nTileSize > 0 ?
                                                         m_nTileSize :
                                                         m_texture.width ),
                                             ( float ) m_texture.height },
                                 Rectangle { ( float ) clip.pos.x,
                                             ( float ) clip.pos.y,
                                             ( float ) clip.size.nWidth,
                                             ( float ) clip.size.nHeight },
                                 Vector2   { 0.0, 0.0 },
                                0.0, // TODO: Rotation
                                fZoomFactor,
                                Color      { color.nRed,
                                             color.nGreen,
                                             color.nBlue,
                                             color.nAlpha } );
        }
    }
}
