/*
 * texturecanvas.h
 *
 *  Created on: Jul 8, 2021
 *      Author: popolony2k
 */

#ifndef __TEXTURECANVAS_H__
#define __TEXTURECANVAS_H__

#include <raylib.h>
#include <string>
#include "drawcanvas.h"


class TextureCanvas : public DrawCanvas  {

    Texture2D       m_texture;
    unsigned int    m_nCurrentTile;
    unsigned int    m_nTileSize;


    public:

    TextureCanvas( void );
    virtual ~TextureCanvas( void );

    bool Load( std :: string strTextureFile );
    bool Unload( void );

    void Reset( void );

    void SetTileSize( unsigned int nTileSize );
    unsigned int GetTileSize( void );

    void Update( void );
};

#endif /* __TEXTURECANVAS_H__ */
