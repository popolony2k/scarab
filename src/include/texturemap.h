/*
 * texture.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __TEXTUREMAP_H__
#define __TEXTUREMAP_H__

#include <queue>
#include <string>
#include "texturecanvas.h"


class TextureMap {

    /**
     * Texture structure definition.
     */
    struct stTexture  {
        int             nDelayMilli;
        TextureCanvas   texture;
    };

    typedef std :: deque<stTexture> TextureList;

    TextureList               m_TextureList;
    TextureList :: iterator   m_itTexture;

    public:

    TextureMap( void );
    virtual ~TextureMap( void );

    bool AddTexture( std :: string   strTextureFile,
                     int nDelayMilli = -1 );

    bool First( void );
    bool Next( void );
    TextureCanvas& GetTexture( void );
};

#endif /* __TEXTUREMAP_H__ */
