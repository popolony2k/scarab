/*
 * sprite.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "collider.h"
#include "drawentity.h"
#include "texturemap.h"


class Sprite : public DrawEntity  {

    Collider       m_Collider;
    TextureMap     m_Textures;

    public:

    Sprite( void );
    virtual ~Sprite( void );

    bool AddTexture( const char *szFileName );

    void Update( void );
};

#endif /* __SPRITE_H__ */
