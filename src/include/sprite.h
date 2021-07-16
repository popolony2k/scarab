/*
 * sprite.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "collider.h"
#include "texturemap.h"
#include <map>


class Sprite : public DrawEntity  {

    typedef std :: map<int, TextureMap*>  TextureSequenceList;

    TextureSequenceList             m_Sequences;
    TextureSequenceList :: iterator m_itActiveSequence;
    bool                            m_bIsValidSequence;

    public:

    Sprite( void );
    virtual ~Sprite( void );

    void AddSpriteSequence( int nSequence,
                            TextureCanvas* pTexture,
                            int64_t nDelayMilli = -1 );
    bool SetActiveSequence( int nSequence );
    void SetVisible( bool bVisible );

    void Move( stCoordinate2D& step );

    void Update( void );
    void Unload( void );
};

#endif /* __SPRITE_H__ */
