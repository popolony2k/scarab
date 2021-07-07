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


class TextureMap {

    typedef std :: vector<Texture2D> TextureList;

    TextureList     m_TextureList;

    public:

    TextureMap( void );
    virtual ~TextureMap( void );

    bool AddTexture( const char *szFileName );
    bool GetTexture( int nIndex, Texture2D& texture );
    int GetTexturesCount( void );
};

#endif /* __TEXTUREMAP_H__ */
