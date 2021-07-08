/*
 * texture.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __TEXTUREMAP_H__
#define __TEXTUREMAP_H__

#include <raylib.h>
#include <queue>
#include <string>


/**
 * Texture structure definition.
 */
struct stTexture  {
    std :: string   strTextureFile;
    Texture2D       texture;
    int             nDelayMilli;
};


class TextureMap {

    typedef std :: deque<stTexture> TextureList;

    TextureList               m_TextureList;
    TextureList :: iterator   m_itTexture;

    public:

    TextureMap( void );
    virtual ~TextureMap( void );

    bool AddTexture( stTexture texture );

    bool First( void );
    bool Next( void );
    stTexture& GetTexture( void );
};

#endif /* __TEXTUREMAP_H__ */
