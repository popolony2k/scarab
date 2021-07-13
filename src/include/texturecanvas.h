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
#include "drawentity.h"


class TextureCanvas : public DrawEntity  {

    Texture2D       m_texture;
    unsigned int    m_nCurrentFrame;
    unsigned int    m_nFrameSplitSize;


    public:

    TextureCanvas( void );
    virtual ~TextureCanvas( void );

    bool Load( std :: string strTextureFile );
    bool Unload( void );

    void Reset( void );

    void SetFrameSplitSize( unsigned int nFrameSplitSize );
    unsigned int GetFrameSplitSize( void );

    void Update( void );
};

#endif /* __TEXTURECANVAS_H__ */
