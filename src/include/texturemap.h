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

    public:

    /**
     * Texture structure definition.
     */
    struct stTextureData  {
        int64_t         nDelayMilli;
        int64_t         nNextTime;
        TextureCanvas   texture;
    };

    TextureMap( void );
    virtual ~TextureMap( void );

    bool AddTexture( std :: string strTextureFile,
                     stDimension2D dimension,
                     int64_t nDelayMilli = -1 );

    bool First( void );
    bool Next( void );
    TextureMap :: stTextureData& GetTextureData( void );

    private:

    typedef std :: deque<stTextureData> TextureList;

    TextureList               m_TextureList;
    TextureList :: iterator   m_itTexture;
};

#endif /* __TEXTUREMAP_H__ */
