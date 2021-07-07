/*
 * texture.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __TEXTUREMAP_H__
#define __TEXTUREMAP_H__

#include <raylib.h>
#include <vector>
#include <string>


/**
 * Texture struct definition.
 */
struct stTexture  {
    std :: string   strTextureFile;
    Texture2D       texture;
};


class TextureMap {

    typedef std :: vector<stTexture> TextureList;

    TextureList     m_TextureList;

    public:

    TextureMap( void );
    virtual ~TextureMap( void );

    bool AddTexture( const char *szFileName );
    bool GetTexture( int nIndex, stTexture& texture );
    int GetTexturesCount( void );
};

#endif /* __TEXTUREMAP_H__ */
