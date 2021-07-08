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


class TextureCanvas : DrawEntity  {

    Texture2D       texture;


    public:

    TextureCanvas( void );
    virtual ~TextureCanvas( void );

    bool Load( std :: string strTextureFile );
    bool Unload( void );

    void Update( void );
};

#endif /* __TEXTURECANVAS_H__ */
